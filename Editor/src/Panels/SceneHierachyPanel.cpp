#include "Panels/SceneHierachyPanel.h"
#include <glm/gtc/type_ptr.hpp>
#include "Utilities/FontsManager/FontManager.h"
#include <entt.hpp>
#include <misc/cpp/imgui_stdlib.h>



namespace { // utility functions for draw modules

	 void drawVec3Control(std::string_view label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 70.0f) {
		 ImGui::PushID(label.data()); // Avoid ID collisions
		 if (ImGui::BeginTable("Vec3Control", 2, ImGuiTableFlags_SizingStretchProp)) {
			 ImGui::TableSetupColumn("LabelCol", ImGuiTableColumnFlags_WidthFixed, columnWidth);
			 ImGui::TableSetupColumn("ControlCol", ImGuiTableColumnFlags_WidthStretch);

			 ImGui::TableNextRow();

			 // Label
			 ImGui::TableSetColumnIndex(0);
			 ImGui::Text("%s", label.data());

			 // Controls
			 ImGui::TableSetColumnIndex(1);
			 float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			 ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

			 ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // No gaps between button & drag
			 ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 3.0f - buttonSize.x);

			 // --- X ---
			 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			 ImGui::PushFont(FontManager::getFont("OpenSans-Bold"));
			 if (ImGui::Button("X", buttonSize)) values.x = resetValue;
			 ImGui::SameLine();
			 ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
			 ImGui::PopStyleColor(3);
			 ImGui::PopFont();

			 // --- Y ---
			 ImGui::SameLine();
			 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
			 if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
			 ImGui::SameLine();
			 ImGui::PushFont(FontManager::getFont("OpenSans-Bold"));
			 ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
			 ImGui::PopStyleColor(3);
			 ImGui::PopFont();

			 // --- Z ---
			 ImGui::SameLine();
			 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
			 ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
			 ImGui::PushFont(FontManager::getFont("OpenSans-Bold"));
			 if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
			 ImGui::SameLine();
			 ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
			 ImGui::PopStyleColor(3);
			 ImGui::PopFont();


			 ImGui::PopItemWidth();
			 ImGui::PopStyleVar();

			 ImGui::EndTable();
		 }
		 ImGui::PopID();
	 }


	 template<typename T>
	 void drawComponent(std::string_view name, RB::Entity entity, void(*func)(RB::Entity), bool removable = true) {
		 // allow overlap to make the button on the same line as the tree node
		 ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap; 
		 flag |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

		 if (entity.hasComponent<T>()) {
			 float lineHeight =  ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			 ImVec2 contentRegionWidth = ImGui::GetContentRegionAvail();

			 ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
			 
			 ImGui::Separator();
			 bool isNodeOpen = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), flag, name.data());

			 ImGui::PopStyleVar();
			 ImGui::SameLine(contentRegionWidth.x - lineHeight * 0.5f );

			 // button 
			 if (ImGui::Button("...", ImVec2{ lineHeight, lineHeight})) {
				 ImGui::OpenPopup("ComponentSettings");
			 }

			 // should remove component
			 bool shouldRemoveComponent = false;
			 if (ImGui::BeginPopup("ComponentSettings")) {
				 if (removable) {
					 if (ImGui::MenuItem("Remove Component"))
						 shouldRemoveComponent = true;
				 }
				 ImGui::EndPopup();
			 }

			 if (isNodeOpen) {
				 func(entity);
				 ImGui::TreePop();
			 }

			 if (shouldRemoveComponent)
				 entity.removeComponent<T>();
		 }
	 }
}

namespace Rubber {

	SceneHierachyPanel::SceneHierachyPanel(const Ref<Scene>& scene)
	{
		RB_CORE_ASSERT(scene, "SceneHierachyPanel::setContext: scene is nullptr!");
		setContext(scene);
	}

	void SceneHierachyPanel::setContext(const Ref<Scene>& scene)
	{
		RB_CORE_ASSERT(scene, "SceneHierachyPanel::setContext: scene is nullptr!");
		m_Context = weakRef<Scene>(scene);
		m_SelectionContext = {};
	}

	void SceneHierachyPanel::onImGuiRender()
	{
		{// Show all entities within a scene 
			ImGui::Begin("Scene Hierarchy");

			auto ContextRef = m_Context.lock();
			RB_CORE_ASSERT(ContextRef, "SceneHierachyPanel::onImGuiRender: scene is nullptr!");
			auto view = ContextRef->m_Registry.view<entt::entity>();
			for (auto entity : view) {
				drawEntityNode( { entity, ContextRef.get()});
			}

			// If left mouse button is pressed and not being consumed by other items, de-select
			if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(0)) {
				m_SelectionContext = {};
			}

			//if right click at blank space, pop up a context window where user can create an empty entity
			if (ImGui::BeginPopupContextWindow(0,  1 | ImGuiPopupFlags_NoOpenOverItems)) {
				if (ImGui::MenuItem("Create Entity")) 
					ContextRef->createEntity("Empty Entity");
				ImGui::EndPopup();
			}

			ImGui::End();
		}

