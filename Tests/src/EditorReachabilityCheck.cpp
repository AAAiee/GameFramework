#include "pch.h"
#include "EditorReachabilityCheck.h"
#include "EditorDriver.h"
#include "UiCatalog.h"
#include "EditorLayer.h"
#include "Rubber/Scene/SceneCamera.h"
#include "Rubber/imGui/ItemProbe.h"
#include "Rubber/Input/InputOverlay.h"
#include "Rubber/Input/KeyCodes.h"
#include "Rubber/Event/KeyEvent.h"
#include "Rubber/Window/Window.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace Rubber {
    namespace {
        void require(bool condition, const std::string& message) {
            if (!condition) throw std::runtime_error(message);
        }

        std::size_t taggedCount(Scene& scene) {
            std::size_t count = 0;
            for (auto _ : scene.getRegistry().view<TagComponent>()) {
                (void)_;
                ++count;
            }
            return count;
        }

        Entity firstTagged(Scene& scene) {
            require(taggedCount(scene) == 1, "Scene has no tagged entity");
            auto view = scene.getRegistry().view<TagComponent>();
            return Entity{*view.begin(), &scene};
        }
    }

    class EditorReachabilityFixture final : public EditorLayer {
    public:
        enum class Step {
            Warmup,
            DumpAndStatic,
            CreateEntity,
            SelectRow,
            AddTransform,
            EditTransform,
            EditTransformY,
            EditTransformZ,
            AddSprite,
            EditSprite,
            EditSpriteColor,
            RemoveSprite,
            AddCamera,
            EditCamera,
            OpenProjection,
            PickPerspective,
            EditFov,
            OpenProjectionOrtho,
            PickOrthographic,
            AddVisibility,
            EditVisible,
            DeleteEntity,
            FileNew,
            CreateThenChord,
            ChordNew,
            FileOpenCancel,
            FileSaveAsCancel,
            ChordOpen,
            ChordSave,
            Splitter,
            Setting,
            FileExit,
            Done
        };

        void onAttach() override {
            EditorLayer::onAttach();
            ItemProbe::enable();
        }

        void onDetach() {
            InputOverlay::clear();
            ItemProbe::disable();
            EditorLayer::onDetach();
        }

        void onImGuiRender() override {
            if (m_Completed) return;
            EditorLayer::onImGuiRender();
            ++m_Frames;
            require(m_Frames < 2400, "Editor UI reachability timed out");
            m_Driver.tick();
            if (m_Driver.busy()) return;
            if (m_Driver.dialogCancelPending()) {
                require(m_Driver.sawDialog(), "File menu did not open a native dialog");
                m_Driver.endDialogCancel();
            }
            advance();
        }

        bool completed() const { return m_Completed; }

        void release() {
            onDetach();
            m_ActiveScene.reset();
            m_FrameBuffer.reset();
        }

    private:
        void fail(const std::string& message) {
            std::ostringstream out;
            out << message << "\n";
            m_Driver.dump(out);
            throw std::runtime_error(out.str());
        }

        void expect(std::string_view path, int occurrence = 0) {
            if (!ItemProbe::find(path, occurrence))
                fail("Expected UI path missing: " + std::string(path));
        }

        void clickFileThen(std::string_view item) {
            m_Driver.click("File");
            m_Driver.waitFrames(1);
            m_Driver.click(item);
        }

        void clickAddComponent(std::string_view item) {
            m_Driver.click("Add Component");
            m_Driver.waitFrames(1);
            m_Driver.click(item);
            m_Driver.waitFrames(2);
        }

        void queueDragBeside(std::string_view buttonLabel) {
            const auto button = leftmost(buttonLabel);
            const auto drag = button ? dragBeside(*button) : std::nullopt;
            if (!drag) fail(std::string("No drag beside the ") + std::string(buttonLabel) + " button");
            m_Driver.dragPoint(ItemProbe::center(*drag), ImVec2(80.0f, 0.0f));
            m_Driver.waitFrames(2);
        }

        void chordWithDialog(int key) {
            m_Driver.beginDialogCancel();
            m_Driver.chord(key);
            m_Driver.waitFrames(2);
        }

        std::optional<UiItem> leftmost(std::string_view label) const {
            std::optional<UiItem> best;
            for (const auto& item : ItemProbe::snapshot()) {
                if (item.label != label) continue;
                if (item.max.x - item.min.x < 8.0f) continue;
                if (!best || item.min.x < best->min.x || (item.min.x == best->min.x && item.min.y < best->min.y))
                    best = item;
            }
            return best;
        }

        std::optional<UiItem> dragBeside(const UiItem& button) const {
            std::optional<UiItem> best;
            for (const auto& item : ItemProbe::snapshot()) {
                if (item.window != button.window) continue;
                if (item.id == button.id) continue;
                if (item.max.x - item.min.x < 16.0f) continue;
                if (item.min.y > button.max.y || item.max.y < button.min.y) continue;
                if (item.min.x + 1.0f < button.max.x) continue;
                if (!best || item.min.x < best->min.x)
                    best = item;
            }
            return best;
        }

        std::optional<UiItem> verticalSplitterBesideViewPort() const {
            auto viewPort = ItemProbe::find("ViewPort");
            if (!viewPort) return std::nullopt;
            std::optional<UiItem> best;
            float bestDist = 1.0e9f;
            for (const auto& item : ItemProbe::snapshot()) {
                if (item.label != "##Splitter") continue;
                const float width = item.max.x - item.min.x;
                const float height = item.max.y - item.min.y;
                if (height < width * 2.0f) continue;
                const float midX = (item.min.x + item.max.x) * 0.5f;
                const float dist = std::abs(midX - viewPort->min.x);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = item;
                }
            }
            return best;
        }

        void advance() {
            switch (m_Step) {
                case Step::Warmup:
                    if (m_Frames < 20) return;
                    m_Step = Step::DumpAndStatic;
                    return;
                case Step::DumpAndStatic:
                    std::cout << "OBSERVED: ui-catalog-dump\n";
                    ItemProbe::dump(std::cout);
                    expect("File");
                    expect("Scene Hierarchy");
                    expect("Property Channel");
                    expect("ViewPort");
                    expect("Setting");
                    expect("##Splitter");
                    m_Driver.rightClickBlank("Scene Hierarchy");
                    m_Driver.waitFrames(1);
                    m_Driver.click("Create Entity");
                    m_Step = Step::CreateEntity;
                    return;
                case Step::CreateEntity:
                    require(taggedCount(*m_ActiveScene) == 1, "Create Entity did not add a row");
                    require(firstTagged(*m_ActiveScene).getComponent<TagComponent>().tag == "Empty Entity",
                        "Create Entity did not name the entity Empty Entity");
                    std::cout << "OBSERVED: Create Entity menu added Empty Entity\n";
                    m_Driver.click("Empty Entity");
                    m_Step = Step::SelectRow;
                    return;
                case Step::SelectRow:
                    expect("Add Component");
                    expect("##Tag");
                    m_Driver.click("Add Component");
                    m_Driver.waitFrames(1);
                    m_Driver.click("Transform");
                    m_Step = Step::AddTransform;
                    return;
                case Step::AddTransform: {
                    require(firstTagged(*m_ActiveScene).hasComponent<TransformComponent>(),
                        "Add Component / Transform did not add Transform");
                    std::cout << "OBSERVED: Add Component / Transform reached the registry\n";
                    const auto xButton = leftmost("X");
                    const auto xDrag = xButton ? dragBeside(*xButton) : std::nullopt;
                    if (!xDrag) fail("No Position X drag beside the X button");
                    m_Driver.dragPoint(ItemProbe::center(*xDrag), ImVec2(80.0f, 0.0f));
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditTransform;
                    return;
                }
                case Step::EditTransform: {
                    auto& transform = firstTagged(*m_ActiveScene).getComponent<TransformComponent>();
                    require(transform.position.x != 0.0f, "Transform X drag did not edit the component");
                    m_BeforeY = transform.position.y;
                    const auto xReset = leftmost("X");
                    if (!xReset) fail("No Position X button to reset");
                    m_Driver.clickPoint(ItemProbe::center(*xReset));
                    const auto yButton = leftmost("Y");
                    const auto yDrag = yButton ? dragBeside(*yButton) : std::nullopt;
                    if (!yDrag) fail("No Position Y drag beside the Y button");
                    m_Driver.dragPoint(ItemProbe::center(*yDrag), ImVec2(80.0f, 0.0f));
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditTransformY;
                    return;
                }
                case Step::EditTransformY: {
                    auto& transform = firstTagged(*m_ActiveScene).getComponent<TransformComponent>();
                    require(transform.position.x == 0.0f, "Transform X reset did not run");
                    require(transform.position.y != m_BeforeY, "Transform Y drag did not edit the component");
                    std::cout << "OBSERVED: Transform X reset and Y drag edited the component\n";
                    queueDragBeside("Z");
                    m_Step = Step::EditTransformZ;
                    return;
                }
                case Step::EditTransformZ: {
                    auto& transform = firstTagged(*m_ActiveScene).getComponent<TransformComponent>();
                    require(transform.position.z != 0.0f, "Transform Z drag did not edit the component");
                    std::cout << "OBSERVED: Transform Z drag edited the component\n";
                    clickAddComponent("Sprite Renderer");
                    m_Step = Step::AddSprite;
                    return;
                }
                case Step::AddSprite:
                    require(firstTagged(*m_ActiveScene).hasComponent<SpriteComponent>(),
                        "Add Component / Sprite Renderer did not add Sprite");
                    std::cout << "OBSERVED: Add Component / Sprite Renderer reached the registry\n";
                    expect("Color Picker");
                    m_BeforeColor = firstTagged(*m_ActiveScene).getComponent<SpriteComponent>().color;
                    m_Driver.click("Color Picker");
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditSprite;
                    return;
                case Step::EditSprite: {
                    expect("sv");
                    expect("hue");
                    std::cout << "OBSERVED: Color Picker opened the hue-bar popup\n";
                    m_Driver.click("ViewPort");
                    m_Driver.waitFrames(1);
                    m_Driver.drag("##X", ImVec2(-80.0f, 0.0f));
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditSpriteColor;
                    return;
                }
                case Step::EditSpriteColor: {
                    const auto color = firstTagged(*m_ActiveScene).getComponent<SpriteComponent>().color;
                    require(color.r < m_BeforeColor.r - 0.01f,
                        "ColorEdit4 ##X drag did not edit Sprite color");
                    std::cout << "OBSERVED: ColorEdit4 ##X drag edited Sprite color.r to "
                        << color.r << "\n";
                    m_Driver.click("...", 0, 2);
                    m_Driver.waitFrames(1);
                    m_Driver.click("Remove Component");
                    m_Step = Step::RemoveSprite;
                    return;
                }
                case Step::RemoveSprite:
                    require(!firstTagged(*m_ActiveScene).hasComponent<SpriteComponent>(),
                        "Remove Component left Sprite attached");
                    std::cout << "OBSERVED: Remove Component removed Sprite Renderer\n";
                    clickAddComponent("Camera");
                    m_Step = Step::AddCamera;
                    return;
                case Step::AddCamera: {
                    require(firstTagged(*m_ActiveScene).hasComponent<CameraComponent>(),
                        "Add Component / Camera did not add Camera");
                    std::cout << "OBSERVED: Add Component / Camera reached the registry\n";
                    expect("Primary");
                    expect("IsFixedRatio");
                    expect("Projection");
                    expect("Size");
                    m_BeforeSize = firstTagged(*m_ActiveScene)
                        .getComponent<CameraComponent>().camera.getOrthoSize();
                    m_Driver.click("Primary");
                    m_Driver.click("IsFixedRatio");
                    m_Driver.drag("Size", ImVec2(80.0f, 0.0f));
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditCamera;
                    return;
                }
                case Step::EditCamera: {
                    auto& camera = firstTagged(*m_ActiveScene).getComponent<CameraComponent>();
                    require(camera.isPrimary, "Primary checkbox did not toggle");
                    require(camera.isFixedAspectRatio, "IsFixedRatio checkbox did not toggle");
                    require(camera.camera.getOrthoSize() != m_BeforeSize,
                        "Size drag did not edit the camera");
                    std::cout << "OBSERVED: Camera Primary, IsFixedRatio, and Size edited\n";
                    m_Driver.click("Projection");
                    m_Driver.waitFrames(1);
                    m_Step = Step::OpenProjection;
                    return;
                }
                case Step::OpenProjection:
                    expect("Orthographic");
                    expect("Perspective");
                    m_Driver.click("Perspective");
                    m_Driver.waitFrames(1);
                    m_Step = Step::PickPerspective;
                    return;
                case Step::PickPerspective: {
                    auto& camera = firstTagged(*m_ActiveScene).getComponent<CameraComponent>().camera;
                    require(camera.getProjectionType() == SceneCamera::ProjectionType::Persp,
                        "Projection / Perspective did not switch the camera");
                    std::cout << "OBSERVED: Projection switched to Perspective\n";
                    expect("FOVY");
                    expect("NearClip");
                    expect("FarClip");
                    m_BeforeFov = camera.getPersFoVY();
                    m_Driver.drag("FOVY", ImVec2(40.0f, 0.0f));
                    m_Driver.waitFrames(2);
                    m_Step = Step::EditFov;
                    return;
                }
                case Step::EditFov: {
                    auto& camera = firstTagged(*m_ActiveScene).getComponent<CameraComponent>().camera;
                    require(camera.getPersFoVY() != m_BeforeFov, "FOVY drag did not edit the camera");
                    std::cout << "OBSERVED: FOVY drag edited the perspective camera\n";
                    m_Driver.click("Projection");
                    m_Driver.waitFrames(1);
                    m_Step = Step::OpenProjectionOrtho;
                    return;
                }
                case Step::OpenProjectionOrtho:
                    m_Driver.click("Orthographic");
                    m_Driver.waitFrames(1);
                    m_Step = Step::PickOrthographic;
                    return;
                case Step::PickOrthographic: {
                    auto& camera = firstTagged(*m_ActiveScene).getComponent<CameraComponent>().camera;
                    require(camera.getProjectionType() == SceneCamera::ProjectionType::Ortho,
                        "Projection / Orthographic did not switch the camera");
                    expect("Size");
                    expect("NearClip");
                    expect("FarClip");
                    std::cout << "OBSERVED: Projection switched to Orthographic\n";
                    clickAddComponent("Visibility");
                    m_Step = Step::AddVisibility;
                    return;
                }
                case Step::AddVisibility:
                    require(firstTagged(*m_ActiveScene).hasComponent<VisibilityControlComponent>(),
                        "Add Component / Visibility did not add Visibility");
                    std::cout << "OBSERVED: Add Component / Visibility reached the registry\n";
                    expect("isVisible");
                    m_Driver.click("isVisible");
                    m_Step = Step::EditVisible;
                    return;
                case Step::EditVisible:
                    require(!firstTagged(*m_ActiveScene).getComponent<VisibilityControlComponent>().isVisible,
                        "isVisible checkbox did not toggle");
                    std::cout << "OBSERVED: isVisible checkbox toggled off\n";
                    m_Driver.click("Empty Entity", 1);
                    m_Driver.waitFrames(1);
                    m_Driver.click("Delete Entity");
                    m_Step = Step::DeleteEntity;
                    return;
                case Step::DeleteEntity:
                    require(taggedCount(*m_ActiveScene) == 0, "Delete Entity did not remove the row");
                    std::cout << "OBSERVED: Delete Entity removed Empty Entity\n";
                    clickFileThen("New");
                    m_Step = Step::FileNew;
                    return;
                case Step::FileNew:
                    require(taggedCount(*m_ActiveScene) == 0, "File / New did not clear the scene");
                    std::cout << "OBSERVED: File / New cleared the scene\n";
                    m_Driver.rightClickBlank("Scene Hierarchy");
                    m_Driver.waitFrames(1);
                    m_Driver.click("Create Entity");
                    m_Step = Step::CreateThenChord;
                    return;
                case Step::CreateThenChord:
                    require(taggedCount(*m_ActiveScene) == 1, "Create Entity after New failed");
                    m_Driver.chord(RB_KEY_N);
                    m_Driver.waitFrames(2);
                    m_Step = Step::ChordNew;
                    return;
                case Step::ChordNew:
                    require(taggedCount(*m_ActiveScene) == 0, "Ctrl+N did not clear the scene");
                    std::cout << "OBSERVED: Ctrl+N overlay cleared the scene through shortcutCallBack\n";
                    m_Driver.click("File");
                    m_Driver.waitFrames(1);
                    m_Driver.beginDialogCancel();
                    m_Driver.click("Open...");
                    m_Step = Step::FileOpenCancel;
                    return;
                case Step::FileOpenCancel:
                    std::cout << "OBSERVED: File / Open... opened a native dialog\n";
                    m_Driver.click("File");
                    m_Driver.waitFrames(1);
                    m_Driver.beginDialogCancel();
                    m_Driver.click("Save As...");
                    m_Step = Step::FileSaveAsCancel;
                    return;
                case Step::FileSaveAsCancel:
                    std::cout << "OBSERVED: File / Save As... opened a native dialog\n";
                    chordWithDialog(RB_KEY_O);
                    m_Step = Step::ChordOpen;
                    return;
                case Step::ChordOpen:
                    std::cout << "OBSERVED: Ctrl+O overlay opened a native dialog\n";
                    chordWithDialog(RB_KEY_S);
                    m_Step = Step::ChordSave;
                    return;
                case Step::ChordSave: {
                    std::cout << "OBSERVED: Ctrl+S overlay opened a native dialog\n";
                    const auto splitter = verticalSplitterBesideViewPort();
                    if (!splitter)
                        fail("No vertical dock splitter beside ViewPort");
                    m_BeforeViewport = m_ViewPortDimension;
                    m_Driver.dragPoint(ItemProbe::center(*splitter), ImVec2(80.0f, 0.0f));
                    m_Driver.waitFrames(3);
                    m_Step = Step::Splitter;
                    return;
                }
                case Step::Splitter:
                    require(m_ViewPortDimension.x != m_BeforeViewport.x
                        || m_ViewPortDimension.y != m_BeforeViewport.y,
                        "ViewPort splitter drag did not change panel size");
                    std::cout << "OBSERVED: ViewPort splitter drag changed panel size to "
                        << m_ViewPortDimension.x << "x" << m_ViewPortDimension.y << "\n";
                    m_Step = Step::Setting;
                    return;
                case Step::Setting: {
                    ImGuiWindow* setting = ImGui::FindWindowByName("Setting");
                    require(setting && setting->Active && setting->DrawList
                        && setting->DrawList->VtxBuffer.Size > 0,
                        "Setting window did not submit stats");
                    std::cout << "OBSERVED: Setting window submitted DrawCount stats\n";
                    clickFileThen("Exit");
                    m_Step = Step::FileExit;
                    return;
                }
                case Step::FileExit:
                    m_Completed = true;
                    std::cout << "OBSERVED: File / Exit closed the editor\n";
                    m_Step = Step::Done;
                    return;
                case Step::Done:
                    return;
            }
        }

        EditorDriver m_Driver;
        glm::vec2 m_BeforeViewport{};
        glm::vec4 m_BeforeColor{1.0f};
        float m_BeforeY = 0.0f;
        float m_BeforeFov = 0.0f;
        float m_BeforeSize = 0.0f;
        unsigned m_Frames = 0;
        Step m_Step = Step::Warmup;
        bool m_Completed = false;
    };

    class EditorEscapeFixture final : public EditorLayer {
    public:
        void onAttach() override {
            EditorLayer::onAttach();
            ItemProbe::enable();
        }

        void onDetach() {
            ItemProbe::disable();
            EditorLayer::onDetach();
        }

        void onImGuiRender() override {
            if (m_Posted) return;
            EditorLayer::onImGuiRender();
            ++m_Frames;
            require(m_Frames < 300, "Escape fixture timed out before posting Escape");
            if (m_Frames < 12) return;
            Application::getEventManager()->enqueue(KeyPressedEvent(RB_KEY_ESCAPE, 0));
            m_Posted = true;
        }

        bool posted() const { return m_Posted; }

        void release() {
            onDetach();
            m_ActiveScene.reset();
            m_FrameBuffer.reset();
        }

    private:
        unsigned m_Frames = 0;
        bool m_Posted = false;
    };

    void checkEditorUi(Application& application) {
        auto* fixture = new EditorReachabilityFixture();
        application.pushLayer(fixture);
        glfwShowWindow(static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow()));
        application.run();
        require(fixture->completed(), "Editor UI driver exited before File / Exit");
        fixture->release();
    }

    void checkEditorUiEscape(Application& application) {
        auto* fixture = new EditorEscapeFixture();
        application.pushLayer(fixture);
        glfwShowWindow(static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow()));
        application.run();
        require(fixture->posted(), "Application closed without posting Escape");
        std::cout << "OBSERVED: Escape posted WindowCloseEvent and closed the editor\n";
        fixture->release();
    }
}
