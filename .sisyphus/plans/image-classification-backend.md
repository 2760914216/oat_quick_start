# Image Classification & Lightweight Editing Backend

## TL;DR

> **Quick Summary**: 为新闻工作者提供图片分类和轻量编辑功能的后端服务，基于 oatpp 1.4.0 + ONNX Runtime，支持场景/内容分类、非破坏性编辑、缩略图生成
> 
> **Deliverables**:
> - ONNX 分类器框架，支持多维度分类 (场景+内容)
> - 图片编辑 API (裁剪/旋转/缩放/亮度/滤镜)
> - 缩略图生成与存储
> - 扩展的 SQLite 数据库表结构
> - 自动化测试用例
> 
> **Estimated Effort**: Medium
> **Parallel Execution**: YES - 3 waves
> **Critical Path**: DB Schema → Classifier Framework → Thumbnail Service → Classification API → Edit API → Tests

---

## Context

### Original Request
为新闻工作者提供更好的图片筛选分类与轻量编辑功能的后端系统，可供多用户使用。采用 oatpp 1.4.0 (组件均为 1.4.0)，可采用 ONNX 等技术提升图片分拣能力。

### Interview Summary

**Key Discussions**:
- **分类维度**: 场景分类 (会议/采访/活动等) + 内容分类 (人物/风景/文档等)，保留扩展能力
- **编辑功能**: 基本编辑 (裁剪、旋转、缩放、亮度/对比度、滤镜)
- **ONNX 模型**: 采用 ONNX Runtime C++ API，由我推荐方案
- **分类时机**: 同步处理，如果资源占用高则标记 dirty 延后处理
- **非破坏性编辑**: 由用户决定存储方式
- **缩略图**: 列表需要返回缩略图供前端预览
- **测试**: 需要自动化测试

**Research Findings**:
- oatpp 1.4.0 + oatpp-openssl 1.4.0 + oatpp-sqlite 1.4.0
- ONNX Runtime C++ API: `Ort::Env`, `Ort::Session`, tensor creation, `Run()`
- 现有 PhotoController 提供上传/下载/列表/删除 API
- ResManager 管理资源目录类型

### Metis Review

**Identified Gaps** (addressed):
- 分类器的模型加载时机 (启动时懒加载)
- 编辑参数存储策略 (JSON 字段存储)
- 缩略图尺寸规格
- 并发分类场景下的资源管理

---

## Work Objectives

### Core Objective
为新闻工作者提供图片自动分类和轻量编辑能力的后端 API，支持多用户场景

### Phase Structure (里程碑阶段)

```
Phase 1: Foundation (第 1-2 波次)
├── 目标: 建立基础设施
├── 交付物: DB Schema + 依赖库 + 测试框架
└── 验收点: 编译通过，所有依赖就绪

Phase 2: Core Services (第 2-3 波次)
├── 目标: 实现核心业务逻辑
├── 交付物: 分类器 + 图像处理服务
└── 验收点: 单机测试通过

Phase 3: API & Integration (第 3 波次)
├── 目标: 对外 API 和集成
├── 交付物: REST API 端点 + 自动化测试
└── 验收点: API 测试通过
```

### Concrete Deliverables

**1. 数据库扩展**
- 扩展 `photos` 表添加分类字段 (scene_category, content_category, classification_status)
- 新建 `photo_edits` 表存储编辑参数 (非破坏性)
- 新建 `photo_thumbnails` 表存储缩略图路径
- 新建 `classification_models` 表管理 ONNX 模型

**2. 分类器框架**
- `ImageClassifier` 抽象类，支持多模型
- `SceneClassifier` 实现场景分类
- `ContentClassifier` 实现内容分类
- 模型懒加载机制，配置指定模型路径
- 同步/异步分类策略 (配置切换)

**3. 图片处理服务**
- `ThumbnailGenerator`: 缩略图生成 (支持多种尺寸)
- `ImageEditor`: 裁剪/旋转/缩放/亮度/对比度/滤镜
- 使用 stb_image + stb_image_write (轻量级)

**4. API 端点**
- `GET /api/photos/{id}/classification` - 获取分类结果
- `POST /api/photos/{id}/classify` - 手动触发分类
- `POST /api/photos/{id}/edit` - 应用编辑
- `GET /api/photos/{id}/edit` - 获取编辑参数
- `GET /api/photos/{id}/thumbnail` - 获取缩略图
- `POST /api/photos/{id}/revert` - 撤销编辑 (非破坏性)

