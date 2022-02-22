#include "StdAfx.h"
#include "Player.h"
#include "Fireball.h"
#include "SpawnPoint.h"
#include "GamePlugin.h"

#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CryCore/StaticInstanceList.h>
#include <CryNetwork/Rmi.h>

namespace
{
	static void RegisterPlayerComponent(Schematyc::IEnvRegistrar& registrar)
	{
		Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
		{
			Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPlayerComponent));
		}
	}

	CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPlayerComponent);
}

inline const char* const BoolToString(bool b)
{
	return b ? "true" : "false";
}


void CPlayerComponent::Initialize()
{
	lives = maxLives;
	hitPoints = maxHitPoints;
	q = IDENTITY;
	
	// The character controller is responsible for maintaining player physics
	m_pCharacterController = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	m_pCharacterController->GetPhysicsParameters().m_mass = 1.f;
	m_pCharacterController->GetPhysicsParameters().m_bCapsule = 0;
	// Offset the default character controller up by one unit
	m_pCharacterController->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, 0.5f)));
	m_pAnimationComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CAdvancedAnimationComponent>();

	// Set the player geometry, this also triggers physics proxy creation
	m_pAnimationComponent->SetMannequinAnimationDatabaseFile("Animations/Mannequin/ADB/thirdPerson.adb");
	m_pAnimationComponent->SetCharacterFile("characters/wizard/magic_man.cdf");
	m_pAnimationComponent->SetControllerDefinitionFile("Animations/Mannequin/ADB/FirstPersonControllerDefinition.xml");
	m_pAnimationComponent->SetDefaultScopeContextName("ThirdPersonCharacter");
	// Queue the idle fragment to start playing immediately on next update
	m_pAnimationComponent->SetDefaultFragmentName("Idle");
	// 
	// Disable movement coming from the animation (root joint offset), we control this entirely via physics
	m_pAnimationComponent->SetAnimationDrivenMotion(true);

	// Load the character and Mannequin data from file
	m_pAnimationComponent->LoadFromDisk();
	m_pAnimationComponent->ResetCharacter();

	// Acquire fragment and tag identifiers to avoid doing so each update

	m_idleFragmentId = m_pAnimationComponent->GetFragmentId("Idle");
	m_jumpingFragmentId = m_pAnimationComponent->GetFragmentId("Jump");
	m_fallingFragmentId = m_pAnimationComponent->GetFragmentId("Falling");
	m_walkFragmentId = m_pAnimationComponent->GetFragmentId("Walk");

	m_idleTagId = m_pAnimationComponent->GetTagId("Idle");
	m_walkTagId = m_pAnimationComponent->GetTagId("Walk");
	m_jumpTagId = m_pAnimationComponent->GetTagId("Jump");
	m_fallingTagId = m_pAnimationComponent->GetTagId("Falling");

	
	m_wasOnGroundLastFrame = false;
	m_wasWalkingLastFrame = false;
	m_wasFallingDown = false;
	t_timeSinceLeftGround = 0.f;

	// Mark the entity to be replicated over the network
	m_pEntity->GetNetEntity()->BindToNetwork();
	
	// Register the RemoteReviveOnClient function as a Remote Method Invocation (RMI) that can be executed by the server on clients
	SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::Register(this, eRAT_NoAttach, false, eNRT_ReliableOrdered);
}

