# RB Editor | C++20 / OpenGL 2D Engine

**简体中文** | [English](README.md) | [GameExample 分支](https://github.com/AAAiee/GameFramework/tree/GameExample)

[代码阅读指南](docs/codebase-guide.md) · [验证记录与限制](docs/validation/spec-11.md)（英文）

RB 是一个使用 **C++20、OpenGL、EnTT 和 Dear ImGui** 编写的 2D Engine 学习项目。`Editor` 分支包含一个基础场景编辑器，可以管理 Entity、编辑 Component、预览场景，并将场景保存为 `.rubber` 文件后重新加载。

## 演示

### Editor 布局与场景保存

<p align="center">
  <img src="./Editor%20Demo/UpdatedThemeAndSerialization.gif" alt="RB Editor layout and scene serialization" width="860">
</p>

<table>
  <tr>
    <td width="50%" align="center">
      <strong>Entity 选择与 Inspector</strong><br><br>
      <img src="./Editor%20Demo/EntitySelect.gif" alt="Entity selection and inspector">
    </td>
    <td width="50%" align="center">
      <strong>添加与移除 Component</strong><br><br>
      <img src="./Editor%20Demo/AddOrRemovingComponent.gif" alt="Add and remove components">
    </td>
  </tr>
  <tr>
    <td width="50%" align="center">
      <strong>Camera Component</strong><br><br>
      <img src="./Editor%20Demo/CameraComponentDemo.gif" alt="Camera component editor">
    </td>
    <td width="50%" align="center">
      <strong>创建 Entity 并编辑场景</strong><br><br>
      <img src="./Editor%20Demo/CreateEntityAndDemo-ezgif.com-video-to-gif-converter.gif" alt="Create an entity and edit the scene">
    </td>
  </tr>
</table>

## 项目亮点

- Editor UI 基于 Dear ImGui Dockspace，包含 Scene Hierarchy、Property Inspector、Viewport 和渲染统计面板，各面板可以重新停靠和调整布局。
- `Scene` 使用 EnTT Registry 管理 Entity 与 Component，并通过轻量 `Entity` wrapper 封装 Component 的添加、移除、查询和访问。
- Scene Hierarchy 支持创建、删除和选择 Entity；Inspector 可以编辑 `Tag`、`Transform`、`Visibility`、`Camera` 和 Sprite color，也可以添加或移除 Component。
- Scene 渲染到 OpenGL Framebuffer 后显示在 Editor Viewport 中。Viewport 尺寸变化时，Framebuffer 和非固定宽高比 Camera 的 aspect ratio 会同步更新。
- yaml-cpp 负责 `.rubber` 场景文件的保存与加载，目前会保存和恢复 Tag、Transform、Camera、Sprite color 和 Visibility。
- Engine 还包含 `1/120s` fixed update、OpenGL Renderer2D、渲染统计，以及采用 current / next 双队列的类型化 EventManager。

## Editor 架构

Editor UI 直接修改 `Scene` 中的 ECS 数据。RendererSystem 将 Scene 渲染到 Framebuffer，Viewport 显示渲染结果；SceneSerializer 负责 `.rubber` 场景文件的读写。

```mermaid
flowchart TB
    UI["Editor UI<br/>Scene Hierarchy / Inspector"]
    Scene["Scene<br/>EnTT ECS"]
    Renderer["RendererSystem"]
    Framebuffer["OpenGL Framebuffer"]
    Viewport["Viewport 面板"]
    Serializer["SceneSerializer<br/>.rubber / YAML"]

    UI <--> Scene
    Scene --> Renderer
    Renderer --> Framebuffer
    Framebuffer --> Viewport
    UI -->|打开 / 保存| Serializer
    Serializer <--> Scene
```

## Editor 功能

| 区域 | 已实现功能 |
|---|---|
| Scene Hierarchy | 创建、删除、选择 Entity |
| Property Inspector | 编辑 Tag、Transform、Visibility、Camera 与 Sprite color；添加或移除 Component |
| Camera | Orthographic / Perspective 切换、FOV / Size、Near / Far Clip、Primary 与固定宽高比 |
| Viewport | Framebuffer 离屏渲染、随面板尺寸调整、非固定宽高比 Camera 的 aspect ratio 更新 |
| File | New、Open、Save As；支持 `Ctrl+N`、`Ctrl+O`、`Ctrl+S` |
| Stats | Draw Calls、Quad / Vertex / Index 数量、Frame Time 与 FPS |

## 功能与源码（Feature → Source Code）

| Feature | 主要实现入口 |
|---|---|
| **Dockspace、Viewport 与文件操作** | [`EditorLayer::onImGuiRender`](Editor/src/EditorLayer.cpp#L62)、[`New / Open / Save`](Editor/src/EditorLayer.cpp#L175) |
| **Scene Hierarchy 与 Inspector** | [`SceneHierachyPanel::onImGuiRender`](Editor/src/Panels/SceneHierachyPanel.cpp#L134)、[`drawComponentNode`](Editor/src/Panels/SceneHierachyPanel.cpp#L212) |
| **Scene 与 System 初始化** | [`Scene::createEntity`](Engine/Source/Rubber/Scene/Scene.cpp#L15)、[`Scene::systemsInit`](Engine/Source/Rubber/Scene/Scene.cpp#L51) |
| **Entity Component API** | [`Entity`](Engine/Source/Rubber/Scene/Utili/Entity.h#L8) |
| **YAML 场景保存与加载** | [`SceneSerializer::serialize`](Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.cpp#L153)、[`deserialize`](Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.cpp#L183) |
| **Framebuffer Viewport** | [`EditorLayer::onUpdate`](Editor/src/EditorLayer.cpp#L42)、[`GLFrameBuffer::resize`](Engine/Source/Platform/OpenGL/GLFrameBuffer.cpp#L47) |
| **Scene 渲染** | [`RendererSystem::onUpdate`](Engine/Source/Rubber/Scene/System/RenderSystem.cpp#L21)、[`Renderer2D`](Engine/Source/Rubber/Renderer/Renderer2D.cpp#L66) |
| **Camera projection** | [`SceneCamera`](Engine/Source/Rubber/Scene/SceneCamera.cpp#L12) |
| **双队列 EventManager** | [`EventManager::dispatchAllEvents`](Engine/Source/Rubber/Event/EventManager.cpp#L8) |
| **Fixed update 主循环** | [`Application::run`](Engine/Source/Rubber/Core/Application.cpp#L122) |

## 构建与运行

### 环境

- Windows x64
- Visual Studio 2022 C++ toolchain
- Git（需要初始化 submodules）

### 步骤

1. 克隆默认的 `Editor` 分支并拉取 submodules：

   ```powershell
   git clone --branch Editor --recurse-submodules https://github.com/AAAiee/GameFramework.git
   ```

2. 运行 `Scripts/Setup-Windows.bat`，使用仓库内的 Premake 生成 Visual Studio 2022 solution。
3. 打开生成的 `Rubber Engine.sln`，构建并运行 `Editor` 项目。
