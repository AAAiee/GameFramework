#include "pch.h"  
#include "WindowInput.h"  
#include <GLFW/glfw3.h>
#include "Rubber/Core/Application.h"
#include "Rubber/Window/Window.h"
#include "Rubber/Input/InputOverlay.h"

namespace Rubber {  
   Input* Input::s_Instance = new WindowInput();
   
   bool WindowInput::isKeyPressedImpl(int keyCode){
	   if (InputOverlay::isPressed(keyCode))
		   return true;
	   auto win =static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
	   bool state = glfwGetKey(win, keyCode);
	   return state == GLFW_PRESS;
   }  

   bool WindowInput::isMouseButtonPressedImpl(int button) {
	   auto win = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
	   bool state = glfwGetMouseButton(win, button);
	   return state == GLFW_PRESS;
   }  

   std::pair<float, float> WindowInput::getMousePositionImpl(){  
	   auto win = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
	   double xPos, yPos;
	   glfwGetCursorPos(win, &xPos, &yPos);
	   return { (float)xPos, (float)yPos };
   }

   void WindowInput::disableCursorImpl()
   {
	   auto win = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
	   glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   }

   void WindowInput::enableCursorImpl()
   {
	   auto win = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
	   glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
   }

}