void CPlayerComponent::InitializeLocalPlayer()
{
	// Create the camera component, will automatically update the viewport every frame
	m_pCameraComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	
	Matrix34 transformation = m_pEntity->GetWorldTM();
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(transformation));

	ypr.x = m_pEntity->GetWorldRotation().v.x + DEG2RAD(-90.f);
	ypr.y = 0;

	// Disable roll
	ypr.z = 0;

	Quat q = Quat(CCamera::CreateOrientationYPR(ypr));

	m_pCameraComponent->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(1.0f, -4.0f, 1.f)));
	// Reset the mouse delta since we "used" it this frame
	m_mouseDeltaRotation = ZERO;

	//m_pMeshComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CStaticMeshComponent>();

	// Create the audio listener component.
	m_pAudioListenerComponent = m_pEntity->GetOrCreateComponent<Cry::Audio::DefaultComponents::CListenerComponent>();

	// Get the input component, wraps access to action mapping so we can easily get callbacks when inputs are triggered
	m_pInputComponent = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();
	
	// Register an action, and the callback that will be sent when it's triggered
	m_pInputComponent->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveLeft, (EActionActivationMode)activationMode);  }); 
	// Bind the 'A' key the "moveleft" action
	m_pInputComponent->BindAction("player", "moveleft", eAID_KeyboardMouse, EKeyId::eKI_A);

	m_pInputComponent->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveRight, (EActionActivationMode)activationMode);  }); 
	m_pInputComponent->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

	m_pInputComponent->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveForward, (EActionActivationMode)activationMode);  }); 
	m_pInputComponent->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

	m_pInputComponent->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveBack, (EActionActivationMode)activationMode);  }); 
	m_pInputComponent->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

	m_pInputComponent->RegisterAction("player", "jump", [this](int activationMode, float value) {HandleInputFlagChange(EInputFlag::Jump, (EActionActivationMode)activationMode); });
	//m_pInputComponent->BindAction("player", "jump", eAID_KeyboardMouse, EKeyId::eKI_Space);
	m_pInputComponent->BindAction("player", "jump", eAID_KeyboardMouse, EKeyId::eKI_Space, true, false, false);

	// Register the shoot action
	m_pInputComponent->RegisterAction("player", "shoot", [this](int activationMode, float value)
		{
			// Only fire on press, not release
			if (activationMode == eAAM_OnPress)
			{
				if (ICharacterInstance* pCharacter = m_pAnimationComponent->GetCharacter())
				{
					IAttachment* pBarrelOutAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName("tongue");

					if (pBarrelOutAttachment != nullptr)
					{
						QuatTS bulletOrigin = pBarrelOutAttachment->GetAttWorldAbsolute();

						SEntitySpawnParams spawnParams;
						spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();

						spawnParams.vPosition = bulletOrigin.t;
						spawnParams.qRotation = bulletOrigin.q;

						const float bulletScale = 2.f;
						spawnParams.vScale = Vec3(bulletScale);
						// Spawn the entity
						if (IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams))
						{
							pEntity->CreateComponentClass<CFireballComponent>();
							CryLog("fireball3");
						}
					}
				}
			}
		});

	// Bind the shoot action to left mouse click
	m_pInputComponent->BindAction("player", "shoot", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);

	if (gEnv->pEntitySystem->GetEntity(16)->GetComponent<CUIManagerComponent>()) {
		uiManager = gEnv->pEntitySystem->GetEntity(16)->GetComponent<CUIManagerComponent>();
	}
		//uiManager = dynamic_cast<CUIManagerComponent*> (IEntityComponent);
}