**5. 测试**
- 分类器单元测试 (mock ONNX)
- 编辑功能测试
- API 集成测试

### Definition of Done

- [ ] 编译通过，无 warning
- [ ] 所有新 API 端点返回正确
- [ ] 分类器能对图片进行分类
- [ ] 缩略图正确生成
- [ ] 编辑参数正确保存和恢复
- [ ] 单元测试通过
- [ ] 集成测试通过

### Must Have

- ONNX Runtime C++ 集成
- 场景分类 + 内容分类 (至少各 3 类)
- 缩略图生成 (支持列表/详情两种尺寸)
- 非破坏性编辑存储
- 配置化的模型路径
- 自动化测试

### Must NOT Have (Guardrails)

- 不修改现有认证逻辑
- 不修改现有用户管理逻辑
- 不处理视频文件
- 不实现前端
- 不训练模型 (只集成推理)

---

## Verification Strategy

### Test Decision

- **Infrastructure exists**: NO (当前项目无测试框架)
- **Automated tests**: YES (需要设置)
- **Framework**: Google Test (gtest)
- **测试策略**: 先搭建测试框架，再实现 TDD

### QA Policy

Every task MUST include agent-executed QA scenarios.

- **Backend API**: 使用 curl 验证 HTTP 响应
- **Image Processing**: 验证输出图片尺寸和内容
- **Classification**: 验证分类结果 JSON 结构

---

## Execution Strategy

### Parallel Execution Waves (细粒度版本)

```
Wave 1 (Foundation - 基础设施) - 8 个任务:
├── Task 1.1: Extend photos table schema - 添加 scene_category, content_category 字段
├── Task 1.2: Add classification metadata fields - 添加 classification_status, dirty 标记
├── Task 1.3: Create photo_edits table - 编辑参数存储表
├── Task 1.4: Create photo_thumbnails table - 缩略图路径表
├── Task 1.5: Create classification_models table - ONNX 模型管理表
├── Task 1.6: Add ONNXRuntime to CMake - onnxruntime 依赖配置
├── Task 1.7: Integrate stb_image library - 图像加载库集成
└── Task 1.8: Setup Google Test framework - 测试框架配置

Wave 2 (Core Services - 核心服务) - 10 个任务:
├── Task 2.1: Define ImageClassifier abstract class - 抽象基类定义
├── Task 2.2: Define ClassificationResult struct - 结果数据结构
├── Task 2.3: Implement ClassifierManager - 多分类器管理
├── Task 2.4: Implement SceneClassifier ONNX integration - 场景分类器
├── Task 2.5: Implement ContentClassifier ONNX integration - 内容分类器
├── Task 2.6: Implement model lazy loading - 模型懒加载机制
├── Task 2.7: Implement ThumbnailGenerator LIST size - 列表缩略图 (150x150)
├── Task 2.8: Implement ThumbnailGenerator DETAIL size - 详情缩略图 (400x400)
├── Task 2.9: Implement ImageEditor crop/rotate/scale - 基本编辑功能
└── Task 2.10: Implement ImageEditor brightness/contrast/filter - 增强编辑功能

Wave 3 (API - 对外接口) - 9 个任务:
├── Task 3.1: GET /photos/{id}/classification endpoint - 获取分类结果 API
├── Task 3.2: POST /photos/{id}/classify endpoint - 手动触发分类 API
├── Task 3.3: POST /photos/{id}/edit endpoint - 应用编辑 API
├── Task 3.4: GET /photos/{id}/edit endpoint - 获取编辑参数 API
├── Task 3.5: POST /photos/{id}/revert endpoint - 撤销编辑 API
├── Task 3.6: GET /photos/{id}/thumbnail endpoint - 获取缩略图 API
├── Task 3.7: Integrate auto-classification on upload - 上传时自动分类
├── Task 3.8: Integrate auto-thumbnail on upload - 上传时自动生成缩略图
└── Task 3.9: Add DB migration handler - 数据库迁移处理

Wave 4 (Tests - 测���) - 4 个任务:
├── Task 4.1: ImageClassifier unit tests - 分类器单元测试
├── Task 4.2: ImageEditor unit tests - 编辑器单元测试
├── Task 4.3: API integration tests - API 集成测试
└── Task 4.4: End-to-end workflow tests - 端到端流程测试
```

