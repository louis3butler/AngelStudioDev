#include "AngelHeuristicBoneSolver.h"
#include "AngelRigTemplate.h"
#include "Engine/StaticMesh.h"
#include "Rendering/PositionVertexBuffer.h"
#include "StaticMeshResources.h"

FAngelLandmarkSolveResult UAngelHeuristicBoneSolver::SolveLandmarks(
	UStaticMesh* Mesh,
	const UAngelRigTemplate* Template
)
{
	FAngelLandmarkSolveResult Result;
	Result.bSuccess = false;

	if (!Mesh || !Template)
	{
		return Result;
	}

	TArray<FVector> Vertices;
	if (!ExtractVertices(Mesh, Vertices) || Vertices.Num() == 0)
	{
		return Result;
	}

	TMap<EAngelLandmarkType, FVector> TypeToPos;
	ComputeHumanoidLandmarks(Vertices, TypeToPos);

	const FVector Center = ComputeCenterOfMass(Vertices);

	for (const FAngelLandmarkDefinition& Def : Template->LandmarkDefinitions)
	{
		FAngelSolvedLandmark Solved;
		Solved.Name = Def.Name;

		const FVector* FoundPos = TypeToPos.Find(Def.Type);
		const FVector Pos = FoundPos ? *FoundPos : Center;

		Solved.Transform = FTransform(Pos);
		Result.Landmarks.Add(Solved);
	}

	Result.bSuccess = true;
	return Result;
}

bool UAngelHeuristicBoneSolver::ExtractVertices(UStaticMesh* Mesh, TArray<FVector>& OutVertices) const
{
	if (!Mesh)
	{
		return false;
	}

	const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
	if (!RenderData || RenderData->LODResources.Num() == 0)
	{
		return false;
	}

	const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
	const FPositionVertexBuffer& PositionBuffer = LOD.VertexBuffers.PositionVertexBuffer;

	const int32 NumVerts = PositionBuffer.GetNumVertices();
	OutVertices.Reserve(NumVerts);

	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector3f PosF = PositionBuffer.VertexPosition(i);
		const FVector Pos = (FVector)PosF;

		OutVertices.Add(Pos);
	}

	return OutVertices.Num() > 0;
}

