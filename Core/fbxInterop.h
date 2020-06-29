//ty hkAnimationInterop
#ifdef FBXINTEROP_EXPORTS
#define FBXINTEROP_API __declspec(dllexport)
#else
#define FBXINTEROP_API __declspec(dllimport)
#endif
#include "Vertex.h"
#include "Mesh.h"
#include "Animation.h"
#include "../Skeleton.h"

extern "C" {
	FBXINTEROP_API Animation* loadAnimation(int count, int length, unsigned char* data, char** names);
	FBXINTEROP_API void unloadAnimation(Animation* a);

	FBXINTEROP_API Skeleton* loadSkeleton(unsigned char* data, int length, short* connectBones);
	FBXINTEROP_API void unloadSkeleton(Skeleton* s);

	FBXINTEROP_API Mesh* loadMesh(int index, Vertex* vertices, int numv, unsigned short* indices, int numi, char** boneNames, int numBones);
	FBXINTEROP_API void unloadMesh(Mesh* m);

	FBXINTEROP_API int exportFbx(Mesh** meshes, int numMeshes,
									Skeleton** skeles, int numSkeles,
									Animation** anims, int totalPaps,
									const char* filename, int mode);
}