### Dependency Matrix (细粒度)

- **Task 1.1-1.2**: photos 表扩展 → Task 3.1, 3.2, 3.7 (分类 API 依赖)
- **Task 1.3-1.5**: 新建表 → Task 3.3, 3.4, 3.5, 3.6 (编辑/缩略图 API 依赖)
- **Task 1.6**: ONNX 依赖 → Task 2.4, 2.5 (分类器实现依赖)
- **Task 1.7**: Image 库 → Task 2.7-2.10 (图像处理依赖)
- **Task 1.8**: 测试框架 → Task 4.1-4.4 (所有测试依赖)
- **Task 2.1-2.3**: 分类器框架 → Task 2.4, 2.5 (具体分类器依赖)
- **Task 2.4-2.5**: 分类器 → Task 3.1, 3.2, 3.7 (API 依赖)
- **Task 2.6**: 懒加载 → Task 2.4, 2.5 (依赖)
- **Task 2.7-2.8**: 缩略图 → Task 3.6, 3.8 (缩略图 API 依赖)
- **Task 2.9-2.10**: 编辑器 → Task 3.3, 3.4, 3.5 (编辑 API 依赖)
- **Task 3.1-3.9**: API → Task 4.3, 4.4 (集成测试依赖)

---

## TODOs (细粒度版本 - 31 个子任务)

### Wave 1: Foundation (8 子任务)

- [x] 1.1. **Extend photos table schema** - 扩展 photos 表添加分类字段

  **What to do**:
  - 修改 `PhotoMetadata` 结构体: 添加 `scene_category`, `content_category`
  - 修改 `PhotoMetadataDb.cpp`: ALTER TABLE ADD COLUMN
  - 添加字段: `scene_category TEXT`, `content_category TEXT`

  **Acceptance Criteria**:
  - [ ] `.schema photos` 包含 scene_category, content_category 字段
  - [ ] 现有数据 migration 正常

  **QA Scenarios**:
  ```
  Scenario: photos 表字段验证
    Tool: Bash
    Steps: sqlite3 database/photo_metadata.db ".schema photos" | grep -E "scene_category|content_category"
    Expected Result: 字段存在
  ```

- [x] 1.2. **Add classification metadata fields** - 添加分类状态字段

  **What to do**:
  - 添加字段: `classification_status TEXT DEFAULT 'pending'`
  - 添加字段: `classification_dirty INTEGER DEFAULT 0`
  - 添加字段: `classified_at TIMESTAMP`
  - 定义状态枚举: pending, processing, completed, failed

  **Acceptance Criteria**:
  - [ ] status 字段存在且默认值为 'pending'
  - [ ] dirty 标记字段存在

  **QA Scenarios**:
  ```
  Scenario: 状态字段验证
    Tool: Bash
    Steps: sqlite3 database/photo_metadata.db "SELECT classification_status FROM photos LIMIT 1"
    Expected Result: 返回 pending 或其他状态值
  ```

- [x] 1.3. **Create photo_edits table** - 创建编辑参数表

  **What to do**:
  - 表结构: `id, photo_id, edit_params (JSON), created_at, updated_at`
  - 添加索引: `photo_id` 索引
  - 添加外键: `photo_id` REFERENCES photos(id)

  **Acceptance Criteria**:
  - [ ] photo_edits 表创建成功
  - [ ] 外键约束正确

  **QA Scenarios**:
  ```
  Scenario: photo_edits 表验证
    Tool: Bash
    Steps: sqlite3 database/photo_metadata.db ".schema photo_edits"
    Expected Result: 包含所有必需字段
  ```

- [x] 1.4. **Create photo_thumbnails table** - 创建缩略图表

  **What to do**:
  - 表结构: `id, photo_id, thumbnail_path, size_type, created_at`
  - size_type 枚举: 'list' (150x150), 'detail' (400x400)
  - 添加索引: `(photo_id, size_type)` 复合索引

  **Acceptance Criteria**:
  - [ ] photo_thumbnails 表创建成功
  - [ ] 复合索引存在

  **QA Scenarios**:
  ```
  Scenario: photo_thumbnails 表验证
    Tool: Bash
    Steps: sqlite3 database/photo_metadata.db ".indices photo_thumbnails"
    Expected Result: 包含 photo_id_size_type 索引
  ```

