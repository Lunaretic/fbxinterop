// fbxInterop
// based on havok2fbx by Highflex https://github.com/Highflex/havok2fbx

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>

// HAVOK stuff now
#include <Common/Base/hkBase.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/Memory/Allocator/Malloc/hkMallocAllocator.h>
#include <Common/Base/System/Io/IStream/hkIStream.h>

#include <Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h>
#include <Common/Serialize/Util/hkStaticClassNameRegistry.h>

#include <cstdio>

// Compatibility
#include <Common/Compat/hkCompat.h>

// Scene
#include <Common/SceneData/Scene/hkxScene.h>

#include <Common/Base/Fwd/hkcstdio.h>

// Geometry
#include <Common/Base/Types/Geometry/hkGeometry.h>

// Serialize
#include <Common/Serialize/Util/hkRootLevelContainer.h>
#include <Common/Serialize/Util/hkLoader.h>
#include <Common/Serialize/Util/hkSerializeUtil.h>
#include <Common/Serialize/Version/hkVersionPatchManager.h>
#include <Common/Serialize/Data/hkDataObject.h>

// Animation
#include <Animation/Animation/Rig/hkaSkeleton.h>
#include <Animation/Animation/hkaAnimationContainer.h>
#include <Animation/Animation/Mapper/hkaSkeletonMapper.h>
#include <Animation/Animation/Playback/Control/Default/hkaDefaultAnimationControl.h>
#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>
#include <Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h>
#include <Animation/Animation/Rig/hkaPose.h>
#include <Animation/Animation/Rig/hkaSkeletonUtils.h>

// Reflection
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkClassMember.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkClassMemberAccessor.h>

// Utils
#include "hkAssetManagementUtil.h"
#include "MathHelper.h"
#include "EulerAngles.h"

// FBX
#include <fbxsdk.h>
#include "FBXCommon.h" // samples common path, todo better way
#include "fbxInterop.h"

// FBX Function prototypes
bool CreateScene(FbxManager* pSdkManager, Mesh** meshes, int meshCount, FbxScene* pScene); // create FBX scene
FbxNode* CreateMesh(FbxManager* pSdkManager, FbxScene* pScene, Mesh* m, std::string);
void CreateSkin(FbxManager* pSdkManager, FbxScene* pScene, Mesh** meshes, FbxNode** fbxMeshes, int meshCount, FbxNode* skele);
FbxNode* CreateSkeleton(FbxManager* pSdkManager, FbxScene* pScene, const char* pName); // create the actual skeleton
void AnimateSkeleton(FbxScene* pScene, FbxNode* pSkeletonRoot); // add animation to it
vector<unsigned char> uintToBytes(unsigned int u);

void PlatformInit();

static void HK_CALL errorReport(const char* msg, void* userContext)
{
	using namespace std;
	printf("%s", msg);
}

vector<unsigned char> uintToBytes(unsigned int u)
{
	vector<unsigned char> arr(4);
	for (int i = 0; i < 4; i++)
		arr[i] = u >> i * 8;
	return arr;
}

vector<float> vector4ToFloats(Vector4 v)
{
	vector<float> arr(4);

	arr[0] = v.X;
	arr[1] = v.Y;
	arr[2] = v.Z;
	arr[3] = v.W;

	return arr;
}

class hkLoader* m_loader;
class hkaSkeleton* m_skeleton;
class hkaAnimation* m_animation;
class hkaAnimationBinding* m_binding;

class hkaAnimation** animations;
class hkaAnimationBinding** bindings;

int numAnims;
int numBindings;

int* boneMap;
int mapLength;

char** animationNames;

bool bAnimationGiven = false;

FBXINTEROP_API Mesh* loadMesh(int index, Vertex* vertices, int numv, unsigned short* indices, int numi, unsigned short* iBoneList, int iBoneListSize)
{
	return new Mesh(index, vertices, numv, indices, numi, iBoneList, iBoneListSize);
}

FBXINTEROP_API void unloadMesh(Mesh* m)
{
	delete m;
}

