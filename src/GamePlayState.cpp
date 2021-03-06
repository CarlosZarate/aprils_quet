#include "Urho3D/Urho2D/AnimatedSprite2D.h"
#include "Urho3D/Urho2D/AnimationSet2D.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/UI/Font.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Urho2D/Sprite2D.h"
#include "Urho3D/UI/Text.h"
#include "Urho3D/UI/UI.h"
#include "iostream"
#include "Urho3D/Urho2D/RigidBody2D.h"
#include "Urho3D/Urho2D/PhysicsWorld2D.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/Urho2D/CollisionCircle2D.h"
#include "Urho3D/Urho2D/PhysicsEvents2D.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/LuaScript/LuaFile.h"
#include "Urho3D/LuaScript/LuaFunction.h"
#include "Urho3D/LuaScript/LuaScript.h"
#include "Urho3D/LuaScript/LuaScriptInstance.h"
#include "Urho3D/IO/FileSystem.h"
#include "BulletEntity.h"
#include "AStarFinder.h"
#include "MapGenerator.h"
#include "Character/Metadata.h"
#include "GamePlayState.h"

// Number of static sprites to draw
static const StringHash VAR_MOVESPEED("MoveSpeed");
static const StringHash VAR_ROTATESPEED("RotateSpeed");

GamePlayState::GamePlayState(Context* context) : State(context)
{
    PlayerEntity::RegisterObject(context);
	BulletEntity::RegisterObject(context);
	EnemyEntity::RegisterObject(context);
	AStarFinder::RegisterObject(context);
    MapGenerator::RegisterObject(context);
    Metadata::RegisterObject(context);
	context_->RegisterSubsystem(new LuaScript(context_));
}

GamePlayState::~GamePlayState()
{
}

bool GamePlayState::Begin()
{
	// Create the UI content
	CreateUI();
	// Create the scene content
	CreateScene();
	// Setup the viewport for displaying the scene
	SetupViewport();
	// Hook up to the frame update events
	SubscribeToEvents();

	GetSubsystem<Input>()->SetMouseVisible(true);

	return State::Begin();
}

bool GamePlayState::Initialize()
{
	return State::Initialize();
}

void GamePlayState::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    PhysicsWorld2D* physicsWorld = scene_->CreateComponent<PhysicsWorld2D>();
    physicsWorld->SetGravity(Vector2::ZERO);

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(7.5f, 7.5, -10.0f));

    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    Graphics* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    MapGenerator* mapGenerator = scene_->CreateComponent<MapGenerator>();
    mapGenerator->initMap(40, 50, 3, 4);

    nodeWall = scene_->CreateChild("NodoWall");

    /*XMLFile* nodoXMLFile = cache->GetResource<XMLFile>("Scenes/nodo_map.xml");
    XMLElement nodoXML(nodoXMLFile->GetRoot());
    nodeWall->LoadXML(nodoXML);*/

    JSONFile* data = new JSONFile(context_);
    File file(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/MapNode.json");
    data->Load(file);

    JSONValue rootjson = data->GetRoot();

    JSONValue blocks = rootjson.Get("blocks");
    JSONValue playerJsonPosition = rootjson.Get("playerpost");
    String stringPosition = playerJsonPosition.GetString();
    //std::cout<< stringPosition <<std::endl;
    Vector2 position(5,7);//rootjson.GetObject("playerpost");

    AStarFinder* finder = scene_->CreateComponent<AStarFinder>();
    finder->LoadMap(mapGenerator->getNodeGrid(), 40, 50, 0.5f);
    
    // Create 2D physics world component

    targetNode_ = scene_->CreateChild("Target");
    Sprite2D* targetsprite = cache->GetResource<Sprite2D>("Urho2D/target.png");
    if (!targetsprite)
        return;

    StaticSprite2D* targetstaticsprite = targetNode_->CreateComponent<StaticSprite2D>();
    targetstaticsprite->SetSprite(targetsprite);
    targetstaticsprite->SetOrderInLayer(65000);

    // Get animation set
    SharedPtr<Node> spriteNode(scene_->CreateChild("Player"));
    spriteNode->SetPosition2D(position);
    player_ = spriteNode->CreateComponent<PlayerEntity>();

    /*LuaFile* scriptFile = cache->GetResource<LuaFile>("Scripts/player.lua");
    if (!scriptFile)
        return;
    LuaScriptInstance* instance = spriteNode->CreateComponent<LuaScriptInstance>();
    instance->CreateObject(scriptFile, "Rotator");*/

    SharedPtr<Node> enemynode(scene_->CreateChild("EnemyNode"));
    enemynode->SetPosition(Vector3(5.0f, 6.0f, 0.0f));
    enemy_ = enemynode->CreateComponent<EnemyEntity>();

    /*SharedPtr<Node> enemynode2(scene_->CreateChild("EnemyNode"));
    enemynode2->SetPosition(Vector3(7.5f, 5.0f, 0.0f));
    enemynode2->CreateComponent<EnemyEntity>();

    SharedPtr<Node> enemynode3(scene_->CreateChild("EnemyNode"));
    enemynode3->SetPosition(Vector3(10.0f, 7.5f, 0.0f));
    enemynode3->CreateComponent<EnemyEntity>();

    SharedPtr<Node> enemynode4(scene_->CreateChild("EnemyNode"));
    enemynode4->SetPosition(Vector3(5.0f, 7.5f, 0.0f));
    enemynode4->CreateComponent<EnemyEntity>();*/
}