- [x] 1.5. **Create classification_models table** - 创建模型管理表

  **What to do**:
  - 表结构: `id, model_name, model_path, model_type, is_active, created_at`
  - model_type 枚举: 'scene', 'content'
  - 添加索引: `model_type`, `is_active`

  **Acceptance Criteria**:
  - [ ] classification_models 表创建成功
  - [ ] 能插入/查询模型记录

  **QA Scenarios**:
  ```
  Scenario: classification_models 表验证
    Tool: Bash
    Steps: sqlite3 database/photo_metadata.db "INSERT INTO classification_models VALUES (1, 'scene', 'models/scene.onnx', 'scene', 1, datetime('now')); SELECT * FROM classification_models;"
    Expected Result: 记录插入成功
  ```

- [x] 1.6. **Add ONNXRuntime to CMake** - CMake 添加 ONNX 依赖

  **What to do**:
  - 修改 `CMakeLists.txt`: 添加 `find_package(ONNXRuntime)`
  - 添加编译选项: `option(ENABLE_ONNX "Enable ONNX Runtime" ON)`
  - 添加头文件路径: `${ONNX_INCLUDE_DIRS}`
  - 添加库链接: `${ONNX_LIBRARIES}`

  **Acceptance Criteria**:
  - [ ] `cmake -B build` 成功
  - [ ] 找到 onnxruntime 头文件

  **QA Scenarios**:
  ```
  Scenario: ONNX 依赖验证
    Tool: Bash
    Steps: cmake -B build 2>&1 | grep -i onnx
    Expected Result: 找到 ONNX 或配置跳过
  ```

- [x] 1.7. **Integrate stb_image library** - 集成 stb_image

  **What to do**:
  - ���加 `stb_image.h`, `stb_image_write.h` 到 `src/image/`
  - 创建 `ImageLoader` 类封装读取/写入
  - 支持格式: JPG, PNG, WebP, BMP
  - 添加 `STB_IMAGE_IMPLEMENTATION` 宏定义

  **Acceptance Criteria**:
  - [ ] 编译通过
  - [ ] 能读取/保存图片

  **QA Scenarios**:
  ```
  Scenario: stb_image 集成验证
    Tool: Bash
    Steps: cmake --build build 2>&1 | grep -i stb
    Expected Result: 无 stb 相关错误
  ```

- [x] 1.8. **Setup Google Test framework** - 配置测试框架

  **What to do**:
  - 在 `CMakeLists.txt` 添加 gtest 配置
  - 创建 `tests/classifier/` 目录
  - 创建 `tests/service/` 目录
  - 添加测试编译目标

  **Acceptance Criteria**:
  - [ ] gtest 编译通过
  - [ ] 空测试可运行

  **QA Scenarios**:
  ```
  Scenario: gtest 配置验证
    Tool: Bash
    Steps: cmake -B build -DBUILD_TESTS=ON && cmake --build build
    Expected Result: 测试可执行文件生成
  ```

### Wave 2: Core Services (10 子任务)

- [x] 2.1. **Define ImageClassifier abstract class** - 定义分类器抽象类

  **What to do**:
  - 创建 `src/classifier/ImageClassifier.hpp`
  - 纯虚函数: `virtual ClassificationResult classify(const ImageData& image) = 0`
  - 虚函数: `virtual bool isModelLoaded() const`
  - 虚函数: `virtual void unloadModel()`
  - 虚析构函数

  **Acceptance Criteria**:
  - [ ] 抽象类定义完整
  - [ ] 编译通过

  **QA Scenarios**:
  ```
  Scenario: 抽象类定义验证
    Tool: Read
    Steps: 检查 ImageClassifier.hpp 是否包含纯虚函数
    Expected Result: 包含 classify = 0
  ```

