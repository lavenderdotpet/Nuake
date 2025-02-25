#pragma once
#include "src/Core/Core.h"
#include "src/Core/Timestep.h"

#include <glm/ext/vector_float3.hpp>
#include "Rigibody.h"
#include <map>
#include "RaycastResult.h"

#include <src/Core/Physics/GhostObject.h>
#include "CharacterController.h"

#include <Jolt/Jolt.h>


namespace JPH
{
	class PhysicsSystem;
	class JobSystemThreadPool;
	class ContactListener;
	class BodyActivationListener;
	class BodyInterface;
	class Shape;
	class CharacterVirtual;

	template<class T>
	class Ref;
}

namespace Nuake
{
	class BPLayerInterfaceImpl;
	class MyContactListener;
	class MyBodyActivationListener;

	namespace Physics
	{
		class DynamicWorld 
		{
		private:
			uint32_t _stepCount;

			Ref<JPH::PhysicsSystem> _JoltPhysicsSystem;
			JPH::JobSystemThreadPool* _JoltJobSystem;
			Scope<MyContactListener> _contactListener;
			Scope<MyBodyActivationListener> _bodyActivationListener;
			JPH::BodyInterface* _JoltBodyInterface;
			BPLayerInterfaceImpl* _JoltBroadphaseLayerInterface;

			std::vector<uint32_t> _registeredBodies;
			std::map<uint32_t, JPH::CharacterVirtual*> _registeredCharacters;

		public:
			DynamicWorld();

			void DrawDebug();

			void SetGravity(const Vector3& g);
			void AddRigidbody(Ref<RigidBody> rb);

			void AddGhostbody(Ref<GhostObject> gb);
			void AddCharacterController(Ref<CharacterController> cc);
			bool IsCharacterGrounded(const Entity& entity);
			// This is going to be ugly. TODO: Find a better way that passing itself as a parameter
			void MoveAndSlideCharacterController(const Entity& entity, const Vector3& velocity);
			void AddForceToRigidBody(Entity& entity, const Vector3& force);

			std::vector<RaycastResult> Raycast(const Vector3& from, const Vector3& to);
			void StepSimulation(Timestep ts);
			void Clear();

		private:
			JPH::Ref<JPH::Shape> GetJoltShape(const Ref<PhysicShape> shape);
			void SyncEntitiesTranforms();
			void SyncCharactersTransforms();

			JPH::Vec3 CreateJoltVec3(const Vector3& input) const
			{
				return JPH::Vec3(input.x, input.y, input.z);
			}
		};
	}
}
