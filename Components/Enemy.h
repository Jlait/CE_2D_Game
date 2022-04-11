#pragma once
#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>
#include <CryAISystem/INavigationSystem.h>
#include <CryAISystem/IAISystem.h>
#include <CryAISystem/Components/IEntityNavigationComponent.h>


class CEnemyComponent final : public IEntityComponent
{
public:
	CEnemyComponent() = default;
	virtual ~CEnemyComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<CEnemyComponent>& desc)
	{
		desc.SetGUID("{326819A5-AC21-4DDC-9CF0-EB3B0D2DB07C}"_cry_guid);
		desc.SetEditorCategory("NPC");
		desc.SetLabel("EnemyComponent");
		desc.SetDescription("Controls enemy AI");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	}

	void Initialize() override;

	static CryGUID& IID()
	{
		static CryGUID id = "{326819A5-AC21-4DDC-9CF0-EB3B0D2DB07C}"_cry_guid;
		return id;
	}

	Cry::Entity::EventFlags GetEventMask() const override;
	Vec3 m_pOriginalPosition;

	void ProcessEvent(const SEntityEvent& event) override;

protected:
	IEntityNavigationComponent* navigation;
	Cry::DefaultComponents::CCharacterControllerComponent* m_pCharacter;
	void OnNavigationCompleted();
	void StateUpdate();

private:

};