- [x] 2.2. **Define ClassificationResult struct** - 结果数据结构

  **What to do**:
  - 定义 `ClassificationResult` 结构体:
    - `std::string category`
    - `float confidence` (0.0 - 1.0)
    - `std::vector<float> allProbabilities`
    - `std::string modelName`
    - `timestamp`
  - 添加 `toJson()` 方法

  **Acceptance Criteria**:
  - [ ] 结构体完整
  - [ ] JSON 序列化正常

  **QA Scenarios**:
  ```
  Scenario: ClassificationResult 验证
    Tool: Read
    Steps: 检查结构体字段完整
    Expected Result: 包含所有必需字段
  ```

- [x] 2.3. **Implement ClassifierManager** - 多分类器管理

  **What to do**:
  - 创建 `src/classifier/ClassifierManager.hpp/cpp`
  - 方法: `registerClassifier(type, classifier)`
  - 方法: `getClassifier(type)`
  - 方法: `classifyAll(image)` - 顺序执行所有分类器
  - 支持配置: 同步/异步模式

  **Acceptance Criteria**:
  - [ ] 能注册多个分类器
  - [ ] 能获取指定类型分类器

  **QA Scenarios**:
  ```
  Scenario: ClassifierManager 验证
    Tool: Read
    Steps: 检查 registerClassifier 和 getClassifier 方法
    Expected Result: 方法存在
  ```

- [x] 2.4. **Implement SceneClassifier ONNX** - 场景分类器实现

  **What to do**:
  - 创建 `src/classifier/SceneClassifier.hpp/cpp`
  - 继承 `ImageClassifier`
  - 实现: `Ort::Session` 加载模型
  - 实现: 输入 tensor 创建
  - 实现: 输出解析 - 场景分类 (会议/采访/活动/人物/风景/文档/其他)
  - 模型路径: `config.scene_model_path`

  **Acceptance Criteria**:
  - [ ] 能加载 ONNX 模型
  - [ ] 能返回分类结果

  **QA Scenarios**:
  ```
  Scenario: SceneClassifier 分类验证
    Tool: Bash
    Steps: 编译检查
    Expected Result: 编译通过
  ```

- [x] 2.5. **Implement ContentClassifier ONNX** - 内容分类器实现

  **What to do**:
  - 创建 `src/classifier/ContentClassifier.hpp/cpp`
  - 继承 `ImageClassifier`
  - 内容分类: 人物/风景/建筑/产品/文档/其他
  - 类似 SceneClassifier 结构

  **Acceptance Criteria**:
  - [ ] 同 SceneClassifier
  - [ ] 编译通过

  **QA Scenarios**:
  ```
  Scenario: ContentClassifier 分类验证
    Tool: Bash
    Steps: 编译检查
    Expected Result: 编译通过
  ```

- [ ] 2.6. **Implement model lazy loading** - 模型懒加载

  **What to do**:
  - 首次调用 classify 时加载模型
  - 内存压力时卸载 (可选)
  - 显式 `reloadModel()` 方法
  - 配置: `lazy_load_enabled`

  **Acceptance Criteria**:
  - [ ] 启动时不加载模型
  - [ ] 首次分类时自动加载

  **QA Scenarios**:
  ```
  Scenario: 懒加载验证
    Tool: 代码审查
    Steps: 检查首次 classify 前模型未加载
    Expected Result: isModelLoaded() = false 在首次调用前
  ```

- [x] 2.7. **Implement ThumbnailGenerator LIST** - 列表缩略图

  **What to do**:
  - 创建 `src/service/ThumbnailGenerator.hpp/cpp`
  - LIST 尺寸: 150x150 (等比缩放)
  - 使用 stb_image_resize
  - 存储路径: `res/Thumbnail/{photo_id}_list.jpg`
  - 返回: thumbnail 记录 ID

  **Acceptance Criteria**:
  - [ ] 生成 150x150 缩略图
  - [ ] 文件保存成功

  **QA Scenarios**:
  ```
  Scenario: LIST 缩略图生成验证
    Tool: Bash
    Steps: 生成缩略图，检查尺寸
    Expected Result: 150x150
  ```

- [x] 2.8. **Implement ThumbnailGenerator DETAIL** - 详情缩略图

  **What to do**:
  - DETAIL 尺寸: 400x400 (等比缩放)
  - 存储路径: `res/Thumbnail/{photo_id}_detail.jpg`
  - 支持质量参数: 80% JPEG 质量

  **Acceptance Criteria**:
  - [ ] 生成 400x400 缩略图
  - [ ] 两种尺寸可同时生成

  **QA Scenarios**:
  ```
  Scenario: DETAIL 缩略图生成验证
    Tool: Bash
    Steps: 生成缩略图，检查尺寸
    Expected Result: 400x400
  ```

