#include "StdAfx.h"
#include "DamageObject.h"
#include "Player.h"
#include "UI_Manager.h"
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryPhysics/physinterface.h>
#include <CryRenderer/IRenderAuxGeom.h>

class CPlayerComponent;

static void RegisterDamageObjectComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CDamageObjectComponent));
		{

		}
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterDamageObjectComponent)

CDamageObjectComponent::CDamageObjectComponent() :
	m_triggerSize(Vec3(1.0f)),
	m_collidedWithPlayer(false)
{
	
}

void CDamageObjectComponent::Initialize()
{
	m_pEntity->LoadGeometry(0, m_geometry.value);
	if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_material.value, false)) 
	{
		m_pEntity->SetSlotMaterial(0, pMaterial);
	}

	SEntityPhysicalizeParams physicsParams;
	physicsParams.type = PE_STATIC;
	physicsParams.nSlot = -1;

	m_pEntity->Physicalize(physicsParams);

	IEntityTriggerComponent* pTriggerComponent = m_pEntity->CreateComponent<IEntityTriggerComponent>();
	const AABB triggerBounds = AABB(m_triggerSize * -0.5f, m_triggerSize * 0.5f);
	pTriggerComponent->SetTriggerBounds(triggerBounds);
}

Cry::Entity::EventFlags CDamageObjectComponent::GetEventMask() const
{
	return Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::EditorPropertyChanged |
		Cry::Entity::EEvent::EntityEnteredThisArea;
}

void CDamageObjectComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case Cry::Entity::EEvent::EntityEnteredThisArea:
	{
		const EntityId enteredEntityId = static_cast<EntityId>(event.nParam[0]);
		IEntity* entity = gEnv->pEntitySystem->GetEntity(enteredEntityId);
		if (entity && entity->GetComponent<CPlayerComponent>())
		{
			m_collidedWithPlayer = true;
			entity->GetComponent<CPlayerComponent>()->ReduceHp(1);

		}
	}
	break;
	case Cry::Entity::EEvent::EditorPropertyChanged:
	{
		Initialize();
	}
	break;
	/* Falling platform juttu
	case Cry::Entity::EEvent::Update:
	{
		const float deltaTime = event.fParam[0];
		if (m_collidedWithPlayer)
		{
			if (m_timeUntilBreak > 0)
			{
				m_timeUntilBreak -= deltaTime;
			}
		}

		if (m_timeUntilBreak <= 0)
		{
			m_collidedWithPlayer = false;
			Vec3 newPos = m_pEntity->GetPos();
			newPos.z -= 10 * deltaTime;
			m_pEntity->SetPos(newPos);
		}
	}
	break;*/
	default:
		break;
	}
}

#ifndef RELEASE
void CDamageObjectComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext& context) const
{
	if (context.bSelected)
	{
		Matrix34 slotTransform = GetWorldTransformMatrix();

		OBB obb = OBB::CreateOBBfromAABB(Matrix33(IDENTITY), AABB(m_triggerSize * -0.5f, m_triggerSize * 0.5f));
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawOBB(obb, slotTransform, false, context.debugDrawInfo.color, eBBD_Faceted);
	}
}
#endif
