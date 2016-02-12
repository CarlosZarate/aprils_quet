
#pragma once

#include "Urho3D/Engine/Engine.h"
#include "StateManager.h"
#include "PlayerEntity.h"
#include "EnemyEntity.h"

namespace Urho3D
{
class Node;
class Scene;
class Sprite;
class Text;
}

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;


//-----------------------------------------------------------------------------
// State
//-----------------------------------------------------------------------------
class GamePlayState : public State
{
	// Enable type information.
	URHO3D_OBJECT(GamePlayState, State);


public:

	//-------------------------------------------------------------------------
	// Constructor & Destructor
	//-------------------------------------------------------------------------
	GamePlayState(Context* context);
	~GamePlayState();

	//-------------------------------------------------------------------------
	// Public Virtual Methods
	//-------------------------------------------------------------------------
	// State Activation
	/// Init is called directly after state is registered
	virtual bool	Initialize();
	virtual bool	Begin();
	virtual void	End();

	//-------------------------------------------------------------------------
	// Public Methods
	//-------------------------------------------------------------------------

private:
	//-------------------------------------------------------------------------
	// Private Methods
	//-------------------------------------------------------------------------
	//////////////////////////////////////////////////////////////////////////
	/// Construct the scene content.
    void CreateScene();
	/// Construct UI.
	void CreateUI();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void HandleBeginContact(StringHash eventType, VariantMap& eventData);

    void HandleMouseButtonDownPressed(StringHash eventType, VariantMap& eventData);

    void HandleJoystickButtonDownPressed(StringHash eventType, VariantMap& eventData);

    void HandleJoystickAxisMove(StringHash eventType, VariantMap& eventData);

	SharedPtr<Scene> scene_;

	SharedPtr<Node> cameraNode_;

	SharedPtr<Text> instructionText;

	SharedPtr<Node> targetNode_;

	SharedPtr<Node> nodeWall;

	/// The controllable character component.
    WeakPtr<PlayerEntity> player_;
    WeakPtr<EnemyEntity> enemy_;

    Vector<Node*> RemoveBulletList;

protected:
    Vector2 GetMousePositionXY();
	//-------------------------------------------------------------------------
	// Protected Variables
	//-------------------------------------------------------------------------

};
