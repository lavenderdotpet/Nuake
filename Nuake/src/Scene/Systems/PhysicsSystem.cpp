#include "PhysicsSystem.h"
#include "src/Scene/Scene.h"
#include "src/Scene/Components/BoxCollider.h"
#include "src/Scene/Components/CapsuleColliderComponent.h"
#include "src/Scene/Components/SphereCollider.h"
#include "src/Scene/Components/MeshCollider.h"
#include "src/Scene/Components/ModelComponent.h"
#include <src/Scene/Components/RigidbodyComponent.h>
#include "src/Scene/Entities/Entity.h"
#include <src/Core/Physics/PhysicsManager.h>
#include <src/Scene/Components/CharacterControllerComponent.h>
#include <src/Scene/Components/QuakeMap.h>
#include <src/Scene/Components/BSPBrushComponent.h>
#include <src/Scene/Components/TriggerZone.h>
#include <src/Scene/Components/CylinderColliderComponent.h>

namespace Nuake
{
	PhysicsSystem::PhysicsSystem(Scene* scene)
	{
		m_Scene = scene;
	}

	bool PhysicsSystem::Init()
	{
		Logger::Log("Initializing the physic system");

		// We need to initialize shapes first, then bodies...
		InitializeShapes();

		InitializeRigidbodies();
		InitializeCharacterControllers();
		InitializeQuakeMap();

		Logger::Log("Physic system initialized successfully");
		return true;
	}

	void PhysicsSystem::Update(Timestep ts)
	{
		if (!Engine::IsPlayMode())
		{
			return;
		}

		ApplyForces();

		PhysicsManager::Get().Step(ts);

		auto brushes = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent>();
		for (auto e : brushes)
		{
			auto [transform, brush] = brushes.get<TransformComponent, BSPBrushComponent>(e);

			for (auto& r : brush.Rigidbody)
			{
				//r->m_Transform->setOrigin(btVector3(transform.GlobalTranslation.x, transform.GlobalTranslation.y, transform.GlobalTranslation.z));
				//r->UpdateTransform(*r->m_Transform);
			}

			if (!brush.IsFunc)
				continue;

			brush.Targets.clear();
			auto targetnameView = m_Scene->m_Registry.view<TransformComponent, NameComponent>();
			for (auto e2 : targetnameView)
			{
				auto [ttransform, name] = targetnameView.get<TransformComponent, NameComponent>(e2);

				if (name.Name == brush.target)
				{
					brush.Targets.push_back(Entity{ e2, m_Scene });
				}
			}
		}

		//auto bspTriggerView = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent, TriggerZone>();
		//for (auto e : bspTriggerView)
		//{
		//	auto [transform, brush, trigger] = bspTriggerView.get<TransformComponent, BSPBrushComponent, TriggerZone>(e);
		//	trigger.GhostObject->ScanOverlap();

		//	brush.Targets.clear();
		//	auto targetnameView = m_Scene->m_Registry.view<TransformComponent, NameComponent>();
		//	for (auto e2 : targetnameView)
		//	{
		//		auto [ttransform, name] = targetnameView.get<TransformComponent, NameComponent>(e2);

		//		if (name.Name == brush.target) {
		//			brush.Targets.push_back(Entity{ e2, m_Scene });
		//		}
		//	}
		//}


		/*auto physicGroup = m_Scene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : physicGroup) {
			auto [transform, rb] = physicGroup.get<TransformComponent, RigidBodyComponent>(e);
			rb.SyncTransformComponent(&m_Scene->m_Registry.get<TransformComponent>(e));
		}*/

		//auto ccGroup = m_Scene->m_Registry.view<TransformComponent, CharacterControllerComponent>();
		//for (auto e : ccGroup) {
		//	auto [transform, rb] = ccGroup.get<TransformComponent, CharacterControllerComponent>(e);
		//	rb.SyncWithTransform(m_Scene->m_Registry.get<TransformComponent>(e));
		//}
	}

	void PhysicsSystem::FixedUpdate(Timestep ts)
	{
		if (!Engine::IsPlayMode())
			return;

		InitializeRigidbodies();
		
	}

	void PhysicsSystem::Exit()
	{
		PhysicsManager::Get().Reset();
	}