FBXINTEROP_API int exportFbx(Mesh** meshes, int numMeshes,
								unsigned char* skeleton, int skeletonSize,
								unsigned char* animation, int animationSize, const char** animNames,
								int* map, int mLength,
								const char* filename, int mode)
{
	bAnimationGiven = animationSize > 0;

	boneMap = new int[mLength];
	memcpy(boneMap, map, sizeof(int) * mLength);
	mapLength = mLength;

	// Perform platform specific initialization for this demo - you should already have something similar in your own code.
	PlatformInit();

	// Need to have memory allocated for the solver. Allocate 1mb for it.
	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, errorReport);
	{
		// load skeleton first!
		m_loader = new hkLoader();
		{
			auto istream = new hkIstream(skeleton, skeletonSize);
			hkRootLevelContainer* container = m_loader->load(istream->getStreamReader());
			HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
			auto ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

			HK_ASSERT2(0x27343435, ac && (ac->m_skeletons.getSize() > 0), "No skeleton loaded");
			m_skeleton = ac->m_skeletons[0];
		}

		animationNames = nullptr;

		if (bAnimationGiven)
		{
			{
				auto istream = new hkIstream(animation, animationSize);
				hkRootLevelContainer* container = m_loader->load(istream->getStreamReader());
				HK_ASSERT2(0x27343437, container != HK_NULL, "Could not load asset");
				auto ac = reinterpret_cast<hkaAnimationContainer*>(container->findObjectByType(hkaAnimationContainerClass.getName()));

				HK_ASSERT2(0x27343435, ac && (ac->m_animations.getSize() > 0), "No animation loaded");
				numAnims = ac->m_animations.getSize();

				animationNames = new char*[numAnims];
				
				animations = new hkaAnimation*[numAnims];
				for (int i = 0 ; i < numAnims; i++)
				{
					animations[i] = ac->m_animations[i];

					// cpp strings are hard
					animationNames[i] = new char[strlen(animNames[i]) * sizeof(char) + 1];
					strncpy_s(animationNames[i], strlen(animNames[i]) * sizeof(char) + 1, animNames[i], _TRUNCATE);
				}

				HK_ASSERT2(0x27343435, ac && (ac->m_bindings.getSize() > 0), "No binding loaded");
				numBindings = ac->m_bindings.getSize();
				
				bindings = new hkaAnimationBinding*[numBindings];
				for (int i = 0; i < numBindings; i++)
					bindings[i] = ac->m_bindings[i];
			}
		}
	}

	// Prepare FBX stuff
	FbxManager* lSdkManager = nullptr;
	FbxScene* lScene = nullptr;

	InitializeSdkObjects(lSdkManager, lScene);

	// Create the scene.
	bool lResult = CreateScene(lSdkManager, meshes, numMeshes, lScene);

	if (!lResult)
	{
		FBXSDK_printf("\n\nAn error occurred while creating the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return 1;
	}

	// Save the scene to FBX.
	// 0 is binary
	lResult = SaveScene(lSdkManager, lScene, filename, mode);

	if (!lResult)
	{
		FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return 1;
	}

	//Cleanup
	delete[] boneMap;
	delete[] animationNames;

	DestroySdkObjects(lSdkManager, lResult);

	hkBaseSystem::quit();
	hkMemoryInitUtil::quit();

	return 0;
}

bool CreateScene(FbxManager *pSdkManager, Mesh** meshes, int meshCount, FbxScene* pScene)
{
	// create scene info
	FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(pSdkManager, "SceneInfo");
	sceneInfo->mTitle = "Extracted Havok Animation";
	sceneInfo->mSubject = "A file extracted from FFXIV and converted from Havok formats to FBX using Godbert.";
	sceneInfo->mAuthor = "Godbert";
	sceneInfo->mRevision = "rev. 1.0";
	sceneInfo->mKeywords = "havok animation ffxiv";

	FbxAxisSystem directXAxisSys(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityEven, FbxAxisSystem::eRightHanded);
	directXAxisSys.ConvertScene(pScene);

	// we need to add the sceneInfo before calling AddThumbNailToScene because
	// that function is asking the scene for the sceneInfo.
	pScene->SetSceneInfo(sceneInfo);
	FbxNode* lSkeletonRoot = CreateSkeleton(pSdkManager, pScene, "Skeleton");
	
	// Build the node tree.
	FbxNode* lRootNode = pScene->GetRootNode();
	lRootNode->AddChild(lSkeletonRoot);

	auto fbxMeshes = new FbxNode*[meshCount];
	
	for (int i = 0; i < meshCount; i++)
	{
		FbxNode* thisFbxMesh = CreateMesh(pSdkManager, pScene, meshes[i], std::string("Mesh ") + std::to_string(i));
		lRootNode->AddChild(thisFbxMesh);
	
		fbxMeshes[i] = thisFbxMesh;
	}
	
	CreateSkin(pSdkManager, pScene, meshes, fbxMeshes, meshCount, lSkeletonRoot);

	// Animation only if specified
	if (bAnimationGiven)
		AnimateSkeleton(pScene, lSkeletonRoot);

	delete[] fbxMeshes;
	return true;
}

