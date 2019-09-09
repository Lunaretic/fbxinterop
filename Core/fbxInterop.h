//ty hkAnimationInterop
#ifdef FBXINTEROP_EXPORTS
#define FBXINTEROP_API __declspec(dllexport)
#else
#define FBXINTEROP_API __declspec(dllimport)
#endif
#include "Vertex.h"
#include "Mesh.h"
#include "Animation.h"

extern "C" {
	FBXINTEROP_API void initFbxInterop(void);
	FBXINTEROP_API void quitFbxInterop(void);

	FBXINTEROP_API Animation* loadAnimation(int count, int length, unsigned char* data, char** names);
	FBXINTEROP_API void unloadAnimation(Animation* a);

	FBXINTEROP_API Mesh* loadMesh(int index, Vertex* vertices, int numv, unsigned short* indices, int numi, unsigned short* iBoneList, int iBoneListSize);
	FBXINTEROP_API void unloadMesh(Mesh* m);

	FBXINTEROP_API int exportFbx(Mesh** meshes, int numMeshes,
									unsigned char* skeleton, int skeletonSize,
									Animation** anims, int totalPaps,
									int* boneMap, int mapLength,
									const char* filename, int mode);
}
