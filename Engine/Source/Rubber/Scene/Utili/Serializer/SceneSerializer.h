#pragma once
#include <Rubber/Core/Utility.h>

namespace Rubber {

	class Scene;

    /** Retains shared ownership of the Scene used for editor YAML serialization.
     * Supported data: Tag; Transform position/rotation/scale; Camera projection
     * settings, primary/fixed-aspect flags; Sprite color; VisibilityControl flag.
     * Entity IDs are the repeated placeholder 123123141, not persistent identity;
     * loading creates fresh entities. Textures, scripts and runtime state do not
     * round trip. Runtime entry points are unimplemented assertion stubs.
     * Calls require the Scene's live Application/context and thread contracts.
     */
	class SceneSerializer {

	public:
		SceneSerializer(const Ref<Scene>& scene);
		~SceneSerializer() = default;

        /** Writes editor YAML. No reliable final write-status result is provided. */
		void serialize(const std::filesystem::path& filepath);
		void serializeRunTime(const std::filesystem::path& filepath);
		

        /** Appends entities to the context; false for missing scene/entity data.
         * YAML errors can throw; failure can leave partial changes. This is not
         * transactional Open or validation/recovery of arbitrary input.
         */
		bool deserialize(const std::filesystem::path& filepath)	;
		bool deserializeRunTime(const std::filesystem::path& filepath);


	private:
		Ref<Scene> m_Context;
	};
	
}
