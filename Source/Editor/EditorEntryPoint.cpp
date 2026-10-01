#include "EnginePCH.h"

#include "Launch/EntryPoint.h"

#include "Editor/HitoriEd/EditorEngine.h"
#include "Programs/ObjViewer/ObjViewerApp.h"
#include "Programs/Benchmark/BenchmarkApp.h"

UClass* GetEngineClass()
{
#ifdef OBJ_VIEWER
	return UObjViewerEngine::StaticClass();
#elif BENCHMARK
	return UBenchmarkEngine::StaticClass();
#else
	return UEditorEngine::StaticClass();
#endif
}