	void PhysicsSystem::InitializeQuakeMap()
	{
		auto quakeBrushesView = m_Scene->m_Registry.view<TransformComponent, BSPBrushComponent, ModelComponent>();
		for (auto e : quakeBrushesView)
		{
			const auto [transformComponent, brushComponent, modelComponent] = quakeBrushesView.get<TransformComponent, BSPBrushComponent, ModelComponent>(e);

			// Doesn't apply to non-solid brushes
			if (!brushComponent.IsSolid)
			{
				continue;
			}

			for (const auto& hull : brushComponent.Hulls)
			{
				const auto entity = Entity({ e, m_Scene });
				const Vector3& startPosition = transformComponent.GetGlobalPosition();
				const Matrix4& startTransform = transformComponent.GetGlobalTransform();
				const Quat& startRotation = transformComponent.GetGlobalRotation();
				const auto collisionShape = CreateRef<Physics::ConvexHullShape>(hull);

				auto rigidBody = CreateRef<Physics::RigidBody>(0.0f, startPosition, startRotation, startTransform, collisionShape, entity);
				brushComponent.Rigidbody.push_back(rigidBody);

				PhysicsManager::Get().RegisterBody(rigidBody);
			}
		}
	}

	void PhysicsSystem::InitializeShapes()
	{
		const auto boxView = m_Scene->m_Registry.view<BoxColliderComponent>();
		for (auto entity : boxView)
		{
			auto& boxComponent = boxView.get<BoxColliderComponent>(entity);
			boxComponent.Box = CreateRef<Physics::Box>(boxComponent.Size);
		}

		const auto capsuleView = m_Scene->m_Registry.view<CapsuleColliderComponent>();
		for (auto entity : capsuleView)
		{
			auto& capsuleComponent = capsuleView.get<CapsuleColliderComponent>(entity);
			float radius = capsuleComponent.Radius;
			float height = capsuleComponent.Height;
			capsuleComponent.Capsule = CreateRef<Physics::Capsule>(radius, height);
		}

		const auto cylinderView = m_Scene->m_Registry.view<CylinderColliderComponent>();
		for (auto entity : cylinderView)
		{
			auto& cylinderComponent = cylinderView.get<CylinderColliderComponent>(entity);
			float radius = cylinderComponent.Radius;
			float height = cylinderComponent.Height;
			cylinderComponent.Cylinder = CreateRef<Physics::Cylinder>(radius, height);
		}

		const auto sphereView = m_Scene->m_Registry.view<SphereColliderComponent>();
		for (auto entity : sphereView)
		{
			auto& sphereComponent = sphereView.get<SphereColliderComponent>(entity);
			sphereComponent.Sphere = CreateRef<Physics::Sphere>(sphereComponent.Radius);
		}

		const auto meshColliderView = m_Scene->m_Registry.view<MeshColliderComponent>();
		for (auto e : meshColliderView)
		{
			Entity entity { e, m_Scene };
			if (!entity.HasComponent<ModelComponent>())
			{
				Logger::Log("Cannot use mesh collider without model component", "physics", WARNING);
			}

			auto meshColliderComponent = meshColliderView.get<MeshColliderComponent>(e);
			const auto& modelComponent = entity.GetComponent<ModelComponent>();

			if (modelComponent.ModelResource)
			{
				uint32_t subMeshId = meshColliderComponent.SubMesh;
				const std::vector<Ref<Mesh>>& submeshes = modelComponent.ModelResource->GetMeshes();
				if (subMeshId >= submeshes.size())
				{
					Logger::Log("Cannot create mesh collider, invalid submesh ID", "physics", WARNING);
				}

				Ref<Mesh> mesh = submeshes[subMeshId];
				meshColliderComponent.Shape = CreateRef<Physics::MeshShape>(mesh);
			}
		}
	}

