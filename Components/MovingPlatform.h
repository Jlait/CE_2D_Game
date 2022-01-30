#pragma once

#include "CryMath/Cry_Math.h"
#include <CryEntitySystem/IEntitySystem.h>
#include <CryEntitySystem/IEntityComponent.h>

class CMovingPlatformComponent final : public IEntityComponent
{
public:
	CMovingPlatformComponent() = default;
	virtual ~CMovingPlatformComponent() = default;

	Vec3 startPos;

	// Reflect type to set a unique identifier for this component
	static void ReflectType(Schematyc::CTypeDesc<CMovingPlatformComponent>& desc)
	{
		desc.SetGUID("{4B6950D9-1FEA-430A-9EF6-0BCBE8383852}"_cry_guid);
		desc.SetEditorCategory("Game");
		desc.SetLabel("MovingPlatform");
		desc.SetDescription("Lerping platform");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	}

	static CryGUID& IID()
	{
		static CryGUID id = "{4B6950D9-1FEA-430A-9EF6-0BCBE8383852}"_cry_guid;
		return id;
	}

	virtual Cry::Entity::EventFlags GetEventMask() const override
	{
		return ENTITY_EVENT_COLLISION |
			Cry::Entity::EEvent::LevelStarted |
			Cry::Entity::EEvent::Update;
	}
	virtual void ProcessEvent(const SEntityEvent& event) override
	{
		switch (event.event)
		{
			case Cry::Entity::EEvent::LevelStarted:
			{
				//const Matrix34 startTransform = m_pEntity->GetWorldTM();
				//m_pEntity->IsInitialized
				//Vec3 startPos = m_pEntity->GetWorldPos();

				//const Matrix34 startTransform = m_pEntity->GetWorldTM();
				CryLog("platformi HÖÖK");
			}
			break;
				// Handle the OnCollision event, in order to have the entity removed on collision
			case Cry::Entity::EEvent::Update:
			{
			
			if (m_pEntity->IsInitialized() && firstUpdate) {
					startPos = m_pEntity->GetWorldPos();
					firstUpdate = false;
			}
				const float frameTime = event.fParam[0];

				// Update the fireball's transformation
				Vec3 velocity = ZERO;
				velocity.z = 5.f;
				Matrix34 transformation = m_pEntity->GetWorldTM();


				if (!reverse) {
					lerp = Lerp(transformation.GetTranslation(),
						startPos + velocity, frameTime / speed);

				}
				else {
					lerp = Lerp(transformation.GetTranslation(),
						startPos, frameTime / speed);
				}
				
				transformation.SetTranslation(lerp);

				if ((startPos.z + velocity.z) - transformation.GetTranslation().z <= lerpSnap)
				{
					reverse = true;
				}

				else if (startPos.z - transformation.GetTranslation().z >= -lerpSnap) 
				{
					reverse = false;
				}

				pe_action_move move;
				move.iJump = 1;
				//// Apply set position and rotation to the entity
				m_pEntity->SetWorldTM(transformation);
				if (!reverse) {
					move.dir = Vec3(0, 0, 1.f);
				}
				else {
					move.dir = Vec3(0, 0, -1.f);
				}

				m_pEntity->GetPhysics()->Action(&move);


			}
			break;
		}
	}
protected:
	const Matrix34 startTransform;
	bool firstUpdate = true;
	bool reverse = false;
	Vec3 lerp;
	const float lerpSnap = 1.f;
	const float speed = 2.f;
};