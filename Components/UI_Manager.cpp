#include "StdAfx.h"
#include "UI_Manager.h"
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>

static void RegisterUIComponent(Schematyc::IEnvRegistrar& registrar)
{
    Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
    {
        Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CUIManagerComponent));
        {
        }
    }
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterUIComponent)

void CUIManagerComponent::Initialize()
{

}

Cry::Entity::EventFlags CUIManagerComponent::GetEventMask() const
{
    return
        Cry::Entity::EEvent::GameplayStarted |
        Cry::Entity::EEvent::Reset;
}

void CUIManagerComponent::ProcessEvent(const SEntityEvent& event)
{
    switch (event.event)
    {
    case Cry::Entity::EEvent::GameplayStarted:
    {
        m_pHitpointsUI.push_back(gEnv->pFlashUI->GetUIElement("hitpointsGFx")->GetMovieClip("Hp_0"));
        m_pHitpointsUI.push_back(gEnv->pFlashUI->GetUIElement("hitpointsGFx")->GetMovieClip("Hp_1"));
        m_pHitpointsUI.push_back(gEnv->pFlashUI->GetUIElement("hitpointsGFx")->GetMovieClip("Hp_2"));

    }
    break;
    case Cry::Entity::EEvent::Reset:
    {

    }
    break;
    }
}

void CUIManagerComponent::ReduceHitpoints(int hp)
{
    for (int j = hp; j > hp - 1; j--) {
        m_pHitpointsUI[j]->SetVisible(false);
    }
}

void CUIManagerComponent::ResetUI() {
    for (int j = 0; j < m_pHitpointsUI.size(); j++) {
        m_pHitpointsUI[j]->SetVisible(true);
    }
}

/*
void CUIComponent::RegisterUIEvents(CCharacterComponent* pCharacter)
{
    pCharacter->m_equipEvent.RegisterListener([this](const string weaponName, const string fireMode, const int clipCount, const int clipCapacity, const int totalAmmo)
        {
            SUIArguments uiArgs;
            uiArgs.AddArgument(weaponName);
            uiArgs.AddArgument(fireMode);
            uiArgs.AddArgument(clipCount);
            uiArgs.AddArgument(clipCapacity);
            uiArgs.AddArgument(totalAmmo);
            m_pWeaponUI->CallFunction("SetupWeapon", uiArgs);
            m_pWeaponUI->SetVisible(true);
        });

    pCharacter->m_wepFiredEvent.RegisterListener([this](const int clipCount)
        {
            m_pWeaponUI->CallFunction("Fire", SUIArguments::Create(clipCount));
            m_pCrosshairUI->CallFunction("Fire");
        });

    pCharacter->m_reloadEvent.RegisterListener([this](const int clipCount)
        {
            m_pWeaponUI->CallFunction("Reload", SUIArguments::Create(clipCount));
        });

    pCharacter->m_switchFireModeEvent.RegisterListener([this](const string fireMode)
        {
            m_pWeaponUI->CallFunction("SetFireMode", SUIArguments::Create(fireMode));
        });

    pCharacter->m_ammoChangedEvent.RegisterListener([this](const int totalAmmo)
        {
            m_pWeaponUI->CallFunction("SetTotalAmmo", SUIArguments::Create(totalAmmo));
        });

    m_pPlayer->m_interactEvent.RegisterListener([this](const SObjectData& objData, const bool isShowing)
        {
            if (isShowing)
            {
                string combinedString = objData.objectKeyword + " " + objData.objectName + " " + objData.objectBonus;
                m_pInteractableUI->CallFunction("UpdateText", SUIArguments::Create(combinedString));
            }
            m_pInteractableUI->SetVisible(isShowing);
        });
}*/
