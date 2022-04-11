#include "StdAfx.h"
#include "Enemy.h"
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>


static void RegisterEnemyComponent(Schematyc::IEnvRegistrar& registrar)
{
    Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
    {
        Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEnemyComponent));
        {
        }
    }
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterEnemyComponent)

void CEnemyComponent::Initialize()
{
    // Instantiate and set up navigation component
    navigation = GetEntity()->GetOrCreateComponent<IEntityNavigationComponent>();
    IEntityNavigationComponent::SMovementProperties movementProperties;
    movementProperties.normalSpeed = 1.8f;
    movementProperties.maxSpeed = 2.0f;
    movementProperties.bStopAtEnd = true;
    navigation->SetMovementProperties(movementProperties);
    navigation->SetNavigationAgentType("MediumSizedCharacters");
}

void CEnemyComponent::OnNavigationCompleted()
{
    navigation->SetNavigationCompletedCallback([this](const bool reached)
        {
            if (reached)
            {
                navigation->NavigateTo(m_pOriginalPosition);
            }
        });
}

void CEnemyComponent::StateUpdate() 
{
    navigation->SetStateUpdatedCallback([this](const Vec3& velocity) {
        // Updates our character velocity
        m_pCharacter->SetVelocity(velocity);

        // Sets our rotation in the direction we are moving
        m_pEntity->SetRotation(Quat::CreateRotationVDir(velocity));
        });
}


Cry::Entity::EventFlags CEnemyComponent::GetEventMask() const
{
    return
        Cry::Entity::EEvent::GameplayStarted |
        Cry::Entity::EEvent::Reset;
}


void CEnemyComponent::ProcessEvent(const SEntityEvent& event)
{
    switch (event.event)
    {
    case Cry::Entity::EEvent::GameplayStarted:
    {
        m_pOriginalPosition = m_pEntity->GetWorldTM().GetTranslation();
    }
    break;
    case Cry::Entity::EEvent::Reset:
    {

    }
    break;
    }
}

