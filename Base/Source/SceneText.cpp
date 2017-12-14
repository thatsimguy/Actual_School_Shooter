#include "SceneText.h"
#include "GL\glew.h"

#include "shader.hpp"
#include "MeshBuilder.h"
#include "Application.h"
#include "Utility.h"
#include "LoadTGA.h"
#include <sstream>
#include "KeyboardController.h"
#include "MouseController.h"
#include "SceneManager.h"
#include "GraphicsManager.h"
#include "ShaderProgram.h"
#include "EntityManager.h"

#include "GenericEntity.h"
#include "GroundEntity.h"
#include "TextEntity.h"
#include "SpriteEntity.h"
#include "Light.h"
#include "SkyBox/SkyBoxEntity.h"
#include "SceneGraph\SceneGraph.h"
#include "SpatialPartition\SpatialPartition.h"

#include <iostream>
using namespace std;

SceneText* SceneText::sInstance = new SceneText(SceneManager::GetInstance());

SceneText::SceneText()
{
}

SceneText::SceneText(SceneManager* _sceneMgr)
{
	_sceneMgr->AddScene("Start", this);
}

SceneText::~SceneText()
{
	CSpatialPartition::GetInstance()->RemoveCamera();
	CSceneGraph::GetInstance()->Destroy();
}

