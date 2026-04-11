# Image Classification & Editing Backend - 项目总览

基于 oatpp 1.4.0 + ONNX Runtime 的图像分类与轻量编辑后端服务，为新闻工作者提供图片分类、缩略图生成和图像编辑功能。

---

## 一、项目架构

### 1.1 技术栈

| 组件 | 技术 | 版本 |
|------|------|------|
| Web 框架 | OATPP | 1.4.0 |
| 数据库 | SQLite | 3.x |
| 图像处理 | stb_image | - |
| ML 推理 | ONNX Runtime | 1.x (可选) |
| 认证 | JWT | - |

### 1.2 模块结构

```
src/
├── classifier/           # 图像分类模块
│   ├── ImageClassifier.hpp
│   ├── ClassificationResult.hpp
│   ├── ClassifierManager.hpp/cpp
│   ├── SceneClassifier.hpp/cpp
│   └── ContentClassifier.hpp/cpp
│
├── service/             # 业务服务模块
│   ├── ThumbnailGenerator.hpp/cpp
│   └── ImageEditor.hpp/cpp
│
├── image/               # 图像处理模块
│   ├── ImageLoader.hpp/cpp
│   ├── stb_image.h
│   └── stb_image_write.h
│
├── db/                  # 数据库模块
│   ├── PhotoMetadataDb.hpp
│   └── PhotoMetadataDb.cpp
│
├── controller/          # API 控制器
│   └── PhotoController.hpp
│
└── dto/                 # 数据传输对象
    └── PhotoDTOs.hpp
```

---

## 二、功能模块

### 2.1 图像分类模块 (Classifier)

**功能**: 自动识别图片场景和内容

| 分类类型 | 类别数 | 类别 |
|----------|--------|------|
| 场景分类 | 7 | 会议, 采访, 活动, 人物, 风景, 文档, 其他 |
| 内容分类 | 6 | 人物, 风景, 建筑, 产品, 文档, 其他 |

**核心组件**:
- `ImageClassifier` - 分类器抽象基类
- `ClassificationResult` - 分类结果数据结构
- `ClassifierManager` - 分类器管理器 (单例)
- `SceneClassifier` - 场景分类器 (ONNX)
- `ContentClassifier` - 内容分类器 (ONNX)

**特性**:
- ONNX 模型懒加载
- 优雅降级 (ONNX 不可用时返回默认结果)
- 自动分类 (上传时触发)

---

### 2.2 缩略图生成模块 (ThumbnailGenerator)

**功能**: 生成多种尺寸的图像缩略图

| 尺寸类型 | 尺寸 | 用途 |
|----------|------|------|
| LIST | 150x150 | 照片列表预览 |
| DETAIL | 400x400 | 照片详情页预览 |

**核心方法**:
```cpp
static std::optional<std::string> generateThumbnail(
    const std::string& photoId,
    const std::vector<unsigned char>& imageData,
    const std::string& sizeType  // "list" 或 "detail"
);
```

**特性**:
- 等比缩放，保持原始宽高比
- JPEG 80% 质量压缩
- 自动生成 (上传时)
- 按需生成 (API 调用时)

---

### 2.3 图像编辑模块 (ImageEditor)

**功能**: 提供非破坏性图像编辑能力

| 功能 | 参数范围 | 说明 |
|------|----------|------|
| 裁剪 | 任意有效坐标 | 指定区域裁剪 |
| 旋转 | 0/90/180/270 | 90° 旋转 |
| 缩放 | 任意尺寸 | 等比缩放 |
| 亮度 | -20 ~ +20 | 调整亮度 |
| 对比度 | 0.5 ~ 2.0 | 调整对比度 |
| 滤镜 | 0-3 | 灰度/复古/怀旧 |

**核心数据结构**:
```cpp
struct EditParams {
    int cropX, cropY, cropWidth, cropHeight;
    int rotation;
    int targetWidth, targetHeight;
    int brightness;
    float contrast;
    int filter;
};
```

**特性**:
- 非破坏性编辑 (存储参数，不修改原图)
- JSON 序列化/反序列化
- 链式操作支持

---

### 2.4 图像加载模块 (ImageLoader)

**功能**: 图像解码和编码

**支持格式**:
- 解码: JPEG, PNG, WebP, BMP, GIF, TIFF
- 编码: JPEG, PNG

**核心方法**:
```cpp
// 加载图像
static std::optional<ImageData> load(const std::string& filepath);

// 保存 JPEG
static bool saveAsJpeg(const std::string& filepath, 
    const std::vector<unsigned char>& data, int w, int h, int c, int quality);
```

---

### 2.5 数据库模块 (PhotoMetadataDb)

**功能**: SQLite 数据库封装

**核心表**:

| 表名 | 用途 |
|------|------|
| photos | 照片元数据 (含分类字段) |
| photo_edits | 编辑参数存储 |
| photo_thumbnails | 缩略图路径存储 |
| classification_models | ONNX 模型信息 |

---

## 三、API 端点

### 3.1 照片管理 (已有)

| 方法 | 路由 | 功能 |
|------|------|------|
| GET | `/api/photos` | 获取照片列表 |
| POST | `/api/photos/upload` | 上传照片 |
| GET | `/api/photos/{id}` | 获取照片详情 |
| GET | `/api/photos/{id}/file` | 下载照片文件 |
| DELETE | `/api/photos/{id}` | 删除照片 |

### 3.2 分类功能 (新增)

| 方法 | 路由 | 功能 |
|------|------|------|
| GET | `/api/photos/{id}/classification` | 获取分类结果 |
| POST | `/api/photos/{id}/classify` | 手动触发分类 |