		{// Show properties presented for a specific entity
			ImGui::Begin("Property Channel");
			if (m_SelectionContext) {
				drawComponentNode(m_SelectionContext);
			}
			ImGui::End();
		}
	} 

	void SceneHierachyPanel::drawEntityNode(Entity entity)
	{
		auto& tag = entity.getComponent<TagComponent>().tag;

		// Only include the selected flag when the entity is selected
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;

		flags |= ImGuiTreeNodeFlags_SpanAvailWidth; // make the node span the entire width of the window

		// draw the entity's tree node
		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity, flags, "%s", tag.c_str());

		// if the entity node is clicked, select it
		if (ImGui::IsItemClicked()) {
			m_SelectionContext = entity;
		}

		// mark the entity to be removed 
		bool shouldRemoveEntity = false;
		//add a pop up item to let the user delete the entity
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Delete Entity")) 
				shouldRemoveEntity = true;
			ImGui::EndPopup();
		}

		if (opened) {
			ImGui::TreePop();
		}

		// remove if anything else that could possibly query info from the entity is done
		if (shouldRemoveEntity) {
			auto contextRef = m_Context.lock();
			RB_CORE_ASSERT(contextRef, "SceneHierachyPanel::drawEntityNode: scene is nullptr!");
			contextRef->removeEntity(entity);

			// de-select entity if the selected entity is deleted
			if(m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	void SceneHierachyPanel::drawComponentNode(Entity entity) 
	{

		drawComponent<TagComponent>("Tag", entity, [](Entity entity)
			{ 
                std::string& tag = entity.getComponent<TagComponent>().tag;
                ImGui::InputText("##Tag", &tag);
			}, false);

		ImGui::SameLine();
		ImGui::PushItemWidth(-2);
		// draw an additional button to let user add components to current selected entity
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent")) {

			if (ImGui::MenuItem("Camera")) {
				if (m_SelectionContext.hasComponent<CameraComponent>()) {
					ImGui::CloseCurrentPopup();
				}
				else {
					m_SelectionContext.addComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (ImGui::MenuItem("Sprite Renderer")) {
				if(m_SelectionContext.hasComponent<SpriteComponent>()) {
					ImGui::CloseCurrentPopup();
				}
				else {
					m_SelectionContext.addComponent <SpriteComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (ImGui::MenuItem("Transform")) {
				if(m_SelectionContext.hasComponent<TransformComponent>()) {
					ImGui::CloseCurrentPopup();
				}
				else {
					m_SelectionContext.addComponent <TransformComponent>();
					ImGui::CloseCurrentPopup();
				}
			}

			if (ImGui::MenuItem("Visibility")) {
				if (m_SelectionContext.hasComponent<VisibilityControlComponent>()) {
					ImGui::CloseCurrentPopup();
				}
				else {
					m_SelectionContext.addComponent <VisibilityControlComponent>(true);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		drawComponent<TransformComponent>("Transform", entity, [](Entity entity)
			{
				// Position, rotation, scale drag bar
				TransformComponent& tsC = entity.getComponent<TransformComponent>();

				drawVec3Control("Position", tsC.position);
				glm::vec3 rotation = glm::degrees(tsC.rotation);
				drawVec3Control("Rotation", rotation);
				tsC.rotation = glm::radians(rotation);
				drawVec3Control("Scale", tsC.scale);
			}, true);

		drawComponent <VisibilityControlComponent>("Visibility", entity, [](Entity entity) {
			auto& vc = entity.getComponent<VisibilityControlComponent>();
			ImGui::Checkbox("isVisible", &vc.isVisible);
			}, true);


		drawComponent<CameraComponent>("Camera", entity, [](Entity entity)
			{
				auto& cc = entity.getComponent<CameraComponent>();
				SceneCamera& camera = cc.camera;

				// Two check boxes for  primary and fixed aspect ratio properties for each camera 
				ImGui::Checkbox("Primary", &cc.isPrimary);
				ImGui::Checkbox("IsFixedRatio", &cc.isFixedAspectRatio);

				const char* projectionTypeName[] = { "Orthographic", "Perspective" };
				const char* currentProjectionName = projectionTypeName[static_cast<uint8_t>(camera.getProjectionType())];

				if (ImGui::BeginCombo("Projection", currentProjectionName))
				{
					for (int i = 0; i < 2; i++) {

						bool isSelected = strcmp(currentProjectionName, projectionTypeName[i]) == 0;
						if (ImGui::Selectable(projectionTypeName[i], isSelected)) {
							currentProjectionName = projectionTypeName[i];
							camera.setProjectionType(static_cast<SceneCamera::ProjectionType>(i));
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				// Show specifications of each camera 
				switch (camera.getProjectionType()) {
					case SceneCamera::ProjectionType::Ortho:
					{
						float orthoSize = camera.getOrthoSize();
						if (ImGui::DragFloat("Size", &orthoSize)) {
							camera.setOrthoSize(orthoSize);
						}

						float orthoNearClip = camera.getOrthoNearClip();
						if (ImGui::DragFloat("NearClip", &orthoNearClip)) {
							camera.setOrthoNearClip(orthoNearClip);
						}

						float orthoFarClip = camera.getOrthoFarClip();
						if (ImGui::DragFloat("FarClip", &orthoFarClip)) {
							camera.setOrthoFarClip(orthoFarClip);
						}
						break;
					}

					case SceneCamera::ProjectionType::Persp:
					{
						float persFOVY = glm::degrees(camera.getPersFoVY());
						if (ImGui::DragFloat("FOVY", &persFOVY, 2.0f, 0.0f, 120.0f, "%.2f")) {
							camera.setPersFovY(glm::radians(persFOVY));
						}

						float persNearClip = camera.getPersNearClip();
						if (ImGui::DragFloat("NearClip", &persNearClip)) {
							camera.setPersNearClip(persNearClip);
						}

						float persFarClip = camera.getPersFarClip();
						if (ImGui::DragFloat("FarClip", &persFarClip)) {
							camera.setPersFarClip(persFarClip);
						}
						break;
					}
				}
			}, true);


		// Only supports a color picker for a color sprite component for now
		drawComponent<SpriteComponent>("Sprite Renderer", entity, [](Entity entity) {
				auto& sprite = entity.getComponent<SpriteComponent>();
				ImGui::ColorEdit4("Color Picker", glm::value_ptr(sprite.color));
			}, true);
	}
}