FVector UAngelHeuristicBoneSolver::ComputeCenterOfMass(const TArray<FVector>& Vertices) const
{
	if (Vertices.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	FVector Sum = FVector::ZeroVector;
	for (const FVector& V : Vertices)
	{
		Sum += V;
	}
	return Sum / (float)Vertices.Num();
}

void UAngelHeuristicBoneSolver::ComputeHumanoidLandmarks(
	const TArray<FVector>& Vertices,
	TMap<EAngelLandmarkType, FVector>& OutPositions
) const
{
	if (Vertices.Num() == 0)
	{
		return;
	}

	FVector Min(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const FVector& V : Vertices)
	{
		Min.X = FMath::Min(Min.X, V.X);
		Min.Y = FMath::Min(Min.Y, V.Y);
		Min.Z = FMath::Min(Min.Z, V.Z);

		Max.X = FMath::Max(Max.X, V.X);
		Max.Y = FMath::Max(Max.Y, V.Y);
		Max.Z = FMath::Max(Max.Z, V.Z);
	}

	const float Height = Max.Z - Min.Z;
	if (Height <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Center = (Min + Max) * 0.5f;

	auto InZBand = [&](const FVector& V, float ZMinPct, float ZMaxPct) -> bool
	{
		const float Pct = (V.Z - Min.Z) / Height;
		return (Pct >= ZMinPct && Pct <= ZMaxPct);
	};

	TArray<FVector> PelvisVerts;
	TArray<FVector> TorsoVerts;
	TArray<FVector> ShoulderBandVerts;
	TArray<FVector> HipBandVerts;
	TArray<FVector> KneeBandVerts;
	TArray<FVector> AnkleBandVerts;
	TArray<FVector> HeadVerts;

	for (const FVector& V : Vertices)
	{
		const float Pct = (V.Z - Min.Z) / Height;

		if (Pct >= 0.25f && Pct <= 0.45f) PelvisVerts.Add(V);
		if (Pct >= 0.35f && Pct <= 0.70f) TorsoVerts.Add(V);
		if (Pct >= 0.70f && Pct <= 0.90f) ShoulderBandVerts.Add(V);
		if (Pct >= 0.35f && Pct <= 0.55f) HipBandVerts.Add(V);
		if (Pct >= 0.15f && Pct <= 0.35f) KneeBandVerts.Add(V);
		if (Pct >= 0.00f && Pct <= 0.15f) AnkleBandVerts.Add(V);
		if (Pct >= 0.85f)                   HeadVerts.Add(V);
	}

	FVector PelvisPos = Center;
	if (PelvisVerts.Num() > 0)
	{
		PelvisPos = ComputeCenterOfMass(PelvisVerts);
		PelvisPos.X = FMath::Lerp(PelvisPos.X, Center.X, 0.7f);
		PelvisPos.Y = FMath::Lerp(PelvisPos.Y, Center.Y, 0.7f);
	}
	OutPositions.Add(EAngelLandmarkType::Pelvis, PelvisPos);

	if (TorsoVerts.Num() > 0)
	{
		TArray<FVector> LowerTorso;
		for (const FVector& V : TorsoVerts)
		{
			if (InZBand(V, 0.35f, 0.50f))
			{
				LowerTorso.Add(V);
			}
		}
		FVector SpineStart = (LowerTorso.Num() > 0) ? ComputeCenterOfMass(LowerTorso) : PelvisPos;
		SpineStart.X = FMath::Lerp(SpineStart.X, Center.X, 0.7f);
		SpineStart.Y = FMath::Lerp(SpineStart.Y, Center.Y, 0.7f);

		TArray<FVector> UpperTorso;
		for (const FVector& V : TorsoVerts)
		{
			if (InZBand(V, 0.55f, 0.75f))
			{
				UpperTorso.Add(V);
			}
		}
		FVector SpineEnd = (UpperTorso.Num() > 0) ? ComputeCenterOfMass(UpperTorso) : SpineStart;
		SpineEnd.X = FMath::Lerp(SpineEnd.X, Center.X, 0.7f);
		SpineEnd.Y = FMath::Lerp(SpineEnd.Y, Center.Y, 0.7f);

		OutPositions.Add(EAngelLandmarkType::SpineStart, SpineStart);
		OutPositions.Add(EAngelLandmarkType::SpineEnd, SpineEnd);

		FVector NeckBase = SpineEnd;
		NeckBase.Z = FMath::Min(Max.Z, NeckBase.Z + Height * 0.05f);
		OutPositions.Add(EAngelLandmarkType::NeckBase, NeckBase);
	}

	if (HeadVerts.Num() > 0)
	{
		FVector HeadCenter = ComputeCenterOfMass(HeadVerts);
		HeadCenter.X = FMath::Lerp(HeadCenter.X, Center.X, 0.7f);
		HeadCenter.Y = FMath::Lerp(HeadCenter.Y, Center.Y, 0.7f);
		OutPositions.Add(EAngelLandmarkType::HeadCenter, HeadCenter);
	}

	auto FindExtremesInBand = [&](const TArray<FVector>& BandVerts, FVector& OutLeft, FVector& OutRight) -> bool
	{
		if (BandVerts.Num() == 0)
		{
			return false;
		}

		bool bInit = false;
		for (const FVector& V : BandVerts)
		{
			if (!bInit)
			{
				OutLeft = OutRight = V;
				bInit = true;
			}
			else
			{
				if (V.X < OutLeft.X)  OutLeft = V;
				if (V.X > OutRight.X) OutRight = V;
			}
		}

		return bInit;
	};

	{
		FVector LeftShoulder(0), RightShoulder(0);
		if (FindExtremesInBand(ShoulderBandVerts, LeftShoulder, RightShoulder))
		{
			OutPositions.Add(EAngelLandmarkType::ShoulderL, LeftShoulder);
			OutPositions.Add(EAngelLandmarkType::ShoulderR, RightShoulder);
		}
	}

	{
		FVector LeftHip(0), RightHip(0);
		if (FindExtremesInBand(HipBandVerts, LeftHip, RightHip))
		{
			OutPositions.Add(EAngelLandmarkType::HipL, LeftHip);
			OutPositions.Add(EAngelLandmarkType::HipR, RightHip);
		}
	}

	{
		FVector LeftKnee(0), RightKnee(0);
		if (FindExtremesInBand(KneeBandVerts, LeftKnee, RightKnee))
		{
			OutPositions.Add(EAngelLandmarkType::KneeL, LeftKnee);
			OutPositions.Add(EAngelLandmarkType::KneeR, RightKnee);
		}
	}

	{
		FVector LeftAnkle(0), RightAnkle(0);
		if (FindExtremesInBand(AnkleBandVerts, LeftAnkle, RightAnkle))
		{
			OutPositions.Add(EAngelLandmarkType::AnkleL, LeftAnkle);
			OutPositions.Add(EAngelLandmarkType::AnkleR, RightAnkle);
		}
	}
}