// Utility to make sure we always return the right index for the given node
// Very handy for skeleton hierachy work in the FBX SDK
FbxNode* GetNodeIndexByName(FbxScene* pScene, std::string NodeName)
{
	// temp hacky
	FbxNode* NodeToReturn = FbxNode::Create(pScene, "empty");

	for (int i = 0; i < pScene->GetNodeCount(); i++)
	{
		std::string CurrentNodeName = pScene->GetNode(i)->GetName();

		if (CurrentNodeName == NodeName)
			NodeToReturn = pScene->GetNode(i);
	}

	return NodeToReturn;
}

int GetNodeIDByName(FbxScene* pScene, std::string NodeName)
{
	int NodeNumber = 0;

	for (int i = 0; i < pScene->GetNodeCount(); i++)
	{
		std::string CurrentNodeName = pScene->GetNode(i)->GetName();

		if (CurrentNodeName == NodeName)
		{
			NodeNumber = i;
		}
	}

	return NodeNumber;
}

FbxNode* CreateMesh(FbxManager *pSdkManager, FbxScene* pScene, Mesh* m, std::string name)
{
	FbxLayerContainer* container = FbxLayerContainer::Create(pSdkManager, (std::string("Mesh ") + std::to_string(m->index)).c_str());
	FbxMesh* fbxMesh = FbxMesh::Create(pScene, name.c_str());
	fbxMesh->InitControlPoints(m->numVertices);
	fbxMesh->InitNormals(m->numVertices);

	FbxGeometryElementVertexColor* colorElement = fbxMesh->CreateElementVertexColor();
	colorElement->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);

	FbxGeometryElementUV* uvElement = fbxMesh->CreateElementUV("set");
	uvElement->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);

	FbxGeometryElementTangent* tangentElement = fbxMesh->CreateElementTangent();
	tangentElement->SetMappingMode(FbxLayerElement::EMappingMode::eByControlPoint);
		
	for (int i = 0; i < m->numVertices; i++)
	{
		auto pos = FbxVector4(m->vertices[i].Position.X,
								m->vertices[i].Position.Y,
								m->vertices[i].Position.Z,
								m->vertices[i].Position.W);

		auto norm = FbxVector4(m->vertices[i].Normal.X,
								m->vertices[i].Normal.Y,
								m->vertices[i].Normal.Z,
								m->vertices[i].Normal.W);

		auto uv = FbxVector4(m->vertices[i].UV.X,
								m->vertices[i].UV.Y,
								m->vertices[i].UV.Z,
								m->vertices[i].UV.W);

		auto colors = FbxColor(m->vertices[i].Color.X,
									m->vertices[i].Color.Y,
									m->vertices[i].Color.Z,
									m->vertices[i].Color.W);

		auto tangent1 = FbxVector4(m->vertices[i].Tangent1.X,
									m->vertices[i].Tangent1.Y,
									m->vertices[i].Tangent1.Z,
									m->vertices[i].Tangent1.W);
		
		auto tangent2 = FbxVector4(m->vertices[i].Tangent2.X,
									m->vertices[i].Tangent2.Y,
									m->vertices[i].Tangent2.Z,
									m->vertices[i].Tangent2.W);
		
		fbxMesh->SetControlPointAt(pos, norm, i);

		uvElement->GetDirectArray().Add(FbxVector2(m->vertices[i].UV.X, m->vertices[i].UV.Y * -1));

		// Color
		colorElement->GetDirectArray().Add(colors);

		// Tangents
		tangentElement->GetDirectArray().Add(tangent1);
		tangentElement->GetDirectArray().Add(tangent2);
		
	}

	for (int i = 0; i < m->numIndices; i += 3)
	{
		fbxMesh->BeginPolygon();
		fbxMesh->AddPolygon(m->indices[i]);
		fbxMesh->AddPolygon(m->indices[i + 1]);
		fbxMesh->AddPolygon(m->indices[i + 2]);
		fbxMesh->EndPolygon();
	}

	FbxNode* meshNode = FbxNode::Create(pScene, name.c_str());
	meshNode->SetNodeAttribute(fbxMesh);

	return meshNode;
}