- [x] 2.9. **Implement ImageEditor basics** - 基本编辑功能

  **What to do**:
  - 创建 `src/service/ImageEditor.hpp/cpp`
  - 裁剪: `crop(int x, int y, int w, int h)`
  - 旋转: `rotate90cw()`, `rotate90ccw()`, `rotate180()`
  - 缩放: `resize(int width, int height)`
  - 非破坏性: 返回编辑参数，不修改原图

  **Acceptance Criteria**:
  - [ ] 裁剪正确
  - [ ] 旋转正确 (90/180/270)
  - [ ] 缩放正确

  **QA Scenarios**:
  ```
  Scenario: 裁剪功能验证
    Tool: 代码审查
    Steps: 检查 crop 实现
    Expected Result: 坐标和尺寸正确
  ```

- [x] 2.10. **Implement ImageEditor enhanced** - 增强编辑

  **What to do**:
  - 亮度: `adjustBrightness(int delta)` (-20 到 +20)
  - 对比度: `adjustContrast(float factor)` (0.5 到 2.0)
  - 滤镜: `applyGrayscale()`, `applySepia()`, `applyVintage()`
  - 链式调用支持

  **Acceptance Criteria**:
  - [ ] 亮度调整 -20 到 +20 正常
  - [ ] 对比度 0.5 到 2.0 正常
  - [ ] 滤镜效果正确

  **QA Scenarios**:
  ```
  Scenario: 亮度调整验证
    Tool: 代码审查
    Steps: 检查 adjustBrightness 实现
    Expected Result: 值在 -20 到 +20 范围内
  ```

### Wave 3: API (9 子任务)

- [x] 3.1. **GET /photos/{id}/classification** - 获取分类结果 API

  **What to do**:
  - 端点: `GET /api/photos/{id}/classification`
  - 返回: `{ photo_id, scene_category, content_category, confidence, classified_at }`
  - 未分类: 返回 `{ status: "pending" }`
  - 错误: 404 如果 photo 不存在

  **Acceptance Criteria**:
  - [ ] 返回分类结果 JSON
  - [ ] 404 处理正确

  **QA Scenarios**:
  ```
  Scenario: 获取分类结果
    Tool: Bash
    Steps: curl http://localhost:8080/api/photos/1/classification
    Expected Result: 返回 JSON 包含 category 字段
  ```

- [x] 3.2. **POST /photos/{id}/classify** - 手动触发分类 API

  **What to do**:
  - 端点: `POST /api/photos/{id}/classify`
  - BODY: 可选 `{ force: true }` 强制重新分类
  - 调用: SceneClassifier + ContentClassifier
  - 返回: 分类结果 JSON

  **Acceptance Criteria**:
  - [ ] 触发分类并返回结果
  - [ ] force 参数有效

  **QA Scenarios**:
  ```
  Scenario: 手动触发分类
    Tool: Bash
    Steps: curl -X POST http://localhost:8080/api/photos/1/classify
    Expected Result: 返回分类结果
  ```

- [x] 3.3. **POST /photos/{id}/edit** - 应用编辑 API

  **What to do**:
  - 端点: `POST /api/photos/{id}/edit`
  - BODY: `{ crop: {}, rotate: {}, brightness: {}, filter: {} }` (JSON)
  - 存储: 编辑参数到 photo_edits 表
  - 返回: 保存的编辑参数

  **Acceptance Criteria**:
  - [ ] 编辑参数保存到数据库
  - [ ] 返回保存结果

  **QA Scenarios**:
  ```
  Scenario: 应用编辑
    Tool: Bash
    Steps: curl -X POST -d '{"brightness": 10}' http://localhost:8080/api/photos/1/edit
    Expected Result: 保存成功
  ```

