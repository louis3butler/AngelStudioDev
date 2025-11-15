#include "AngelLandmarkDetector.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"
#include "AngelHeuristicBoneSolver.h"
#include "Engine/StaticMesh.h"

FAngelLandmarkSolveResult UAngelLandmarkDetector::DetectLandmarks(
	UStaticMesh* Mesh,
	UAngelRigTemplate* Template)
{
	if (!Mesh || !Template)
	{
		return FAngelLandmarkSolveResult();
	}

	UAngelHeuristicBoneSolver* Solver =
		NewObject<UAngelHeuristicBoneSolver>(GetTransientPackage(), UAngelHeuristicBoneSolver::StaticClass());

	if (!Solver)
	{
		return FAngelLandmarkSolveResult();
	}

	return Solver->SolveLandmarks(Mesh, Template);
}