void CreateSkin(FbxManager* pSdkManager, FbxScene* pScene, Mesh** originalMeshes, FbxNode** meshes, int numMeshes, FbxNode* skele)
{
	FbxArray<FbxNode*> nodes;

	for (int j = 0; j < numMeshes; j++)
	{
		FbxMesh* thisMesh = reinterpret_cast<FbxMesh*>(meshes[j]->GetNodeAttributeByIndex(0));
		nodes.Add(thisMesh->GetNode());

		FbxSkin* skin = FbxSkin::Create(pSdkManager, std::string("Mesh " + std::to_string(j) + " skin").c_str());
		skin->SetSkinningType(FbxSkin::eBlend);
		skin->SetGeometry(thisMesh);
		thisMesh->AddDeformer(skin);

		for (int i = 0; i < mapLength; i++)
		{
			int map = boneMap[i];

			std::string clusterName = std::string("Mesh ") + std::to_string(j) + std::string(m_skeleton->m_bones[map].m_name.cString()) + std::string(" cluster");
			FbxCluster* thisMeshBoneCluster = FbxCluster::Create(pSdkManager, clusterName.c_str());
			thisMeshBoneCluster->SetLinkMode(FbxCluster::ELinkMode::eNormalize);
			thisMeshBoneCluster->SetLink(GetNodeIndexByName(pScene, m_skeleton->m_bones[map].m_name.cString()));
			thisMeshBoneCluster->SetTransformLinkMatrix(GetNodeIndexByName(pScene, m_skeleton->m_bones[map].m_name.cString())->EvaluateGlobalTransform());

			for (int k = 0; k < originalMeshes[j]->numVertices; k++)
			{
				auto blendIndices = uintToBytes(originalMeshes[j]->vertices[k].BlendIndices);
				auto blendWeights = vector4ToFloats(originalMeshes[j]->vertices[k].BlendWeights);

				for (unsigned int blendIndex = 0; blendIndex < blendIndices.size(); blendIndex++)
				{
					// ensure the blend index is in line with the remapping for this mesh
					int oldbi = blendIndices[blendIndex];
					bool applies = false;
					blendIndices[blendIndex] = originalMeshes[j]->boneList[blendIndices[blendIndex]];

					if (oldbi != blendIndices[blendIndex])
					{
						// swap again idek
						blendIndices[blendIndex] = boneMap[blendIndices[blendIndex]];

						// compare against map
						applies = blendIndices[blendIndex] == map;
					}
					else
					{
						// compare against i
						applies = blendIndices[blendIndex] == i;
					}

					// make sure index has weight
					if (blendWeights[blendIndex] != 0 && applies)
						thisMeshBoneCluster->AddControlPointIndex(k, blendWeights[blendIndex]);
				}
			}
			// Don't let this cluster deform this skin if we haven't added any indices.
			// If we add it, it can't differentiate indices between meshes, and affects
			// all control point indices on all meshes as if they were the ones set.
			// does this even work lol
			if (thisMeshBoneCluster->GetControlPointIndicesCount() > 0)
			{
				skin->AddCluster(thisMeshBoneCluster);
				nodes.Add(thisMeshBoneCluster->GetLink());
			}
		}
	}


	// Dunno if this much bind pose management is necessary
	FbxPose* pose = pScene->GetPose(0);

	for (int i = 0; i < nodes.GetCount(); i++)
	{
		FbxNode* clusterLink = nodes.GetAt(i);
		FbxMatrix bind = clusterLink->EvaluateGlobalTransform();
	
		pose->Add(clusterLink, bind);
	}
}