void CPlayerComponent::UpdateMovementRequest(float frameTime) {
	Vec3 velocity = ZERO;

	float moveSpeed = 30.5f;
	float airMoveSpeed = 8.5f;

	if (m_pCharacterController->IsOnGround()) {
		jumping = false;
	}
	else jumping = true;

	if (m_pCharacterController->IsOnGround() && m_inputFlags & EInputFlag::MoveLeft)
	{
		velocity.x -= moveSpeed * frameTime;
		RotateCharacter(90.f, m_pEntity->GetWorldTM());
	}

	else if (!m_pCharacterController->IsOnGround() && m_inputFlags & EInputFlag::MoveLeft)
	{
		velocity.x -= airMoveSpeed * frameTime;
		RotateCharacter(90.f, m_pEntity->GetWorldTM());
	}

	if (m_pCharacterController->IsOnGround() && m_inputFlags & EInputFlag::MoveRight)
	{
		velocity.x += moveSpeed * frameTime;
		RotateCharacter(-90.f, m_pEntity->GetWorldTM());
	}

	else if (!m_pCharacterController->IsOnGround() && m_inputFlags & EInputFlag::MoveRight)
	{
		velocity.x += airMoveSpeed * frameTime;
		RotateCharacter(-90.f, m_pEntity->GetWorldTM());
	}

	if (m_pCharacterController->IsOnGround() && m_inputFlags & EInputFlag::Jump && !jumping) 
	{
		Vec3 jumpVelocity = Vec3(0, 0, 8.f);
		m_pCharacterController->ChangeVelocity(jumpVelocity, Cry::DefaultComponents::CCharacterControllerComponent::EChangeVelocityMode::Add);
		HandleInputFlagChange(EInputFlag::Jump, eAAM_OnRelease);
	}

	m_pCharacterController->ChangeVelocity(velocity, Cry::DefaultComponents::CCharacterControllerComponent::EChangeVelocityMode::Add);
}

void CPlayerComponent::UpdateAnimation(float frameTime)
{
	if (m_walkAction && (m_walkAction->GetStatus() < IAction::Exiting) && !m_pCharacterController->IsWalking())
	{
		m_walkAction->Stop();
	}

	if (m_jumpAction && (m_jumpAction->GetStatus() < IAction::Exiting)) {
		if (m_pCharacterController->IsOnGround()) 
		{
			m_jumpAction->Stop();
		}
		else if (m_wasFallingDown && m_pCharacterController->GetVelocity().z <= 0)
		{
			//CryLog("Falling");
			m_jumpAction->Stop();
			m_jumpAction = new SSimpleAction(30U, m_fallingFragmentId);
			m_pAnimationComponent->SetTagWithId(m_fallingTagId, true);
			m_pAnimationComponent->QueueCustomFragment(*m_jumpAction);
		}
	}

	// Falling
	/*
	if (m_fallingAction && (m_fallingAction->GetStatus() < IAction::Exiting) && m_wasFallingDown && m_pCharacterController->GetVelocity().z <= 0 && !m_pCharacterController->IsOnGround())
	{
		CryLog("falling");
		m_fallingAction->Stop();
		m_fallingAction = new SSimpleAction(30U, m_fallingFragmentId);
		m_pAnimationComponent->SetTagWithId(m_fallingTagId, true);
		m_pAnimationComponent->QueueCustomFragment(*m_fallingAction);
	}*/

	if (!m_idleAction)
	{
		m_pAnimationComponent->SetTagWithId(m_idleTagId, true);
		m_idleAction = new SSimpleAction(0U, m_idleFragmentId, TAG_STATE_EMPTY, IAction::Interruptable);
		m_pAnimationComponent->QueueCustomFragment(*m_idleAction);

		if (!m_pCharacterController->IsOnGround()) 
		{
			m_pAnimationComponent->SetTagWithId(m_jumpTagId, true);
			m_jumpAction = new SSimpleAction(30U, m_jumpingFragmentId);
			m_pAnimationComponent->QueueCustomFragment(*m_jumpAction);
		}
	}
	
	// Jump
	if (m_wasOnGroundLastFrame && !m_pCharacterController->IsOnGround())
	{
		CryLog("Jump");
		m_pAnimationComponent->QueueFragmentWithId(m_jumpingFragmentId);
		m_jumpAction = new SSimpleAction(45U, m_jumpingFragmentId);
		m_pAnimationComponent->QueueCustomFragment(*m_jumpAction);
	}

	else if (!m_wasWalkingLastFrame && m_pCharacterController->IsWalking() && m_pCharacterController->IsOnGround())
	{
		m_walkAction = new SSimpleAction(20U, m_walkFragmentId);
		m_pAnimationComponent->SetTagWithId(m_walkTagId, true);
		m_pAnimationComponent->QueueCustomFragment(*m_walkAction);
	}

	// Landing
	if (!m_wasOnGroundLastFrame && m_pCharacterController->IsOnGround() && t_timeSinceLeftGround >= 0.2f)
	{
		CryLog("Land");
		m_jumpAction->Stop();
		m_jumpLandAction = new SSimpleAction(40U, m_idleFragmentId);
		m_pAnimationComponent->QueueCustomFragment(*m_jumpLandAction);
	}
	
	// Update the mannequin tags
	/*
	if (m_pCharacterController->IsOnGround()) {
		m_pAnimationComponent->SetTagWithId(m_idleTagId, true);
	}
	else m_pAnimationComponent->SetTagWithId(m_idleTagId, false);*/

}

