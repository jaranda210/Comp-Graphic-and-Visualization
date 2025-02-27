///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ + offset);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the scene textures
	LoadSceneTextures();
	// define the object materials for lighting
	DefineObjectMaterials();
	// add and define light sources for 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("textures/lightwood.jpg", "desk");
	CreateGLTexture("textures/navyblue.jpg", "cup");
	CreateGLTexture("textures/aluminum.png", "cuptb");
	CreateGLTexture("textures/blackplastic.png", "mouse");
	CreateGLTexture("textures/grayplastic.jpg", "computer");
	CreateGLTexture("textures/screen.png", "image");
	CreateGLTexture("textures/keyboard.png", "keyboard");
	CreateGLTexture("textures/mouse.png", "keyboardmouse");
	CreateGLTexture("textures/earphones.jpg", "airpods");
	CreateGLTexture("textures/background.png", "wall");

	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL aluminumMaterial;
	aluminumMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	aluminumMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	aluminumMaterial.shininess = 80.0;
	aluminumMaterial.tag = "aluminum";

	m_objectMaterials.push_back(aluminumMaterial);

	OBJECT_MATERIAL matMaterial;
	matMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	matMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	matMaterial.shininess = 20.0;
	matMaterial.tag = "mat";

	m_objectMaterials.push_back(matMaterial);


	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.shininess = 64.0;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -10.0f, 10.0f, -0.5f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1 (index 0)
	m_pShaderManager->setVec3Value("pointLights[0].position", 7.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.07f, 0.07f, 0.07f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/


	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
	// Mat
	scaleXYZ = glm::vec3(15.0f, 1.0f, 6.0f);
	positionXYZ = glm::vec3(0.0f, 0.8f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.10f, 0.08f, 0.45f, 1.0f);
	SetShaderTexture("cup");
	SetShaderMaterial("mat");

	m_basicMeshes->DrawPlaneMesh();

	// Cup
	scaleXYZ = glm::vec3(1.0f, 3.28f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 1.2f, -1.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.05f, 0.07f, 0.2f, 1.0f);
	// Texture
	SetShaderTexture("cup");
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawCylinderMesh();

	// Cup Top
	scaleXYZ = glm::vec3(1.01f, 0.4f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 4.5f, -1.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.05f, 0.07f, 0.2f, 1.0f);
	// Texture
	SetShaderTexture("cuptb");
	SetShaderMaterial("aluminum");

	m_basicMeshes->DrawCylinderMesh();

	// Cup Bottom
	scaleXYZ = glm::vec3(1.01f, 0.4f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 0.8f, -1.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.05f, 0.07f, 0.2f, 1.0f);
	// Texture
	SetShaderTexture("cuptb");
	SetShaderMaterial("aluminum");

	m_basicMeshes->DrawCylinderMesh();

	// Airpods
	scaleXYZ = glm::vec3(1.5f, 0.6f, 1.4f);
	positionXYZ = glm::vec3(-10.0f, 1.1f, 1.6f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture
	SetShaderTexture("airpods");
	SetShaderMaterial("aluminum");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("aluminum");

	m_basicMeshes->DrawBoxMesh();

	// Mouse
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.5f);
	positionXYZ = glm::vec3(10.0f, 1.1f, 1.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture("mouse");
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.5f);
	positionXYZ = glm::vec3(10.0f, 1.2f, 1.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Texture
	SetShaderTexture("mouse");
	SetShaderMaterial("aluminum");

	m_basicMeshes->DrawSphereMesh();

	// Computer
	// Bottom 
	scaleXYZ = glm::vec3(14.0f, 0.5001f, 7.0f);
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.05f, 0.07f, 0.2f, 1.0f);
	// Texture
	SetShaderTexture("keyboard");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top);

	SetShaderTexture("computer");
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawBoxMesh();

	//Top
	scaleXYZ = glm::vec3(14.0f, 8.0f, 0.5f);
	positionXYZ = glm::vec3(0.0f, 4.8f, -3.7f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.05f, 0.07f, 0.2f, 1.0f);
	// Texture
	SetShaderTexture("image");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);

	SetShaderTexture("computer");
	SetShaderMaterial("plastic");
	
	m_basicMeshes->DrawBoxMesh();

	// Desk
	scaleXYZ = glm::vec3(40.0f, 1.5f, 20.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Texture
	SetShaderTexture("desk");
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();
}