// Create the skeleton first
FbxNode* CreateSkeleton(FbxManager* pSdkManager, FbxScene* pScene, const char* pName)
{
	// get number of bones and apply reference pose
	const int numBones = m_skeleton->m_bones.getSize();

	FbxArray<FbxNode*> nodes;

	// create base limb objects first
	for (hkInt16 b = 0; b < numBones; b++)
	{
		const hkaBone& bone = m_skeleton->m_bones[b];

		hkQsTransform localTransform = m_skeleton->m_referencePose[b];
		const hkVector4& pos = localTransform.getTranslation();
		const hkQuaternion& rot = localTransform.getRotation();

		FbxSkeleton* lSkeletonLimbNodeAttribute1 = FbxSkeleton::Create(pScene, pName);

		if (b == 0)
			lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eRoot);
		else
			lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eLimbNode);

		lSkeletonLimbNodeAttribute1->Size.Set(1.0);
		std::string baseJointName = std::string(bone.m_name);
		FbxNode* BaseJoint = FbxNode::Create(pScene, baseJointName.c_str());
		BaseJoint->SetNodeAttribute(lSkeletonLimbNodeAttribute1);

		// Set Translation
		BaseJoint->LclTranslation.Set(FbxVector4(pos.getSimdAt(0), pos.getSimdAt(1), pos.getSimdAt(2)));

		// convert quat to euler
		Quat QuatTest = { rot.m_vec.getSimdAt(0), rot.m_vec.getSimdAt(1), rot.m_vec.getSimdAt(2), rot.m_vec.getSimdAt(3) };
		EulerAngles inAngs = Eul_FromQuat(QuatTest, EulOrdXYZs);
		BaseJoint->LclRotation.Set(FbxVector4(rad2deg(inAngs.x), rad2deg(inAngs.y), rad2deg(inAngs.z)));

		nodes.Add(BaseJoint);
		pScene->GetRootNode()->AddChild(BaseJoint);
	}

	// process parenting and transform now
	for (int c = 0; c < numBones; c++)
	{
		const hkInt32& parent = m_skeleton->m_parentIndices[c];

		if (parent != -1)
		{
			const char* ParentBoneName = m_skeleton->m_bones[parent].m_name;
			const char* CurrentBoneName = m_skeleton->m_bones[c].m_name;
			std::string CurBoneNameString = CurrentBoneName;
			std::string ParentBoneNameString = ParentBoneName;

			FbxNode* ParentJointNode = GetNodeIndexByName(pScene, ParentBoneName);
			FbxNode* CurrentJointNode = GetNodeIndexByName(pScene, CurrentBoneName);
			ParentJointNode->AddChild(CurrentJointNode);
		}
	}

	// set bind post stuff
	FbxPose* pose = FbxPose::Create(pSdkManager, "Bindpose");
	pose->SetIsBindPose(true);
	
	for (int i = 0; i < nodes.GetCount(); i++)
	{
		FbxNode* node = nodes.GetAt(i);
		FbxMatrix bind = node->EvaluateGlobalTransform();
	
		pose->Add(node, bind);
	}
	
	pScene->AddPose(pose);

	return pScene->GetRootNode();
}