void SceneText::Init()
{
	currProg = GraphicsManager::GetInstance()->LoadShader("default", "Shader//Texture.vertexshader", "Shader//Texture.fragmentshader");
	
	// Tell the shader program to store these uniform locations
	currProg->AddUniform("MVP");
	currProg->AddUniform("MV");
	currProg->AddUniform("MV_inverse_transpose");
	currProg->AddUniform("material.kAmbient");
	currProg->AddUniform("material.kDiffuse");
	currProg->AddUniform("material.kSpecular");
	currProg->AddUniform("material.kShininess");
	currProg->AddUniform("lightEnabled");
	currProg->AddUniform("numLights");
	currProg->AddUniform("lights[0].type");
	currProg->AddUniform("lights[0].position_cameraspace");
	currProg->AddUniform("lights[0].color");
	currProg->AddUniform("lights[0].power");
	currProg->AddUniform("lights[0].kC");
	currProg->AddUniform("lights[0].kL");
	currProg->AddUniform("lights[0].kQ");
	currProg->AddUniform("lights[0].spotDirection");
	currProg->AddUniform("lights[0].cosCutoff");
	currProg->AddUniform("lights[0].cosInner");
	currProg->AddUniform("lights[0].exponent");
	currProg->AddUniform("lights[1].type");
	currProg->AddUniform("lights[1].position_cameraspace");
	currProg->AddUniform("lights[1].color");
	currProg->AddUniform("lights[1].power");
	currProg->AddUniform("lights[1].kC");
	currProg->AddUniform("lights[1].kL");
	currProg->AddUniform("lights[1].kQ");
	currProg->AddUniform("lights[1].spotDirection");
	currProg->AddUniform("lights[1].cosCutoff");
	currProg->AddUniform("lights[1].cosInner");
	currProg->AddUniform("lights[1].exponent");
	currProg->AddUniform("colorTextureEnabled");
	currProg->AddUniform("colorTexture");
	currProg->AddUniform("textEnabled");
	currProg->AddUniform("textColor");
	
	// Tell the graphics manager to use the shader we just loaded
	GraphicsManager::GetInstance()->SetActiveShader("default");

	lights[0] = new Light();
	GraphicsManager::GetInstance()->AddLight("lights[0]", lights[0]);
	lights[0]->type = Light::LIGHT_DIRECTIONAL;
	lights[0]->position.Set(0, 20, 0);
	lights[0]->color.Set(1, 1, 1);
	lights[0]->power = 1;
	lights[0]->kC = 1.f;
	lights[0]->kL = 0.01f;
	lights[0]->kQ = 0.001f;
	lights[0]->cosCutoff = cos(Math::DegreeToRadian(45));
	lights[0]->cosInner = cos(Math::DegreeToRadian(30));
	lights[0]->exponent = 3.f;
	lights[0]->spotDirection.Set(0.f, 1.f, 0.f);
	lights[0]->name = "lights[0]";

	lights[1] = new Light();
	GraphicsManager::GetInstance()->AddLight("lights[1]", lights[1]);
	lights[1]->type = Light::LIGHT_DIRECTIONAL;
	lights[1]->position.Set(1, 1, 0);
	lights[1]->color.Set(1, 1, 0.5f);
	lights[1]->power = 0.4f;
	lights[1]->name = "lights[1]";

	currProg->UpdateInt("numLights", 1);
	currProg->UpdateInt("textEnabled", 0);
	
	// Create the playerinfo instance, which manages all information about the player
	playerInfo = CPlayerInfo::GetInstance();
	playerInfo->Init();

	// Create and attach the camera to the scene
	//camera.Init(Vector3(0, 0, 10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	camera.Init(playerInfo->GetPos(), playerInfo->GetTarget(), playerInfo->GetUp());
	playerInfo->AttachCamera(&camera);
	GraphicsManager::GetInstance()->AttachCamera(&camera);

	// Load all the meshes
	MeshBuilder::GetInstance()->GenerateAxes("reference");
	MeshBuilder::GetInstance()->GenerateCrossHair("crosshair");
	MeshBuilder::GetInstance()->GenerateQuad("quad", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("quad")->textureID = LoadTGA("Image//calibri.tga");
	MeshBuilder::GetInstance()->GenerateText("text", 16, 16);
	MeshBuilder::GetInstance()->GetMesh("text")->textureID = LoadTGA("Image//calibri.tga");
	MeshBuilder::GetInstance()->GetMesh("text")->material.kAmbient.Set(1, 0, 0);
	MeshBuilder::GetInstance()->GenerateRing("ring", Color(1, 0, 1), 36, 1, 0.5f);
	MeshBuilder::GetInstance()->GenerateSphere("lightball", Color(1, 1, 1), 18, 36, 1.f);
	MeshBuilder::GetInstance()->GenerateSphere("sphere", Color(1, 0, 0), 18, 36, 0.5f);
	MeshBuilder::GetInstance()->GenerateCone("cone", Color(0.5f, 1, 0.3f), 36, 10.f, 10.f);
	MeshBuilder::GetInstance()->GenerateCube("cube", Color(1.0f, 1.0f, 0.0f), 1.0f);
	MeshBuilder::GetInstance()->GetMesh("cone")->material.kDiffuse.Set(0.99f, 0.99f, 0.99f);
	MeshBuilder::GetInstance()->GetMesh("cone")->material.kSpecular.Set(0.f, 0.f, 0.f);
	MeshBuilder::GetInstance()->GenerateQuad("GRASS_DARKGREEN", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("GRASS_DARKGREEN")->textureID = LoadTGA("Image//Snow.tga");
	MeshBuilder::GetInstance()->GenerateQuad("GEO_GRASS_LIGHTGREEN", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("GEO_GRASS_LIGHTGREEN")->textureID = LoadTGA("Image//Snow.tga");
	MeshBuilder::GetInstance()->GenerateCube("cubeSG", Color(1.0f, 0.64f, 0.0f), 1.0f);

	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_FRONT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_BACK", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_LEFT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_RIGHT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_TOP", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_BOTTOM", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_FRONT")->textureID = LoadTGA("Image//SkyBox//violentdays_ft.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_BACK")->textureID = LoadTGA("Image//SkyBox//violentdays_bk.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_LEFT")->textureID = LoadTGA("Image//SkyBox//violentdays_lf.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_RIGHT")->textureID = LoadTGA("Image//SkyBox//violentdays_rt.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_TOP")->textureID = LoadTGA("Image//SkyBox//violentdays_up.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_BOTTOM")->textureID = LoadTGA("Image//SkyBox//violentdays_dn.tga");
	MeshBuilder::GetInstance()->GenerateRay("laser", 10.0f);
	MeshBuilder::GetInstance()->GenerateQuad("GRIDMESH", Color(1, 1, 1), 10.f);

	// Dick
	MeshBuilder::GetInstance()->GenerateOBJ("Dick", "OBJ//dick.obj");
	MeshBuilder::GetInstance()->GetMesh("Dick")->textureID = LoadTGA("Image//Pig.tga");
	MeshBuilder::GetInstance()->GenerateOBJ("DickHead", "OBJ//dick_head.obj");
	MeshBuilder::GetInstance()->GetMesh("DickHead")->textureID = LoadTGA("Image//Pig.tga");
	MeshBuilder::GetInstance()->GenerateOBJ("DickWhy", "OBJ//dick_why.obj");
	MeshBuilder::GetInstance()->GetMesh("DickWhy")->textureID = LoadTGA("Image//Pig.tga");

	// Door
	MeshBuilder::GetInstance()->GenerateOBJ("Door", "OBJ//door.obj");
	MeshBuilder::GetInstance()->GetMesh("Door")->textureID = LoadTGA("Image//Door.tga");

	// Coke
	MeshBuilder::GetInstance()->GenerateOBJ("CokeCola", "OBJ//Coke_UnRendered.obj");
	MeshBuilder::GetInstance()->GetMesh("CokeCola")->textureID = LoadTGA("Image//Red.tga");

	MeshBuilder::GetInstance()->GenerateOBJ("Coke", "OBJ//Coke.obj");
	MeshBuilder::GetInstance()->GetMesh("Coke")->textureID = LoadTGA("Image//coke_label.tga");

	//Models
	//High Resulotion
	MeshBuilder::GetInstance()->GenerateOBJ("Model", "OBJ//FuckThis.obj");
	MeshBuilder::GetInstance()->GetMesh("Model")->textureID = LoadTGA("Image//NPC.tga");
	//Medium Resolution
	MeshBuilder::GetInstance()->GenerateOBJ("Model3", "OBJ//Models.obj");
	MeshBuilder::GetInstance()->GetMesh("Model3")->textureID = LoadTGA("Image//ModelHigh.tga");

	//Low Resolution
	MeshBuilder::GetInstance()->GenerateOBJ("Model2", "OBJ//FuckYou.obj");
	MeshBuilder::GetInstance()->GetMesh("Model2")->textureID = LoadTGA("Image//Red.tga");


	//FUCK THIS 
	MeshBuilder::GetInstance()->GenerateOBJ("Hillary", "OBJ//Hillary.obj");
	MeshBuilder::GetInstance()->GetMesh("Hillary")->textureID = LoadTGA("Image//SignBoard.tga");
	MeshBuilder::GetInstance()->GenerateOBJ("Stick", "OBJ//Stick.obj");
	MeshBuilder::GetInstance()->GetMesh("Stick")->textureID = LoadTGA("Image//SignBoard.tga");

	//Explosive Pig
	MeshBuilder::GetInstance()->GenerateOBJ("Pork", "OBJ//Explosive_Pig.obj");
	MeshBuilder::GetInstance()->GetMesh("Pork")->textureID = LoadTGA("Image//Pig.tga");

	MeshBuilder::GetInstance()->GenerateQuad("Pistolicon", Color(1, 1, 1), 2.f);
	MeshBuilder::GetInstance()->GetMesh("Pistolicon")->textureID = LoadTGA("Image//pistol_icon.tga");

	// Set up the Spatial Partition and pass it to the EntityManager to manage
	CSpatialPartition::GetInstance()->Init(100, 100, 10, 10);
	CSpatialPartition::GetInstance()->SetMesh("GRIDMESH");
	CSpatialPartition::GetInstance()->SetCamera(&camera);
	CSpatialPartition::GetInstance()->SetLevelOfDetails(40000.0f, 160000.0f);
	EntityManager::GetInstance()->SetSpatialPartition(CSpatialPartition::GetInstance());

	// Create entities into the scene
	Create::Entity("reference", Vector3(0.0f, 0.0f, 0.0f)); // Reference
	Create::Entity("lightball", Vector3(lights[0]->position.x, lights[0]->position.y, lights[0]->position.z)); // Lightball
	//Create::Entity("Model3", Vector3(45.0f, 00.0f, 15.0f));


	// Door object
	m_doorLocation = Vector3(10.f, -10.f, 90.f);
	GenericEntity* aDoor = Create::Asset("Door", m_doorLocation, Vector3(5.f, 5.f, 5.f));
	aDoor->SetCollider(true);
	aDoor->SetAABB(Vector3(0.2f, 0.2f, 0.2f), Vector3(-0.5f, -0.5f, -0.5f));
	EntityManager::GetInstance()->AddEntity(aDoor, false);

	// Coke object
	GenericEntity *CokeCan = Create::Entity("Coke", Vector3(35.f, -10.0f, 5.0f), Vector3(5.f, 5.f, 5.f));
	CokeCan->SetCollider(true);
	CokeCan->SetAABB(Vector3(10.5f, 10.5f, 10.5f), Vector3(-10.5f, -10.5f, -10.5f));
	CokeCan->InitLOD("Coke", "CokeCola","sphere");

	/*GenericEntity *NPC = Create::Entity("Model", Vector3(45.f, 00.0f, 15.0f), Vector3(5.f, 5.f, 5.f));
	NPC->SetCollider(true);
	NPC->SetAABB(Vector3(10.5f, 10.5f, 10.5f), Vector3(-10.5f, -10.5f, -10.5f));*/
	//CokeCan->InitLOD("Coke", "CokeCola", "sphere");


	// Dick object
	GenericEntity* aDick = Create::Entity("DickHead", Vector3(0.f, 0.f, 0.f));
	aDick->SetCollider(true);
	aDick->SetAABB(Vector3(0.5f, 0.5f, 0.5f), Vector3(-0.5f, -0.5f, -0.5f));
	aDick->InitLOD("DickHead", "Dick", "DickWhy");

	// NPC object
	GenericEntity* NPC = Create::Entity("Model", Vector3(30, -10, 50),Vector3(2.5,2.5,2.5));
	NPC->SetCollider(true);
	NPC->SetAABB(Vector3(10.5f, 10.5f, 10.5f), Vector3(-10.5f, -10.5f, -10.5f));
	NPC->InitLOD("Model","Model3", "Model2");




	// FIRST CUBE
	GenericEntity* aCube = Create::Entity("Stick", Vector3(-20.0f, -10.0f, -20.0f));
	aCube->SetCollider(true);
	aCube->SetAABB(Vector3(1.9f, 3.9f, 1.9f), Vector3(-1.9f, -3.9f, -1.9f));
	//aCube->InitLOD("cube", "sphere", "cubeSG");



	// Making first cube into base (Scene Graph)
	CSceneNode* theNode = CSceneGraph::GetInstance()->AddNode(aCube);
	if (theNode == NULL)
	{
		cout << "EntityManager::AddEntity: Unable to add to scene graph!" << endl;
	}



	//SECOND CUBE
	GenericEntity* anotherCube = Create::Entity("Hillary", Vector3(-20.0f, 0.0f, -20.0f));
	anotherCube->SetCollider(true);
	anotherCube->SetAABB(Vector3(5.0f, 5.0f, 5.0f), Vector3(-5.0f, -5.0f, -5.0f));



	// Making second cube into child (Scene Graph)
	CSceneNode* anotherNode = theNode->AddChild(anotherCube);
	
	if (anotherNode == NULL)
	{
		cout << "EntityManager::AddEntity: Unable to add to scene graph!" << endl;
	}
	
	


	/*GenericEntity* baseCube = Create::Asset("cube", Vector3(0.0f, 0.0f, 0.0f));
	CSceneNode* baseNode = CSceneGraph::GetInstance()->AddNode(baseCube);

	CUpdateTransformation* baseMtx = new CUpdateTransformation();
	baseMtx->ApplyUpdate(1.0f, 0.0f, 0.0f, 1.0f);
	baseMtx->SetSteps(-60, 60);
	baseNode->SetUpdateTransformation(baseMtx);

	GenericEntity* childCube = Create::Asset("cubeSG", Vector3(0.0f, 0.0f, 0.0f));
	CSceneNode* childNode = baseNode->AddChild(childCube);
	childNode->ApplyTranslate(0.0f, 1.0f, 0.0f);

	GenericEntity* grandchildCube = Create::Asset("cubeSG", Vector3(0.0f, 0.0f, 0.0f));
	CSceneNode* grandchildNode = childNode->AddChild(grandchildCube);
	grandchildNode->ApplyTranslate(0.0f, 0.0f, 1.0f);
	CUpdateTransformation* aRotateMtx = new CUpdateTransformation();
	aRotateMtx->ApplyUpdate(1.0f, 0.0f, 0.0f, 1.0f);
	aRotateMtx->SetSteps(-120, 60);
	grandchildNode->SetUpdateTransformation(aRotateMtx);*/
	
	groundEntity = Create::Ground("GRASS_DARKGREEN", "GEO_GRASS_LIGHTGREEN");
//	Create::Text3DObject("text", Vector3(0.0f, 0.0f, 0.0f), "DM2210", Vector3(10.0f, 10.0f, 10.0f), Color(0, 1, 1));
	Create::Sprite2DObject("crosshair", Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));
	Create::Sprite2DObject("Pistolicon", Vector3(170.0f, -150.0f, 0.0f), Vector3(270.0f, 200.0f, 200.0f));
	//fuckin hardcoded this mother fucking shit

	SkyBoxEntity* theSkyBox = Create::SkyBox("SKYBOX_FRONT", "SKYBOX_BACK",
											 "SKYBOX_LEFT", "SKYBOX_RIGHT",
											 "SKYBOX_TOP", "SKYBOX_BOTTOM");

	// Customise the ground entity
	groundEntity->SetPosition(Vector3(0, -10, 0));
	groundEntity->SetScale(Vector3(100.0f, 100.0f, 100.0f));
	groundEntity->SetGrids(Vector3(10.0f, 1.0f, 10.0f));
	playerInfo->SetTerrain(groundEntity);

	// Create a CEnemy instance
	srand((int)time(NULL));
	for (size_t i = 0; i < 10; i++)
	{
		theEnemy = new CEnemy();
		float x = 1.0f + (i * rand() % 1000 - 500.0f);
		float y = 1.0f + (i * rand() % 1000 - 500.0f);
		theEnemy->SetRandomSeed(rand());
		theEnemy->Init(x, y);
		theEnemy->SetTerrain(groundEntity);

		// Target is the door (exit)
		theEnemy->SetTarget(Vector3(m_doorLocation.x, 0.f, m_doorLocation.z));

		theEnemy->SetDoorLocation(Vector3(m_doorLocation.x, 0.f, m_doorLocation.z));
		theEnemy = NULL;

		playerInfo->left++;
	}

	// Setup the 2D entities
	float halfWindowWidth = Application::GetInstance().GetWindowWidth() / 2.0f;
	float halfWindowHeight = Application::GetInstance().GetWindowHeight() / 2.0f;
	float fontSize = 25.0f;
	float halfFontSize = fontSize / 2.0f;
	for (int i = 0; i < 10; ++i)
	{
		textObj[i] = Create::Text2DObject("text", Vector3(-halfWindowWidth, -halfWindowHeight + fontSize*i + halfFontSize, 0.0f), "", Vector3(fontSize, fontSize, fontSize), Color(0.0f,1.0f,0.0f));
	}
	textObj[0]->SetText("Shooter Simulator");
}

void SceneText::Update(double dt)
{
	// Update our entities
	EntityManager::GetInstance()->Update(dt);

	// THIS WHOLE CHUNK TILL <THERE> CAN REMOVE INTO ENTITIES LOGIC! Or maybe into a scene function to keep the update clean
	if(KeyboardController::GetInstance()->IsKeyDown('1'))
		glEnable(GL_CULL_FACE);
	if(KeyboardController::GetInstance()->IsKeyDown('2'))
		glDisable(GL_CULL_FACE);
	if(KeyboardController::GetInstance()->IsKeyDown('3'))
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if(KeyboardController::GetInstance()->IsKeyDown('4'))
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	if(KeyboardController::GetInstance()->IsKeyDown('5'))
	{
		lights[0]->type = Light::LIGHT_POINT;
	}
	else if(KeyboardController::GetInstance()->IsKeyDown('6'))
	{
		lights[0]->type = Light::LIGHT_DIRECTIONAL;
	}
	else if(KeyboardController::GetInstance()->IsKeyDown('7'))
	{
		lights[0]->type = Light::LIGHT_SPOT;
	}

	if(KeyboardController::GetInstance()->IsKeyDown('I'))
		lights[0]->position.z -= (float)(10.f * dt);
	if(KeyboardController::GetInstance()->IsKeyDown('K'))
		lights[0]->position.z += (float)(10.f * dt);
	if(KeyboardController::GetInstance()->IsKeyDown('J'))
		lights[0]->position.x -= (float)(10.f * dt);
	if(KeyboardController::GetInstance()->IsKeyDown('L'))
		lights[0]->position.x += (float)(10.f * dt);
	if(KeyboardController::GetInstance()->IsKeyDown('O'))
		lights[0]->position.y -= (float)(10.f * dt);
	if(KeyboardController::GetInstance()->IsKeyDown('P'))
		lights[0]->position.y += (float)(10.f * dt);

	if (KeyboardController::GetInstance()->IsKeyReleased('M'))
	{
		CSceneNode* theNode = CSceneGraph::GetInstance()->GetNode(1);
		Vector3 pos = theNode->GetEntity()->GetPosition();
		theNode->GetEntity()->SetPosition(Vector3(pos.x + 50.0f, pos.y, pos.z + 50.0f));
	}
	if (KeyboardController::GetInstance()->IsKeyReleased('N'))
	{
		CSpatialPartition::GetInstance()->PrintSelf();
	}

	// if the left mouse button was released
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::LMB))
	{
		cout << "Left Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::RMB))
	{
		cout << "Right Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::MMB))
	{
		cout << "Middle Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_XOFFSET) != 0.0)
	{
		cout << "Mouse Wheel has offset in X-axis of " << MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_XOFFSET) << endl;
	}
	if (MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_YOFFSET) != 0.0)
	{
		cout << "Mouse Wheel has offset in Y-axis of " << MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_YOFFSET) << endl;
	}
	// <THERE>

	// Update the player position and other details based on keyboard and mouse inputs
	playerInfo->Update(dt);

	//camera.Update(dt); // Can put the camera into an entity rather than here (Then we don't have to write this)
	//pls work
	/*CameraEffects_One_O_One->pistolicon = MeshBuilder::GetInstance()->GenerateQuad("Pistolicon", Color(1, 1, 1), 1.f);
	CameraEffects_One_O_One->pistolicon->textureID = LoadTGA("Image//pistol_icon.tga");*/
	
	GraphicsManager::GetInstance()->UpdateLights(dt);

	std::ostringstream ss;
	ss.precision(5);
	float fps = (float)(1.f / dt);
	ss << "FPS: " << fps;
	textObj[1]->SetText(ss.str());

	std::ostringstream ss1;
	ss1.precision(4);
	ss1 << "Player:" << playerInfo->GetPos();
	textObj[2]->SetText(ss1.str());

	// Output current weapon
	std::ostringstream ss2;
	ss2.precision(4);
	string weaponOutput;
	weaponOutput = "Pistol";
	ss2 << "Weapon:" << weaponOutput;
	textObj[3]->SetText(ss2.str());

	// Output ammo of current weapon
	std::ostringstream ss3;
	ss3.precision(4);
	int ammoOutput = 0;
	int totalAmmo = 0;
	ammoOutput = playerInfo->GetPrimaryAmmo();
	totalAmmo = playerInfo->GetTotalPrimaryAmmo();
	ss3 << "Ammo:" << ammoOutput << "/" << totalAmmo;
	textObj[4]->SetText(ss3.str());

	// Output number of NPCs left
	std::ostringstream ss4;
	ss4.precision(4);
	ss4 << "Left:" << playerInfo->left;
	textObj[4]->SetText(ss4.str());

	// Output score
	std::ostringstream ss5;
	ss5.precision(4);
	ss5 << "Score:" << playerInfo->score;
	textObj[5]->SetText(ss5.str());
}

