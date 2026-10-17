#pragma once
#include "Rubber/Event/Event.h"
namespace Rubber
{
	/**
	* @brief MouseMoveEvent class that represents the mouse move event
	* fields : x The x position of the mouse
	*          y The y position of the mouse
	*/
	class  MouseMoveEvent : public Event
	{
	public:
     MouseMoveEvent(float x, float y)
			:m_MouseX(x), m_MouseY(y) {
		}

		inline float getX() const { return m_MouseX; }

		inline float getY() const { return m_MouseY; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "Mouse moved to x: " << getX() << " y: " << getY();
			return ss.str();
		}
		EVENT_CATEGORY(toUnderType(EventCategory::MOUSE) | toUnderType(EventCategory::INPUT))
		EVENT_TYPE(EventType::MouseMoved)

	private:
		float m_MouseX, m_MouseY;
	};

	/**
	* @brief MouseScrolledEvent class that represents the mouse scrolled event
	* fields : xOffset The x offset of the mouse scroll
	*		   yOffset The y offset of the mouse scroll
	* 
	**/
	class  MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			:m_xOffset(xOffset), m_yOffset(yOffset) {
		}

		inline float getXOffset() const { return m_xOffset; }

		inline float getYOffset() const { return m_yOffset; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "Mouse Scrolled with x: " << getXOffset() << " y: " << getYOffset();
			return ss.str();
		}

		EVENT_CATEGORY(toUnderType(EventCategory::MOUSE) | toUnderType(EventCategory::INPUT))
		EVENT_TYPE(EventType::MouseScrolled)

	private:
		float m_xOffset, m_yOffset;
	};

	/**
	* @brief MouseButtonEvent class that represents the mouse button event
	* Base class for MousePressedEvent and MouseReleasedEvent
	* fields : button The button of the mouse
	**/
	class  MouseButtonEvent : public Event
	{
	public:
		EVENT_CATEGORY(toUnderType(EventCategory::MOUSE_BUTTON) | toUnderType(EventCategory::INPUT))
		inline int getButton() const { return m_Button; }

	protected:
		MouseButtonEvent(int button)
			:m_Button(button) { }

	private:
		int m_Button;
	};

	/**
	* @brief MousePressedEvent class that represents the mouse pressed event 
	**/
	class  MousePressedEvent : public MouseButtonEvent
	{
	public:
		MousePressedEvent(int button)
			:MouseButtonEvent(button){}
		
		EVENT_TYPE(EventType::MouseButtonPressed)
	
		std::string toString() const override
		{
			std::stringstream ss;
			ss << "Mouse key (Pressed): " << getButton();
			return ss.str();
		}
	};

	/**
	* @brief MouseReleasedEvent class that represents the mouse released event
	**/
	class  MouseReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseReleasedEvent(int button)
			:MouseButtonEvent(button){}
		
		EVENT_TYPE(EventType::MouseButtonReleased)
	
		std::string toString() const override
		{
			std::stringstream ss;
			ss << "Mouse key (Released): " << getButton();
			return ss.str();
		}
	};
}