void GamePlayState::CreateUI()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void GamePlayState::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    renderer->SetNumViewports(1);
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void GamePlayState::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GamePlayState, HandleUpdate));
    SubscribeToEvent(E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GamePlayState, HandleBeginContact));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(GamePlayState, HandleMouseButtonDownPressed));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(GamePlayState, HandleJoystickButtonDownPressed));
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(GamePlayState, HandleJoystickAxisMove));
}

void GamePlayState::HandleBeginContact(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;
    RigidBody2D* BodyA = static_cast<RigidBody2D*>(eventData[P_BODYA].GetPtr());
    RigidBody2D* BodyB = static_cast<RigidBody2D*>(eventData[P_BODYB].GetPtr());
    Node* nodeA = BodyA->GetNode();
    Node* nodeB = BodyB->GetNode();
    if(nodeA->GetName() == "bullet")
        RemoveBulletList.Push(nodeA);
    if(nodeB->GetName() == "bullet")
        RemoveBulletList.Push(nodeB);

}

void GamePlayState::HandleJoystickAxisMove(StringHash eventType, VariantMap& eventData)
{
    using namespace JoystickAxisMove;

    int button = eventData[P_AXIS].GetInt();
    float position = eventData[P_POSITION].GetFloat();
    std::cout<<button<<" - "<<position<<std::endl;
}

void GamePlayState::HandleJoystickButtonDownPressed(StringHash eventType, VariantMap& eventData)
{
    using namespace JoystickButtonDown;

    int button = eventData[P_BUTTON].GetInt();
    std::cout<<button<<std::endl;
}

void GamePlayState::HandleMouseButtonDownPressed(StringHash eventType, VariantMap& eventData)
{
	using namespace MouseButtonDown;

	int button = eventData[P_BUTTON].GetInt();


		if (MOUSEB_LEFT == button)
		{
            player_->Shoot();
		}

		if (MOUSEB_RIGHT == button)
            player_->ChangeTarget();
}

void GamePlayState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Move the camera, scale movement with time step
    //MoveCamera(timeStep);
    Input* input = GetSubsystem<Input>();

    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    /*if (input->GetMouseButtonDown(1))
    {
        std::cout << "Button 1 Down" << std::endl;
    }*/

    Vector2 posplayer(player_->GetNode()->GetPosition2D());


    if (player_)
    {
        // Clear previous controls
        player_->controls_.Set(CTRL_DOWN | CTRL_LEFT | CTRL_RIGHT | CTRL_UP | LOOK_LEFT, false);
        player_->controls_.Set(CTRL_UP, input->GetKeyDown('W'));
        //player_->controls_.Set(CTRL_UP, input-> GetJoystickByIndex(1)->GetButtonDown(11) );
        player_->controls_.Set(CTRL_DOWN, input->GetKeyDown('S'));
        player_->controls_.Set(CTRL_LEFT, input->GetKeyDown('A'));
        player_->controls_.Set(CTRL_RIGHT, input->GetKeyDown('D'));
        player_->controls_.Set(LOOK_LEFT, (posplayer.x_ > GetMousePositionXY().x_));
    }

    if (input->GetKeyDown('O'))
    {
        stateManager_->PopStack();
		stateManager_->PushToStack("MainMenuStage");
    }

    if (input->GetKeyPress(KEY_F7))
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        XMLFile* nodoXMLFile = cache->GetResource<XMLFile>("Scenes/nodo_map.xml");
        XMLElement nodoXML(nodoXMLFile->GetRoot());
        nodeWall->LoadXML(nodoXML);
        std::cout<<"Nodo cargado"<<std::endl;
    }

    //scene_->GetComponent<AStarFinder>()->drawdebug();
    enemy_->CastTarget(player_->GetNode(),true);

    PhysicsWorld2D* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    //physicsWorld->DrawDebugGeometry();

    targetNode_->SetPosition2D(GetMousePositionXY());

    Vector2 campos(posplayer*0.75f + targetNode_->GetPosition2D()*0.25f);
    cameraNode_->Translate2D((campos - cameraNode_->GetPosition2D())*0.05);

    while(!RemoveBulletList.Empty())
    {
        Node* current = RemoveBulletList.Back();
        current->Remove();
        RemoveBulletList.Pop();
    }
}

void GamePlayState::End()
{
    scene_.Reset();
	cameraNode_.Reset();
	if (GetSubsystem<UI>())
	{
		GetSubsystem<UI>()->Clear();
	}
    UnsubscribeFromAllEvents();
	State::End();
}

Vector2 GamePlayState::GetMousePositionXY()
{
    Input* input = GetSubsystem<Input>();
    Graphics* graphics = GetSubsystem<Graphics>();
    Vector3 screenPoint = Vector3((float)input->GetMousePosition().x_ / graphics->GetWidth(), (float)input->GetMousePosition().y_ / graphics->GetHeight(), 0.0f);

    Vector3 worldPoint = cameraNode_->GetComponent<Camera>()->ScreenToWorldPoint(screenPoint);
    return Vector2(worldPoint.x_, worldPoint.y_);
}

void GamePlayState::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown('W'))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('S'))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('A'))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('D'))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }

    if (input->GetKeyDown('O'))
    {
        stateManager_->PopStack();
		stateManager_->PushToStack("MainMenuStage");
    }

}
