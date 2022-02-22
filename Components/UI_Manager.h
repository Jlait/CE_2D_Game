#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <FlashUI/FlashUI.h>

struct IUIElement;

class CUIManagerComponent final : public IEntityComponent
{
public:
	CUIManagerComponent() = default;
	virtual ~CUIManagerComponent() = default;
	void ReduceHitpoints(int i); // the amount of hp left
	void ResetUI();

	// Reflect type to set a unique identifier for this component
	// and provide additional information to expose it in the sandbox
	static void ReflectType(Schematyc::CTypeDesc<CUIManagerComponent>& desc)
	{
		desc.SetGUID("{566E9E80-9868-4619-966D-BF033AE3F9AD}"_cry_guid);
		desc.SetEditorCategory("UI");
		desc.SetLabel("UI Manager");
		desc.SetDescription("Manages the ingame UI");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Singleton, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	}

	static CryGUID& IID()
	{
		static CryGUID id = "{566E9E80-9868-4619-966D-BF033AE3F9AD}"_cry_guid;
		return id;
	}

protected:
	virtual void Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;

private:
	void RegisterUIEvents();


	//IUIElement* m_pCrosshairUI;
	//IUIElement* m_pWeaponUI;
//	IUIElement* m_pInteractableUI;
	std::vector<IFlashVariableObject*> m_pHitpointsUI;
	//EntityId entityId = 1500;
};