///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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

// declare the global variables
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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
	glm::vec3 positionXYZ)
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
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
  /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/pavers.jpg",
		"floor");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/drywall.jpg",
		"drywall");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/beads.png",
		"cylinder");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/circular-brushed-gold-texture.jpg",
		"cylinder_top");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/wood.jpg",
		"plank");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/iphonebox.jpg",
		"box");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/iphone.png",
		"iphone");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/orange.jpg",
		"ball");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/cone.jpg",
		"cone");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/mint.jpg",
		"mint");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/top.png",
		"top");

	// a?er the texture image data is loaded into memory, the 
	// loaded textures need to be bound to texture slots - there 
	// are a total of 16 available slots for scene textures 
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
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.ambientStrength = 0.2f;
	cheeseMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cheeseMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.shininess = 0.3;
	cheeseMaterial.tag = "cheese";

	m_objectMaterials.push_back(cheeseMaterial);

	OBJECT_MATERIAL breadMaterial;
	breadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	breadMaterial.ambientStrength = 0.3f;
	breadMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	breadMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	breadMaterial.shininess = 0.5;
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	OBJECT_MATERIAL darkBreadMaterial;
	darkBreadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	darkBreadMaterial.ambientStrength = 0.2f;
	darkBreadMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	darkBreadMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	darkBreadMaterial.shininess = 0.0;
	darkBreadMaterial.tag = "darkbread";

	m_objectMaterials.push_back(darkBreadMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	backdropMaterial.ambientStrength = 0.6f;
	backdropMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.1f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	grapeMaterial.ambientStrength = 0.1f;
	grapeMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.3f);
	grapeMaterial.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	grapeMaterial.shininess = 0.5;
	grapeMaterial.tag = "grape";

	m_objectMaterials.push_back(grapeMaterial);
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

	// Primary light source (Point light)
	m_pShaderManager->setVec3Value("lightSources[0].position", -3.0f, 5.0f, 8.0f); // Position adjusted for better lighting
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.1f, 0.1f); // Increased ambient light for warmth
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.5f, 0.5f); // Brighter diffuse color
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.4f, 0.4f, 0.4f); // Higher specular color for shininess
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 6.5f); // Increased intensity for more reflection

	// Secondary light source (Point light)
	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 5.0f, 8.0f); // Position adjusted to match first light
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f); // Low ambient for fill
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.6f, 0.6f); // Slightly higher diffuse
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.3f, 0.3f, 0.3f); // Moderate specular
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 5.4f); // Increased intensity for reflection

	// Tertiary light source (Directional light)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 10.0f, 0.0f); // High above for directional lighting
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.3f, 0.3f, 0.3f); // Higher ambient to prevent shadows
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.0f, 1.0f, 1.0f); // Bright white for diffuse
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.5f, 0.5f, 0.5f); // Increased specular for highlights
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 1.5f); // Increased intensity for stronger reflections
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes.
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	glm::vec3 positionXYZ;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 model;
	
	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// --- Render the backdrop ---
	SetShaderTexture("drywall");  // Set the texture for the backdrop
	SetTextureUVScale(1.0f, 1.0f);  // No UV scaling for the backdrop texture
	SetShaderMaterial("metal");    // Use a default material for the backdrop

	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, .6f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -1.1f, -0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plank");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawBoxMesh();

	// --- Render the iPhone box (cube) --- 
	SetShaderTexture("iphone");  // White color
	SetTextureUVScale(1.0f, 1.0f); // No repetition
	SetShaderTexture("metal"); // Metal texture for the iPhone box

	// Scaling and position for the main iPhone box
	scaleXYZ = glm::vec3(3.0f, 0.5f, 1.5f);  // Long and flat like a phone box
	glm::vec3 positionBox(0.0f, -0.5f, 0.0f);  // Placed on the desk

	// Tilt the box a little to the right (rotation around the Y-axis)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 50.0f;  // Tilt to the right
	ZrotationDegrees = 0.0f;

	// Apply transformations and render the main box
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionBox);
	m_basicMeshes->DrawBoxMesh();  // Render the box

	// --- Render the skinny box on top of the main box --- 
	SetShaderTexture("box");  // Use the same texture for the skinny box
	SetTextureUVScale(1.0f, 1.0f); // No repetition
	SetShaderTexture("glass"); // Glass texture for the skinny box

	// Define the scale and position for the skinny box
	scaleXYZ = glm::vec3(3.0f, 0.1f, 1.5f);  // Match the width and depth, but make it thin
	glm::vec3 positionSkinnyBox = positionBox;  // Start at the main box's position

	// Raise it higher above the main box
	positionSkinnyBox.y += scaleXYZ.y + 0.12f;  // Adjust this value as needed for positioning

	// Apply transformations for the skinny box
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionSkinnyBox);

	// Render the skinny box
	m_basicMeshes->DrawBoxMesh();

	// --- Render the tapered cylinder (soda can) on top of the iPhone box --- 
	SetShaderTexture("cone"); // Use the same texture; you can rename it later if needed
	SetShaderTexture("metal"); // Use metal texture for the cylinder

	scaleXYZ = glm::vec3(0.2f, 0.5f, 0.2f); // Base size (width) smaller
	glm::vec3 positionCylinder = positionBox;  // Start at the box's position

	// Move the cylinder on top of the box and adjust its position to the left and slightly forward
	positionCylinder.y += 0.30f;  // Move it on top of the box
	positionCylinder.x -= 0.50f;  // Move it slightly to the left from the center of the box
	positionCylinder.z += 0.80f;  // Move it slightly forward

	// Apply the transformations to the tapered cylinder
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionCylinder);

	// Render the tapered cylinder
	m_basicMeshes->DrawCylinderMesh();  // Ensure this function correctly draws a cylinder

	// --- Render the cone on top of the tapered cylinder --- 
	SetShaderTexture("mint"); // Use the same texture; you can rename it later if needed
	SetShaderTexture("cheese"); // Cheese texture for the cone

	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // Adjust values as needed for the cone
	glm::vec3 positionCone = positionCylinder; // Start at the cylinder's position

	// Move the cone directly on top of the cylinder
	positionCone.y += 0.50f;  // Move it up to sit on top of the cylinder (adjust as needed)

	// Apply the transformations to the cone
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionCone);

	// Render the cone
	m_basicMeshes->DrawConeMesh();  // Ensure this function correctly draws a cone

	// --- Render the orange (sphere) behind the cone ---  
	SetShaderTexture("ball"); // Fruit orange color
	SetShaderTexture("grape"); // Fruit grape color
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.60f);   // Make it smaller like an orange
	glm::vec3 positionOrange = positionBox;  // Start at the box's position

	// Move the orange to the right of the box
	positionOrange.y += 0.65f;  // Move it on top of the box
	positionOrange.z -= 0.45f;  // Move it slightly behind the box
	positionOrange.x += 0.30f;  // Move it to the right from the center of the box 

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionOrange);
	m_basicMeshes->DrawSphereMesh();  // Render the orange

	// --- Render the Downy Unstopables bottle (cylinder) to the left of the iPhone box ---  
	SetShaderTexture("cylinder"); // washer beads texture
	SetShaderTexture("darkbread"); // Dark bread texture for the bottle

	// Set texture wrapping mode to repeat (only if you want it to repeat along the cylinder)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Horizontal wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Vertical clamping to avoid tiling at the top and bottom

	// Adjust texture scaling to make it fit the cylinder smoothly
	SetTextureUVScale(1.0f, 1.0f); // Set to 1.0f for no repetition and proper wrapping around the cylinder

	// Scaling and position for the cylinder
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.4f);   // Larger size for the bottle
	glm::vec3 positionBottle(-3.0f, -0.8f, 0.0f);  // Move it to the left of the box

	// Apply transformations and render the cylinder
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionBottle);
	m_basicMeshes->DrawCylinderMesh(); // Draw the Downy Unstopables bottle

	// --- Render the tapered cylinder (top) on top of the Downy Unstopables bottle ---  
	SetShaderTexture("top"); // Use the same texture or a different one for the top
	SetShaderTexture("darkbread"); // Use the same texture or a different one for the top

	// Adjust texture wrapping for the tapered cylinder (if needed)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Horizontal wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Vertical clamping to avoid tiling at the top and bottom

	// Scaling and position for the tapered cylinder (top)
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.5f); // Adjust size for the top part (wider at the bottom)
	glm::vec3 positionTop = positionBottle; // Start at the bottle's position

	// Move the tapered cylinder on top of the bottle
	positionTop.y += scaleXYZ.y + (scaleXYZ.y / 2.5f); // Move it above the cylinder by half its height
	positionTop.y += 1.65f; // Additional height adjustment to raise the tapered cylinder higher
	positionTop.x = positionBottle.x; // Keep it centered with respect to the bottle
	positionTop.z = positionBottle.z; // Keep the same depth as the bottle

	// Apply transformations and render the tapered cylinder
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionTop);
	m_basicMeshes->DrawTaperedCylinderMesh(); // Ensure you have a function to draw the tapered cylinder


}