- [x] 3.4. **GET /photos/{id}/edit** - 获取编辑参数 API

  **What to do**:
  - 端点: `GET /api/photos/{id}/edit`
  - 返回: photo_edits 表中最新编辑参数
  - 无编辑: 返回 `{ edit_params: {} }`

  **Acceptance Criteria**:
  - [ ] 返回编辑参数 JSON
  - [ ] 空编辑返回空对象

  **QA Scenarios**:
  ```
  Scenario: 获取编辑参数
    Tool: Bash
    Steps: curl http://localhost:8080/api/photos/1/edit
    Expected Result: 返回 JSON
  ```

- [x] 3.5. **POST /photos/{id}/revert** - 撤销编辑 API

  **What to do**:
  - 端点: `POST /api/photos/{id}/revert`
  - 删除: photo_edits 中对应记录
  - 返回: `{ success: true }`

  **Acceptance Criteria**:
  - [ ] 编辑记录删除
  - [ ] 返回确认

  **QA Scenarios**:
  ```
  Scenario: 撤销编辑
    Tool: Bash
    Steps: curl -X POST http://localhost:8080/api/photos/1/revert
    Expected Result: success: true
  ```

- [x] 3.6. **GET /photos/{id}/thumbnail** - 获取缩略图 API

  **What to do**:
  - 端点: `GET /api/photos/{id}/thumbnail?size=list|detail`
  - 默认: size=list
  - 生成: 如果不存在则自动生成
  - 返回: 缩略图文件 (JPEG)

  **Acceptance Criteria**:
  - [ ] 返回缩略图文件
  - [ ] size 参数有效

  **QA Scenarios**:
  ```
  Scenario: 获取缩略图
    Tool: Bash
    Steps: curl "http://localhost:8080/api/photos/1/thumbnail?size=detail" -o thumb.jpg
    Expected Result: 文件存在且为 JPEG
  ```

- [x] 3.7. **Integrate auto-classification on upload** - 上传时自动分类

  **What to do**:
  - 修改 `uploadPhoto` 端点
  - 上传成功后调用 ClassifierManager::classifyAll
  - 更新 photos 表: category, status
  - 可配置: 同步/异步

  **Acceptance Criteria**:
  - [ ] 上传后自动分类
  - [ ] 状态更新正确

  **QA Scenarios**:
  ```
  Scenario: 上传自动分类
    Tool: Bash
    Steps: 上传图片后查询 classification_status
    Expected Result: 不为 pending
  ```

- [x] 3.8. **Integrate auto-thumbnail on upload** - 上传时自动生成缩略图

  **What to do**:
  - uploadPhoto 成功后调用 ThumbnailGenerator
  - 生成 LIST 和 DETAIL 两种尺寸
  - 更新 photo_thumbnails 表

  **Acceptance Criteria**:
  - [ ] 上传后自动生成缩略图
  - [ ] 两张缩略图存在

  **QA Scenarios**:
  ```
  Scenario: 上传自动缩略图
    Tool: Bash
    Steps: 上传图片后查询 photo_thumbnails
    Expected Result: 2 条记录
  ```

- [x] 3.9. **Add DB migration handler** - 数据库迁移处理

  **What to do**:
  - 创建 `MigrationManager` 类
  - 版本号: schema_version 字段
  - 迁移: 从 1.0 到 1.1
  - 回滚: 不支持 (记录在注释)

  **Acceptance Criteria**:
  - [ ] 迁移执行成功
  - [ ] 版本号更新

  **QA Scenarios**:
  ```
  Scenario: 数据库迁移
    Tool: Bash
    Steps: 检查 schema_version
    Expected Result: 1.1
  ```

### Wave 4: Tests (4 子任务)

- [x] 4.1. **ImageClassifier unit tests** - 分类器单元测试

  **What to do**:
  - `tests/classifier/ImageClassifierTest.cpp`
  - Mock Ort::Session
  - 测试: classify() 方法
  - 测试: 懒加载机制
  - 测试: 错误处理

  **Acceptance Criteria**:
  - [ ] 基础分类测试通过
  - [ ] Mock 测试通过

  **QA Scenarios**:
  ```
  Scenario: 分类器单元测试
    Tool: Bash
    Steps: ./build/tests/image_classifier_test
    Expected Result: All tests passed
  ```

