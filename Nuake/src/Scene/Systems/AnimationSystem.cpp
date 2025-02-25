#include "AnimationSystem.h"

#include "src/Scene/Scene.h"
#include "src/Scene/Entities/Entity.h"
#include "src/Scene/Components/SkinnedModelComponent.h"
#include <src/Scene/Components/BoneComponent.h>
#include <future>

namespace Nuake
{
	AnimationSystem::AnimationSystem(Scene* scene)
	{
		m_Scene = scene;
	}

	bool AnimationSystem::Init()
	{
		return true;
	}

	void AnimationSystem::Update(Timestep ts)
	{
		auto view = m_Scene->m_Registry.view<TransformComponent, SkinnedModelComponent>();
		for (auto e : view)
		{
			auto [transformComponent, skinnedComponent] = view.get<TransformComponent, SkinnedModelComponent>(e);

			auto& model = skinnedComponent.ModelResource;
			if (!model)
			{
				continue;
			}

			Ref<SkeletalAnimation> animation = model->GetCurrentAnimation();
			if (animation && model->IsPlaying)
			{
				float newAnimationTime = animation->GetCurrentTime() + (ts * animation->GetTicksPerSecond());
				animation->SetCurrentTime(newAnimationTime);
			}

			auto& rootBone = model->GetSkeletonRootNode();
			UpdateBonePositionTraversal(rootBone, animation, animation->GetCurrentTime(), model->IsPlaying);
		}
	}

	void AnimationSystem::UpdateBonePositionTraversal(SkeletonNode& bone, Ref<SkeletalAnimation> animation, float time, bool isPlaying)
	{
		auto& animationTrack = animation->GetTrack(bone.Name);

		Entity boneEnt = m_Scene->GetEntityByID(bone.EntityHandle);
		if (boneEnt.IsValid())
		{
			auto& transformComponent = boneEnt.GetComponent<TransformComponent>();
			bone.FinalTransform = transformComponent.GetGlobalTransform() * bone.Offset;

			if (!animationTrack.IsEmpty() && isPlaying)
			{
				// Get Update transform
				animationTrack.Update(time);

				const Matrix4& finalTransform = animationTrack.GetFinalTransform();

				Vector3 localPosition;
				Quat localRotation;
				Vector3 localScale;
				Decompose(finalTransform, localPosition, localRotation, localScale);

				transformComponent.SetLocalPosition(localPosition);
				transformComponent.SetLocalRotation(localRotation);
				transformComponent.SetLocalScale(localScale);
				transformComponent.SetLocalTransform(finalTransform);
				transformComponent.Dirty = false;
			}
		}

		for (auto& childBone : bone.Children)
		{
			UpdateBonePositionTraversal(childBone, animation, time, isPlaying);
		}
	}


	void AnimationSystem::FixedUpdate(Timestep ts)
	{
		
	}

	void AnimationSystem::EditorUpdate()
	{

	}

	void AnimationSystem::Exit()
	{

	}
}
