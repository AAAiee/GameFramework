#pragma once
#include "Rubber/Core/Utility.h"

namespace Rubber {
	class Layer;
	/** Owns stored pointers and deletes only those still stored at destruction.
	 * Ordinary layers precede overlays. Push takes ownership; a successful pop
	 * removes stack ownership without deleting the object, transferring lifetime
	 * responsibility to the caller. Stack operations do not attach or detach.
	 */
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		/** push the layers in, but always before overlay
		 */
		void pushLayer(Layer* layer);
		
		/** Remove only from ordinary layers; absent or overlay pointers are a no-op.
		 * On removal the caller takes responsibility for the still-live object.
		 */
		void popLayer(Layer* layer);

		/** push overlay to the end;
		 */
		void pushOverlay(Layer* layer);

		/** Remove only from overlays; absent or ordinary pointers are a no-op.
		 * On removal the caller takes responsibility for the still-live object.
		 */
		void popOverlay(Layer* layer);

		/** implement begin and end iterator to support traversing the layers
		 */
		Vector<Layer*>::iterator begin();

		Vector<Layer*>::iterator end();

	private:

		// a contiguous container to store our layers
		Vector<Layer*> m_Layers;

		// using vector because we want to
		// insert layer in the middle, 
		// Overlay at the last,
		// also need reverse iterator for handling events
		unsigned int m_LayerInsertIndex;
	};
}

