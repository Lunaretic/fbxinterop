//ty hkAnimationInterop
#ifdef FBXINTEROP_EXPORTS
#define FBXINTEROP_API __declspec(dllexport)
#else
#define FBXINTEROP_API __declspec(dllimport)
#endif
#include "Vertex.h"
#include "Mesh.h"

extern "C" {
	FBXINTEROP_API void initFbxInterop(void);
	FBXINTEROP_API void quitFbxInterop(void);

	FBXINTEROP_API Mesh* loadMesh(int index, Vertex* vertices, int numv, unsigned short* indices, int numi, unsigned short* iBoneList, int iBoneListSize);
	FBXINTEROP_API void unloadMesh(Mesh* m);

	FBXINTEROP_API int exportFbx(Mesh** meshes, int numMeshes,
									unsigned char* skeleton, int skeletonSize,
									unsigned char* animation, int animationSize, const char** animNames,
									int* boneMap, int mapLength,
									const char* filename, int mode);
}