Cry::Entity::EventFlags CPlayerComponent::GetEventMask() const
{
	return
		Cry::Entity::EEvent::BecomeLocalPlayer |
		Cry::Entity::EEvent::GameplayStarted |
		Cry::Entity::EEvent::Update |
		Cry::Entity::EEvent::Reset;
}

void CPlayerComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case Cry::Entity::EEvent::BecomeLocalPlayer:
	{
		InitializeLocalPlayer();
	}
	break;
	case Cry::Entity::EEvent::GameplayStarted: 
	{
		const Matrix34 newTransform = CSpawnPointComponent::GetFirstSpawnPointTransform();
		Revive(newTransform);
	}

	case Cry::Entity::EEvent::Update:
	{
		// Don't update the player if we haven't spawned yet
		if(!m_isAlive)
			return;
		
		const float frameTime = event.fParam[0];

		const float moveSpeed = 20.5f;
		Vec3 velocity = ZERO;

		UpdateMovementRequest(frameTime);

		UpdateAnimation(frameTime);
		m_wasOnGroundLastFrame = m_pCharacterController->IsOnGround();
		m_wasWalkingLastFrame = m_pCharacterController->IsWalking();
		m_wasFallingDown = m_pCharacterController->GetVelocity().z < 0;
		if (m_pCharacterController->IsOnGround())
		{
			t_timeSinceLeftGround = 0.0f;
		}
		else 
		{
			t_timeSinceLeftGround += frameTime;
		}
	}
	break;
	case Cry::Entity::EEvent::Reset:
	{
		
		// Disable player when leaving game mode.
		m_isAlive = event.nParam[0] != 0;

		if (m_isAlive) 
		{
			ResetAnimations();
			m_wasOnGroundLastFrame = false;
			m_wasWalkingLastFrame = false;
			m_wasFallingDown = false;
			t_timeSinceLeftGround = 0.0;
		}
	}
	break;
	}
}

bool CPlayerComponent::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == InputAspect)
	{
		ser.BeginGroup("PlayerInput");

		const CEnumFlags<EInputFlag> prevInputFlags = m_inputFlags;

		ser.Value("m_inputFlags", m_inputFlags.UnderlyingValue(), 'ui8');

		if (ser.IsReading())
		{
			const CEnumFlags<EInputFlag> changedKeys = prevInputFlags ^ m_inputFlags;

			const CEnumFlags<EInputFlag> pressedKeys = changedKeys & prevInputFlags;
			if (!pressedKeys.IsEmpty())
			{
				HandleInputFlagChange(pressedKeys, eAAM_OnPress);
			}

			const CEnumFlags<EInputFlag> releasedKeys = changedKeys & prevInputFlags;
			if (!releasedKeys.IsEmpty())
			{
				HandleInputFlagChange(pressedKeys, eAAM_OnRelease);
			}
		}

		ser.EndGroup();
	}
	
	return true;
}