	void PhysicsSystem::InitializeRigidbodies()
	{
		auto view = m_Scene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : view)
		{
			auto [transform, rigidBodyComponent] = view.get<TransformComponent, RigidBodyComponent>(e);
			Entity ent = Entity({ e, m_Scene });
			Ref<Physics::RigidBody> rigidBody;
			Ref<Physics::PhysicShape> shape;
			if (rigidBodyComponent.GetRigidBody())
			{
				continue;
			}

			if (ent.HasComponent<BoxColliderComponent>())
			{
				BoxColliderComponent& boxComponent = ent.GetComponent<BoxColliderComponent>();
				shape = CreateRef<Physics::Box>(boxComponent.Size);
			}

			if (ent.HasComponent<CapsuleColliderComponent>())
			{
				auto& capsuleComponent = ent.GetComponent<CapsuleColliderComponent>();
				float radius = capsuleComponent.Radius;
				float height = capsuleComponent.Height;
				shape = CreateRef<Physics::Capsule>(radius, height);
			}

			if (ent.HasComponent<CylinderColliderComponent>())
			{
				auto& cylinderComponent = ent.GetComponent<CylinderColliderComponent>();
				float radius = cylinderComponent.Radius;
				float height = cylinderComponent.Height;
				shape = CreateRef<Physics::Cylinder>(radius, height);
			}

			if (ent.HasComponent<SphereColliderComponent>())
			{
				const auto& component = ent.GetComponent<SphereColliderComponent>();
				shape = CreateRef<Physics::Sphere>(component.Radius);
			}

			if (ent.HasComponent<MeshColliderComponent>())
			{
				if (!ent.HasComponent<ModelComponent>())
				{
					Logger::Log("Cannot use mesh collider without model component", "physics", WARNING);
				}

				const auto& modelComponent = ent.GetComponent<ModelComponent>();
				const auto& component = ent.GetComponent<MeshColliderComponent>();

				if (modelComponent.ModelResource)
				{
					uint32_t subMeshId = component.SubMesh;
					const std::vector<Ref<Mesh>>& submeshes = modelComponent.ModelResource->GetMeshes();
					if (subMeshId >= submeshes.size())
					{
						Logger::Log("Cannot create mesh collider, invalid submesh ID", "physics", WARNING);
					}

					Ref<Mesh> mesh = submeshes[subMeshId];
					shape = CreateRef<Physics::MeshShape>(mesh);
				}
			}

			if (!shape)
			{
				continue;
			}

			rigidBody = CreateRef<Physics::RigidBody>(rigidBodyComponent.Mass, transform.GetGlobalPosition(), transform.GetGlobalRotation(), transform.GetGlobalTransform(), shape, ent);
			rigidBody->setLockXAxis(rigidBodyComponent.LockX);
			rigidBody->setLockYAxis(rigidBodyComponent.LockY);
			rigidBody->setLockZAxis(rigidBodyComponent.LockZ);
			PhysicsManager::Get().RegisterBody(rigidBody);
			rigidBodyComponent.Rigidbody = rigidBody;
		}
	}

	void PhysicsSystem::InitializeCharacterControllers()
	{
		auto characterControllerView = m_Scene->m_Registry.view<TransformComponent, CharacterControllerComponent>();
		for (const auto& e : characterControllerView)
		{
			Entity entity = Entity({ e, m_Scene });
			auto [transformComponent, characterControllerComponent] = characterControllerView.get<TransformComponent, CharacterControllerComponent>(e);

			Ref<Physics::PhysicShape> shape;
			if (entity.HasComponent<CapsuleColliderComponent>())
			{
				const auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
				shape = capsuleColliderComponent.Capsule;
			}
			else if (entity.HasComponent<BoxColliderComponent>())
			{
				const auto& capsuleColliderComponent = entity.GetComponent<BoxColliderComponent>();
				shape = capsuleColliderComponent.Box;
			}

			Physics::CharacterControllerSettings settings
			{
				shape,
				characterControllerComponent.Friction,
				characterControllerComponent.MaxSlopeAngle,
				characterControllerComponent.AutoStepping,
				characterControllerComponent.StickToFloorStepDown,
				characterControllerComponent.StepDownExtra,
				characterControllerComponent.SteppingStepUp,
				characterControllerComponent.SteppingMinDistance,
				characterControllerComponent.SteppingForwardDistance
			};

			auto characterController = CreateRef<Physics::CharacterController>(std::move(settings));
			characterController->SetEntity(entity); // Used to link back to the entity
			characterController->Position = transformComponent.GetGlobalPosition();
			characterController->Rotation = transformComponent.GetGlobalRotation();
			characterControllerComponent.SetCharacterController(characterController);
			PhysicsManager::Get().RegisterCharacterController(characterController);
		}
	}

	void PhysicsSystem::ApplyForces()
	{
		auto view = m_Scene->m_Registry.view<TransformComponent, RigidBodyComponent>();
		for (auto e : view)
		{
			auto [transform, rigidBodyComponent] = view.get<TransformComponent, RigidBodyComponent>(e);
			Entity ent = Entity({ e, m_Scene });
			Ref<Physics::RigidBody> rigidBody;

			// Not initialized yet.
			if (!rigidBodyComponent.GetRigidBody() || rigidBodyComponent.QueuedForce == Vector3() || rigidBodyComponent.Mass == 0.0)
			{
				continue;
			}

			PhysicsManager::Get().GetWorld()->AddForceToRigidBody(ent, rigidBodyComponent.QueuedForce);

			rigidBodyComponent.QueuedForce = Vector3();
		}
	}
}
