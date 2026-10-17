#include <pch.h>
#include "LayerStack.h"
#include "Rubber/Layer/Layer.h"

Rubber::LayerStack::LayerStack()
	:m_LayerInsertIndex(0)
{
}

Rubber::LayerStack::~LayerStack()
{
	for (Layer* layer : m_Layers)
	{
		delete layer;
	}
}

void Rubber::LayerStack::pushLayer(Layer* layer)
{
  m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
  m_LayerInsertIndex++;
}

void Rubber::LayerStack::popLayer(Layer* layer)
{
	const auto ordinaryEnd = m_Layers.begin() + m_LayerInsertIndex;
	auto it = std::find(m_Layers.begin(), ordinaryEnd, layer);
	if (it != ordinaryEnd)
	{
       m_Layers.erase(it);
       m_LayerInsertIndex--;
	}
}

void Rubber::LayerStack::pushOverlay(Layer* overlay)
{
	m_Layers.emplace_back(overlay);
}

void Rubber::LayerStack::popOverlay(Layer* overlay)
{
	auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
	if (it != m_Layers.end())
	{
		m_Layers.erase(it);
	}
}

// implement begin and end iterator to support enforced loop reference 
Rubber::Vector<Rubber::Layer*>::iterator Rubber::LayerStack::begin() {
	return m_Layers.begin();
}

Rubber::Vector<Rubber::Layer*>::iterator Rubber::LayerStack::end() {
	return m_Layers.end();
}
