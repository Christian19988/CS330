///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// declaration of the global variables and defines
namespace
{
    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // camera object used for viewing and interacting with the 3D scene
    Camera* g_pCamera = nullptr;

    // these variables are used for mouse movement processing
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // time between current frame and last frame
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // movement speed variable
    float gCameraSpeed = 2.5f; // Default speed
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    // initialize the member variables
    m_pShaderManager = pShaderManager;
    m_pWindow = NULL;
    g_pCamera = new Camera();
    // default camera view parameters
    g_pCamera->Position = glm::vec3(0.5f, 5.5f, 10.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
    // free up allocated memory
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (NULL != g_pCamera)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = nullptr;

    // Try to create the displayed OpenGL window
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    // Set the mouse position and scroll callbacks
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
    glfwSetScrollCallback(window, &ViewManager::Scroll_Callback); // Updated scroll callback

    // Capture mouse and hide the cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Enable blending for supporting transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize the first mouse position
    glfwSetCursorPos(window, WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    gLastX = WINDOW_WIDTH / 2.0f;
    gLastY = WINDOW_HEIGHT / 2.0f;
    gFirstMouse = true; // Reset to true to recenter mouse on first move

    m_pWindow = window;

    return window;
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    // When the first mouse move event is received, record the last positions
    if (gFirstMouse)
    {
        gLastX = xMousePos;
        gLastY = yMousePos;
        gFirstMouse = false;

        // Recenter the mouse cursor
        glfwSetCursorPos(window, WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
        return; // Return to avoid processing on the first move
    }

    // Calculate the offsets for camera movement
    float xOffset = xMousePos - gLastX;
    float yOffset = gLastY - yMousePos; // Inverted y-offset

    // Update last positions
    gLastX = xMousePos;
    gLastY = yMousePos;

    // Move the 3D camera according to the calculated offsets
    if (g_pCamera)
    {
        g_pCamera->ProcessMouseMovement(xOffset, yOffset);

        // Debugging output
        std::cout << "Mouse Offset: (" << xOffset << ", " << yOffset << ")\n";
        std::cout << "Camera Position: (" << g_pCamera->Position.x << ", "
            << g_pCamera->Position.y << ", " << g_pCamera->Position.z << ")\n";
        std::cout << "Camera Yaw: " << g_pCamera->Yaw << ", Pitch: " << g_pCamera->Pitch << "\n";
    }
}

/***********************************************************
 *  Scroll_Callback()
 *
 *  This method is called when the mouse scroll wheel is used.
 ***********************************************************/
void ViewManager::Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
    // Adjust the camera speed based on scroll input
    gCameraSpeed += static_cast<float>(yOffset); // Increase/decrease speed
    gCameraSpeed = glm::clamp(gCameraSpeed, 0.1f, 10.0f); // Clamp speed to a reasonable range

    // Debug output to see the new speed level
    std::cout << "Camera Speed: " << gCameraSpeed << "\n";
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
    // Close the window if the escape key has been pressed
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    // If the camera object is null, then exit this method
    if (g_pCamera == nullptr)
    {
        return;
    }

    // Process camera movements with adjusted speed
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime * gCameraSpeed);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime * gCameraSpeed);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime * gCameraSpeed);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime * gCameraSpeed);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(UP, gDeltaTime * gCameraSpeed);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(DOWN, gDeltaTime * gCameraSpeed);
    }
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // Per-frame timing to control movement speed
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // Process keyboard events for camera control
    ProcessKeyboardEvents();

    // Get the current view matrix from the camera
    view = g_pCamera->GetViewMatrix();

    // Get the current projection matrix based on camera zoom and aspect ratio
    projection = g_pCamera->GetProjectionMatrix((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);

    // Update shader matrices and camera position
    if (m_pShaderManager != nullptr)
    {
        m_pShaderManager->setMat4Value(g_ViewName, view);
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}