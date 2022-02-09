#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include "CryString/CryString.h"
////////////////////////////////////////////////////////
// Physicalized bullet shot from weaponry, expires on collision with another object
////////////////////////////////////////////////////////
class CFireballComponent final : public IEntityComponent
{
public:
	virtual ~CFireballComponent() {}

	// IEntityComponent
	virtual void Initialize() override
	{
		// Set the model
		const int geometrySlot = 0;
		m_pEntity->LoadGeometry(geometrySlot, "%ENGINE%/EngineAssets/Objects/primitive_sphere.cgf");

		// Load the custom bullet material.
		// This material has the 'mat_bullet' surface type applied, which is set up to play sounds on collision with 'mat_default' objects in Libs/MaterialEffects
		auto* pBulletMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("materials/fireball");
		m_pEntity->SetMaterial(pBulletMaterial);


	}

	// Reflect type to set a unique identifier for this component
	static void ReflectType(Schematyc::CTypeDesc<CFireballComponent>& desc)
	{
		desc.SetGUID("{B53A9A5F-F27A-42CB-82C7-B1E379C41A2A}"_cry_guid);
	}

	virtual Cry::Entity::EventFlags GetEventMask() const override 
	{ 
		return ENTITY_EVENT_COLLISION |
		Cry::Entity::EEvent::Update;
	}
	virtual void ProcessEvent(const SEntityEvent& event) override
	{
		switch (event.event)
		{
			// Handle the OnCollision event, in order to have the entity removed on collision
			case Cry::Entity::EEvent::Update:
			{
				if (event.event == ENTITY_EVENT_COLLISION)
				{
					// Collision info can be retrieved using the event pointer
					//EventPhysCollision *physCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);

					// Queue removal of this entity, unless it has already been done
					gEnv->pEntitySystem->RemoveEntity(GetEntityId());
				}
			
			// Update the fireball's transformation
			Vec3 velocity = ZERO;
			velocity.y = -0.05f;
			Matrix34 transformation = m_pEntity->GetWorldTM();
			transformation.AddTranslation(transformation.TransformVector(velocity));

			//// Apply set position and rotation to the entity
			m_pEntity->SetWorldTM(transformation);
			}	
		}
	}

	 //~IEntityComponent
};