- [x] 4.2. **ImageEditor unit tests** - 编辑器单元测试

  **What to do**:
  - `tests/service/ImageEditorTest.cpp`
  - 测试: crop 裁剪
  - 测试: rotate 旋转
  - 测试: resize 缩放
  - 测试: brightness/contrast
  - 测试: filters

  **Acceptance Criteria**:
  - [ ] 所有编辑测试通过

  **QA Scenarios**:
  ```
  Scenario: 编辑器单元测试
    Tool: Bash
    Steps: ./build/tests/image_editor_test
    Expected Result: All tests passed
  ```

- [x] 4.3. **API integration tests** - API 集成测试

  **What to do**:
  - `tests/api/PhotoApiTest.cpp`
  - 测试: 分类端点 (Get/Post)
  - 测试: 编辑端点 (Get/Post/Revert)
  - 测试: 缩略图端点
  - 测试: 错误处理

  **Acceptance Criteria**:
  - [ ] 所有 API 测试通过

  **QA Scenarios**:
  ```
  Scenario: API 集成测试
    Tool: Bash
    Steps: ./build/tests/photo_api_test
    Expected Result: All tests passed
  ```

- [x] 4.4. **End-to-end workflow tests** - 端到端测试

  **What to do**:
  - `tests/e2e/WorkflowTest.cpp`
  - 流程: 上传 → 自动分类 → 编辑 → 获取 → 撤销
  - 验证: 每个步骤状态正确
  - 验证: 数据库记录正确

  **Acceptance Criteria**:
  - [ ] 完整流程测试通过

  **QA Scenarios**:
  ```
  Scenario: 端到端测试
    Tool: Bash
    Steps: ./build/tests/workflow_test
    Expected Result: All tests passed
  ```

## Final Verification Wave (MANDATORY)

- [x] F1. **Plan Compliance Audit** - 验证所有 31 个子任务完成度
- [x] F2. **Code Quality Review** - 代码质量检查
- [x] F3. **Integration Tests** - 集成测试运行
- [x] F4. **Scope Fidelity Check** - 范围偏差检查

---

## Commit Strategy (31 个子任务)

### Wave 1: Foundation (8 commits)
- **1.1**: `db: extend photos table - add scene_category, content_category`
- **1.2**: `db: add classification status fields - status, dirty, classified_at`
- **1.3**: `db: create photo_edits table`
- **1.4**: `db: create photo_thumbnails table`
- **1.5**: `db: create classification_models table`
- **1.6**: `deps: add onnxruntime to CMakeLists.txt`
- **1.7**: `deps: integrate stb_image library`
- **1.8**: `test: setup google test framework`

### Wave 2: Core Services (10 commits)
- **2.1**: `classifier: define ImageClassifier abstract class`
- **2.2**: `classifier: define ClassificationResult struct`
- **2.3**: `classifier: implement ClassifierManager`
- **2.4**: `classifier: implement SceneClassifier with ONNX`
- **2.5**: `classifier: implement ContentClassifier with ONNX`
- **2.6**: `classifier: implement lazy loading mechanism`
- **2.7**: `service: implement ThumbnailGenerator LIST size`
- **2.8**: `service: implement ThumbnailGenerator DETAIL size`
- **2.9**: `service: implement ImageEditor basic (crop/rotate/resize)`
- **2.10**: `service: implement ImageEditor enhanced (brightness/contrast/filter)`

### Wave 3: API (9 commits)
- **3.1**: `api: GET /photos/{id}/classification endpoint`
- **3.2**: `api: POST /photos/{id}/classify endpoint`
- **3.3**: `api: POST /photos/{id}/edit endpoint`
- **3.4**: `api: GET /photos/{id}/edit endpoint`
- **3.5**: `api: POST /photos/{id}/revert endpoint`
- **3.6**: `api: GET /photos/{id}/thumbnail endpoint`
- **3.7**: `api: integrate auto-classification on upload`
- **3.8**: `api: integrate auto-thumbnail on upload`
- **3.9**: `db: add migration handler`

### Wave 4: Tests (4 commits)
- **4.1**: `test: ImageClassifier unit tests`
- **4.2**: `test: ImageEditor unit tests`
- **4.3**: `test: API integration tests`
- **4.4**: `test: End-to-end workflow tests`

---

## Success Criteria

### Verification Commands
```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Final Checklist
- [ ] All classification endpoints return correct JSON
- [ ] All edit endpoints preserve original image
- [ ] Thumbnails generated at correct sizes
- [ ] Unit tests pass
- [ ] Integration tests pass