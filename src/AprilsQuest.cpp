
#include "AprilsQuest.h"
#include "SplashState.h"
#include "MainMenuState.h"
#include "GamePlayState.h"

#include "Urho3D/DebugNew.h"

// Number of static sprites to draw
static const StringHash VAR_MOVESPEED("MoveSpeed");
static const StringHash VAR_ROTATESPEED("RotateSpeed");

URHO3D_DEFINE_APPLICATION_MAIN(AprilsQuest)

AprilsQuest::AprilsQuest(Context* context) :
    BaseApplication(context)
{
}

void AprilsQuest::Start()
{
    stateManager_ = new StateManager(context_);
    // Execute base class startup
    BaseApplication::Start();

    // Main Menu
	State * mainMenuState = new MenuState(context_);
	mainMenuState->SetStateId("MainMenu");
	stateManager_->RegisterState(mainMenuState);

	// Game State
	State * gameplayState = new GamePlayState(context_);
	gameplayState->SetStateId("GamePlayState");
	stateManager_->RegisterState(gameplayState);

    SplashState * splashState = new SplashState(context_);
	splashState->SetStateId("SplashState");
	stateManager_->RegisterState(splashState);

	splashState->SetNextState("MainMenu");

	stateManager_->PushToStack("GamePlayState");
}
