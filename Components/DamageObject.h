// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <CrySchematyc/ResourceTypes.h>

////////////////////////////////////////////////////////
// Spawn point
////////////////////////////////////////////////////////
class CDamageObjectComponent final : public IEntityComponent
#ifndef RELEASE
	, public IEntityComponentPreviewer
#endif
{
public:
	CDamageObjectComponent();
	virtual ~CDamageObjectComponent() = default;

	// Reflect type to set a unique identifier for this component
	static void ReflectType(Schematyc::CTypeDesc<CDamageObjectComponent>& desc)
	{
		desc.SetGUID("{8F4B0051-B39D-4ECE-9D7E-5652EF12BC56}"_cry_guid);
		desc.SetEditorCategory("Game");
		desc.SetLabel("DamageObject");
		desc.SetDescription("Damages on collision");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

		desc.AddMember(&CDamageObjectComponent::m_geometry, 'geom', "Geometry", "Geometry", "Object geometry", "");
		desc.AddMember(&CDamageObjectComponent::m_triggerSize, 'trig', "TriggerSize", "Trigger Size", "Size of the damage trigger", Vec3(1.0f));
		desc.AddMember(&CDamageObjectComponent::m_material, 'mat', "Material", "Material", "Material for object", "");
	}

	void Initialize() override;

	static CryGUID& IID()
	{
		static CryGUID id = "{4B6950D9-1FEA-430A-9EF6-0BCBE8383852}"_cry_guid;
		return id;
	}

	Cry::Entity::EventFlags GetEventMask() const override;

	void ProcessEvent(const SEntityEvent& event) override;


#ifndef RELEASE
	virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }

	virtual void SerializeProperties(Serialization::IArchive& archive) final {}

	virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext& context) const final;
#endif

private:
	Schematyc::GeomFileName m_geometry;
	Schematyc::MaterialFileName m_material;
	bool m_collidedWithPlayer;
	Vec3 m_triggerSize;

};