### 3.3 编辑功能 (新增)

| 方法 | 路由 | 功能 |
|------|------|------|
| POST | `/api/photos/{id}/edit` | 应用编辑 |
| GET | `/api/photos/{id}/edit` | 获取编辑参数 |
| POST | `/api/photos/{id}/revert` | 撤销编辑 |

### 3.4 缩略图功能 (新增)

| 方法 | 路由 | 功能 |
|------|------|------|
| GET | `/api/photos/{id}/thumbnail` | 获取缩略图 |

---

## 四、数据库 Schema

### 4.1 photos 表 (扩展)

```sql
ALTER TABLE photos ADD COLUMN scene_category TEXT;
ALTER TABLE photos ADD COLUMN content_category TEXT;
ALTER TABLE photos ADD COLUMN classification_status TEXT DEFAULT 'pending';
ALTER TABLE photos ADD COLUMN classification_dirty INTEGER DEFAULT 0;
ALTER TABLE photos ADD COLUMN classified_at TEXT;
ALTER TABLE photos ADD COLUMN schema_version TEXT DEFAULT '1.1';
```

### 4.2 photo_edits 表

```sql
CREATE TABLE photo_edits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    photo_id TEXT NOT NULL,
    edit_params TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
);
```

### 4.3 photo_thumbnails 表

```sql
CREATE TABLE photo_thumbnails (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    photo_id TEXT NOT NULL,
    thumbnail_path TEXT NOT NULL,
    size_type TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
);
```

### 4.4 classification_models 表

```sql
CREATE TABLE classification_models (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    model_name TEXT NOT NULL,
    model_path TEXT NOT NULL,
    model_type TEXT NOT NULL,
    is_active INTEGER DEFAULT 1,
    created_at TEXT NOT NULL
);
```

---

## 五、工作流程

### 5.1 照片上传流程

```
1. 客户端 POST /api/photos/upload (带图片数据)
2. 服务器验证 JWT
3. 保存图片到 res/Photo/
4. 插入照片元数据到数据库
5. 自动执行分类 (Task 3.7)
6. 自动生成缩略图 (Task 3.8)
7. 返回上传结果
```

### 5.2 分类流程

```
1. 上传时自动触发 或 手动 POST /api/photos/{id}/classify
2. 加载图片数据
3. 更新分类状态为 'processing'
4. 加载 ONNX 模型 (懒加载)
5. 执行推理
6. 更新分类结果到数据库
7. 更新分类状态为 'completed'
```

### 5.3 编辑流程

```
1. 客户端 POST /api/photos/{id}/edit (带编辑参数)
2. 服务器验证 JWT 和照片权限
3. 将编辑参数序列化为 JSON
4. 存储到 photo_edits 表
5. 返回保存结果
```

---

## 六、测试

### 6.1 测试框架

- Google Test (gtest)
- CMake 构建系统集成

### 6.2 测试套件

| 测试 | 文件 | 测试数 |
|------|------|--------|
| 分类器测试 | `tests/classifier/ImageClassifierTest.cpp` | 25 |
| 图像服务测试 | `tests/service/ImageServiceTest.cpp` | 34 |
| API 测试 | `tests/api/PhotoApiTest.cpp` | 59 |
| 工作流测试 | `tests/e2e/WorkflowTest.cpp` | 13 |

### 6.3 运行测试

```bash
# 构建
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# 运行单个测试
./build/image_classifier_test
./build/image_service_test

# 运行所有测试
ctest --test-dir build
```

---

## 七、配置

### 7.1 CMake 配置

```cmake
# ONNX Runtime (可选)
option(ENABLE_ONNX "Enable ONNX Runtime" ON)

# 测试
option(BUILD_TESTS "Build tests" ON)
```

### 7.2 配置文件

在 `openspec/config.yaml` 中配置:

```yaml
classifier:
  scene_model_path: "models/scene_classifier.onnx"
  content_model_path: "models/content_classifier.onnx"
  lazy_load_enabled: true
  async_mode: false
```

---

## 八、错误处理

### 8.1 错误码

| 错误码 | 说明 |
|--------|------|
| 400 | 请求参数错误 |
| 401 | 未认证 |
| 403 | 无权限 |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |

### 8.2 降级策略

- ONNX 不可用时返回默认分类结果
- 缩略图生成失败不影响上传成功
- 编辑失败返回具体错误信息

---

## 九、参考文档

| 文档 | 路径 | 说明 |
|------|------|------|
| API 参考 | `appref.md` | HTTP API 详细文档 |
| 存储参考 | `storageref.md` | 存储架构文档 |
| 分类器文档 | `classifierref.md` | 图像分类模块 |
| 缩略图文档 | `thumbnailref.md` | 缩略图生成模块 |
| 编辑器文档 | `editorref.md` | 图像编辑模块 |
| 图像加载文档 | `imageref.md` | 图像加载模块 |
| 数据库文档 | `databaseref.md` | 数据库 Schema 文档 |

---

## 十、构建与运行

### 10.1 构建

```bash
# 配置
cmake -B build

# 编译
cmake --build build

# 运行测试
ctest --test-dir build
```

### 10.2 运行

```bash
# 启动服务器
./build/oat_quick_start
```

### 10.3 测试 API

```bash
# 登录获取 token
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"123456"}'

# 上传照片 (带 token)
curl -X POST http://localhost:8080/api/photos/upload \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: image/jpeg" \
  --data-binary @photo.jpg
```