void SceneText::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GraphicsManager::GetInstance()->UpdateLightUniforms();

	// Setup 3D pipeline then render 3D
	GraphicsManager::GetInstance()->SetPerspectiveProjection(45.0f, 4.0f / 3.0f, 0.1f, 10000.0f);
	GraphicsManager::GetInstance()->AttachCamera(&camera);
	EntityManager::GetInstance()->Render();

	// Setup 2D pipeline then render 2D
	int halfWindowWidth = Application::GetInstance().GetWindowWidth() / 2;
	int halfWindowHeight = Application::GetInstance().GetWindowHeight() / 2;
	GraphicsManager::GetInstance()->SetOrthographicProjection(-halfWindowWidth, halfWindowWidth, -halfWindowHeight, halfWindowHeight, -10, 10);
	GraphicsManager::GetInstance()->DetachCamera();
	EntityManager::GetInstance()->RenderUI();
}

void SceneText::Exit()
{
	// Detach camera from other entities
	GraphicsManager::GetInstance()->DetachCamera();
	playerInfo->DetachCamera();

	if (playerInfo->DropInstance() == false)
	{
#if _DEBUGMODE==1
		cout << "Unable to drop PlayerInfo class" << endl;
#endif
	}

	// Delete the lights
	delete lights[0];
	delete lights[1];
}