void CPlayerComponent::OnReadyForGameplayOnServer()
{
	CRY_ASSERT(gEnv->bServer, "This function should only be called on the server!");

	const Matrix34 newTransform = CSpawnPointComponent::GetFirstSpawnPointTransform();

	Revive(newTransform);
	
	// Invoke the RemoteReviveOnClient function on all remote clients, to ensure that Revive is called across the network
	SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::InvokeOnOtherClients(this, RemoteReviveParams{ newTransform.GetTranslation(), Quat(newTransform)});
	
	// Go through all other players, and send the RemoteReviveOnClient on their instances to the new player that is ready for gameplay
	const int channelId = m_pEntity->GetNetEntity()->GetChannelId();
	CGamePlugin::GetInstance()->IterateOverPlayers([this, channelId](CPlayerComponent& player)
	{
		// Don't send the event for the player itself (handled in the RemoteReviveOnClient event above sent to all clients)
		if (player.GetEntityId() == GetEntityId())
			return;

		// Only send the Revive event to players that have already respawned on the server
		if (!player.m_isAlive)
			return;

		// Revive this player on the new player's machine, on the location the existing player was currently at
		const QuatT currentOrientation = QuatT(player.GetEntity()->GetWorldTM());
		SRmi<RMI_WRAP(&CPlayerComponent::RemoteReviveOnClient)>::InvokeOnClient(&player, RemoteReviveParams{ currentOrientation.t, currentOrientation.q }, channelId);
	});
}

void CPlayerComponent::ReduceHp(int dmg)
{
	hitPoints -= dmg;
	uiManager->ReduceHitpoints(hitPoints);
	if (hitPoints == 0)
	{
		ReduceLives();
	}
	CryLog("Hitpoints: " + hitPoints);
}

void CPlayerComponent::ReduceLives()
{
	lives -= 1;
	hitPoints = 3;
	CryLog("Lives: ");
	if (lives == 0) {
		GameOver();
	}
}

void CPlayerComponent::GameOver()
{
	CryLog("Game Over");
	Revive(CSpawnPointComponent::GetFirstSpawnPointTransform());
	lives = maxLives;
	hitPoints = maxHitPoints;
	uiManager->ResetUI();
}

bool CPlayerComponent::RemoteReviveOnClient(RemoteReviveParams&& params, INetChannel* pNetChannel)
{
	// Call the Revive function on this client
	Revive(Matrix34::Create(Vec3(1.f), params.rotation, params.position));

	return true;
}

void CPlayerComponent::Revive(const Matrix34& transform)
{
	m_isAlive = true;
	
	// Set the entity transformation, except if we are in the editor
	// In the editor case we always prefer to spawn where the viewport is
	/*if(!gEnv->IsEditor())
	{*/
		m_pEntity->SetWorldTM(transform);
	//}
	
	// Reset input now that the player respawned
	m_inputFlags.Clear();
	NetMarkAspectsDirty(InputAspect);
	
	m_mouseDeltaRotation = ZERO;
	m_activeFragmentId = FRAGMENT_ID_INVALID;

}

void CPlayerComponent::HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type)
{
	switch (type)
	{
	case EInputFlagType::Hold:
	{
		if (activationMode == eAAM_OnRelease)
		{
			m_inputFlags &= ~flags;
		}
		else
		{
			m_inputFlags |= flags;
		}

		/*
		if (activationMode == eAAM_OnPress) {

		}*/
	}
	break;
	case EInputFlagType::Toggle:
	{
		if (activationMode == eAAM_OnRelease)
		{
			// Toggle the bit(s)
			m_inputFlags ^= flags;
		}
	}
	break;
	}
	
	if(IsLocalClient())
	{
		NetMarkAspectsDirty(InputAspect);
	}
}

void CPlayerComponent::RotateCharacter(float angle, Matrix34 transformation) {
	Ang3 ypr = CCamera::CreateAnglesYPR(Matrix33(transformation));

	ypr.x = m_pEntity->GetWorldRotation().v.x + DEG2RAD(angle);
	ypr.y = 0;

	// Disable roll
	ypr.z = 0;

	Quat q = Quat(CCamera::CreateOrientationYPR(ypr));
	m_pAnimationComponent->SetTransformMatrix(Matrix34::Create(Vec3(1.f), q, Vec3(0, 0, 0)));
}