// Create animation stack.
void AnimateSkeleton(FbxScene* pScene, FbxNode* pSkeletonRoot)
{
	for (int a = 0; a < numAnims; a++)
	{
		FbxString lAnimStackName;
		FbxTime lTime;
		int lKeyIndex = 0;

		lAnimStackName = animationNames[a];
		FbxAnimStack* lAnimStack = FbxAnimStack::Create(pScene, lAnimStackName);

		// The animation nodes can only exist on AnimLayers therefore it is mandatory to
		// add at least one AnimLayer to the AnimStack. And for the purpose of this example,
		// one layer is all we need.
		FbxAnimLayer* lAnimLayer = FbxAnimLayer::Create(pScene, "Base Layer");
		lAnimStack->AddMember(lAnimLayer);

		// havok related animation stuff now
		const int numBones = m_skeleton->m_bones.getSize();

		int FrameNumber = animations[a]->getNumOriginalFrames();
		int TrackNumber = animations[a]->m_numberOfTransformTracks;
		int FloatNumber = animations[a]->m_numberOfFloatTracks;

		float AnimDuration = animations[a]->m_duration;
		hkReal incrFrame = animations[a]->m_duration / (hkReal)(FrameNumber - 1);

		hkLocalArray<float> floatsOut(FloatNumber);
		hkLocalArray<hkQsTransform> transformOut(TrackNumber);
		floatsOut.setSize(FloatNumber);
		transformOut.setSize(TrackNumber);
		hkReal startTime = 0.0;

		hkArray<hkInt16> tracks;
		tracks.setSize(TrackNumber);
		for (int i = 0; i<TrackNumber; ++i) tracks[i] = i;

		hkReal time = startTime;
	
		// used to store the bone id used inside the FBX scene file
		int* BoneIDContainer;
		BoneIDContainer = new int[numBones];

		// store IDs once to cut down process time
		for (int y = 0; y < numBones; y++)
		{
			const char* CurrentBoneName = m_skeleton->m_bones[y].m_name;
			std::string CurBoneNameString = CurrentBoneName;
			BoneIDContainer[y] = GetNodeIDByName(pScene, CurrentBoneName);
		}

		//hmm
		hkaAnimatedSkeleton* animatedSkeleton = new hkaAnimatedSkeleton(m_skeleton);
		hkaAnimationControl* control = new hkaDefaultAnimationControl(bindings[a]);
		animatedSkeleton->addAnimationControl(control);

		hkaPose pose(animatedSkeleton->getSkeleton());

		// loop through keyframes
		for (int iFrame = 0; iFrame<FrameNumber; ++iFrame, time += incrFrame)
		{
			control->setLocalTime(time);
			animatedSkeleton->sampleAndCombineAnimations(pose.accessUnsyncedPoseModelSpace().begin(), pose.getFloatSlotValues().begin());
			const hkQsTransform* transforms = pose.getSyncedPoseModelSpace().begin();

			// assume 1-to-1 transforms
			// loop through animated bones
			for (int i = 0; i < TrackNumber; ++i)
			{
				FbxNode* CurrentJointNode = pScene->GetNode(BoneIDContainer[i]);

				// create curves on frame zero otherwise just get them
				const bool pCreate = iFrame == 0;

				// Translation
				FbxAnimCurve* lCurve_Trans_X = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
				FbxAnimCurve* lCurve_Trans_Y = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
				FbxAnimCurve* lCurve_Trans_Z = CurrentJointNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

				// Rotation
				FbxAnimCurve* lCurve_Rot_X = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
				FbxAnimCurve* lCurve_Rot_Y = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
				FbxAnimCurve* lCurve_Rot_Z = CurrentJointNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

				FbxAnimCurve* lCurve_Scal_X = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, pCreate);
				FbxAnimCurve* lCurve_Scal_Y = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, pCreate);
				FbxAnimCurve* lCurve_Scal_Z = CurrentJointNode->LclScaling.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, pCreate);

				hkQsTransform transform = transforms[i];
				const hkVector4f anim_pos = transform.getTranslation();
				const hkQuaternion anim_rot = transform.getRotation();
				const hkVector4f anim_scal = transform.getScale();

				//30 fps
				lTime.SetTime(0, 0, 0, iFrame, 0, 0, lTime.eFrames30);

				FbxAnimCurveDef::EInterpolationType anim_curve_def = FbxAnimCurveDef::eInterpolationConstant;

				// Translation first
				lCurve_Trans_X->KeyModifyBegin();
				lKeyIndex = lCurve_Trans_X->KeyAdd(lTime);
				lCurve_Trans_X->KeySetValue(lKeyIndex, anim_pos.getSimdAt(0));
				lCurve_Trans_X->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Trans_X->KeyModifyEnd();

				lCurve_Trans_Y->KeyModifyBegin();
				lKeyIndex = lCurve_Trans_Y->KeyAdd(lTime);
				lCurve_Trans_Y->KeySetValue(lKeyIndex, anim_pos.getSimdAt(1));
				lCurve_Trans_Y->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Trans_Y->KeyModifyEnd();

				lCurve_Trans_Z->KeyModifyBegin();
				lKeyIndex = lCurve_Trans_Z->KeyAdd(lTime);
				lCurve_Trans_Z->KeySetValue(lKeyIndex, anim_pos.getSimdAt(2));
				lCurve_Trans_Z->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Trans_Z->KeyModifyEnd();

				// Rotation
				Quat QuatRotNew = { anim_rot.m_vec.getSimdAt(0), anim_rot.m_vec.getSimdAt(1), anim_rot.m_vec.getSimdAt(2), anim_rot.m_vec.getSimdAt(3) };
				EulerAngles inAngs_Animation = Eul_FromQuat(QuatRotNew, EulOrdXYZs);

				lCurve_Rot_X->KeyModifyBegin();
				lKeyIndex = lCurve_Rot_X->KeyAdd(lTime);
				lCurve_Rot_X->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.x)));
				lCurve_Rot_X->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Rot_X->KeyModifyEnd();

				lCurve_Rot_Y->KeyModifyBegin();
				lKeyIndex = lCurve_Rot_Y->KeyAdd(lTime);
				lCurve_Rot_Y->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.y)));
				lCurve_Rot_Y->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Rot_Y->KeyModifyEnd();

				lCurve_Rot_Z->KeyModifyBegin();
				lKeyIndex = lCurve_Rot_Z->KeyAdd(lTime);
				lCurve_Rot_Z->KeySetValue(lKeyIndex, float(rad2deg(inAngs_Animation.z)));
				lCurve_Rot_Z->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Rot_Z->KeyModifyEnd();

				// Scale
				lCurve_Scal_X->KeyModifyBegin();
				lKeyIndex = lCurve_Scal_X->KeyAdd(lTime);
				lCurve_Scal_X->KeySetValue(lKeyIndex, anim_scal.getSimdAt(0));
				lCurve_Scal_X->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Scal_X->KeyModifyEnd();

				lCurve_Scal_Y->KeyModifyBegin();
				lKeyIndex = lCurve_Scal_Y->KeyAdd(lTime);
				lCurve_Scal_Y->KeySetValue(lKeyIndex, anim_scal.getSimdAt(1));
				lCurve_Scal_Y->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Scal_Y->KeyModifyEnd();

				lCurve_Scal_Z->KeyModifyBegin();
				lKeyIndex = lCurve_Scal_Z->KeyAdd(lTime);
				lCurve_Scal_Z->KeySetValue(lKeyIndex, anim_scal.getSimdAt(2));
				lCurve_Scal_Z->KeySetInterpolation(lKeyIndex, anim_curve_def);
				lCurve_Scal_Z->KeyModifyEnd();
			}
		}
	}
}

// [id=keycode]
#include <Common/Base/keycode.cxx>

// [id=productfeatures]
// We're not using anything product specific yet. We undef these so we don't get the usual
// product initialization for the products.
#undef HK_FEATURE_PRODUCT_AI
//#undef HK_FEATURE_PRODUCT_ANIMATION
#undef HK_FEATURE_PRODUCT_CLOTH
#undef HK_FEATURE_PRODUCT_DESTRUCTION_2012
#undef HK_FEATURE_PRODUCT_DESTRUCTION
#undef HK_FEATURE_PRODUCT_BEHAVIOR
#undef HK_FEATURE_PRODUCT_PHYSICS_2012
#undef HK_FEATURE_PRODUCT_SIMULATION
#undef HK_FEATURE_PRODUCT_PHYSICS

// We can also restrict the compatibility to files created with the current version only using HK_SERIALIZE_MIN_COMPATIBLE_VERSION.
// If we wanted to have compatibility with at most version 650b1 we could have used something like:
// #define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 650b1.
#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 201130r1 //FFXIV is compatible with 201130r1

#include <Common/Base/Config/hkProductFeatures.cxx>

// Platform specific initialization
#include <Common/Base/System/Init/PlatformInit.cxx>