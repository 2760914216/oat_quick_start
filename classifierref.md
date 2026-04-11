# Image Classification Backend - 图像分类模块参考文档

基于 oatpp 1.4.0 + ONNX Runtime 的图像分类后端服务，为新闻工作者提供图片自动分类能力。

---

## 一、模块概述

### 1.1 功能特性

| 特性 | 说明 |
|------|------|
| 场景分类 | 会议、采访、活动、人物、风景、文档、其他 (7类) |
| 内容分类 | 人物、风景、建筑、产品、文档、其他 (6类) |
| 模型懒加载 | 首次分类时才加载 ONNX 模型 |
| 优雅降级 | ONNX 不可用时返回默认分类结果 |
| 自动分类 | 上传照片时自动触发分类 |

### 1.2 目录结构

```
src/classifier/
├── ImageClassifier.hpp      # 分类器抽象基类
├── ClassificationResult.hpp # 分类结果数据结构
├── ClassifierManager.hpp    # 分类器管理器 (单例)
├── ClassifierManager.cpp
├── SceneClassifier.hpp      # 场景分类器实现
├── SceneClassifier.cpp
├── ContentClassifier.hpp    # 内容分类器实现
└── ContentClassifier.cpp
```

---

## 二、核心组件

### 2.1 ImageClassifier 抽象基类

```cpp
class ImageClassifier {
public:
    virtual ~ImageClassifier() = default;
    
    // 执行图像分类
    virtual ClassificationResult classify(
        const std::vector<unsigned char>& imageData
    ) = 0;
    
    // 检查模型是否已加载
    virtual bool isModelLoaded() const = 0;
    
    // 卸载模型释放内存
    virtual void unloadModel() = 0;
    
    // 获取模型类型
    virtual std::string getModelType() const = 0;
};
```

### 2.2 ClassificationResult 数据结构

```cpp
struct ClassificationResult {
    std::string category;           // 分类结果类别
    float confidence = 0.0f;       // 置信度 (0.0-1.0)
    std::vector<float> allProbabilities;  // 所有类别的概率
    std::string modelName;         // 模型名称
    std::string timestamp;          // 分类时间
    
    // 转换为 JSON
    std::string toJson() const;
};
```

### 2.3 ClassifierManager 分类器管理器

单例模式管理多个分类器：

```cpp
class ClassifierManager {
public:
    static ClassifierManager& getInstance();
    
    // 注册分类器
    void registerClassifier(
        const std::string& type, 
        std::shared_ptr<ImageClassifier> classifier
    );
    
    // 获取分类器
    std::shared_ptr<ImageClassifier> getClassifier(
        const std::string& type
    );
    
    // 对图像执行所有已注册分类器的分类
    std::vector<ClassificationResult> classifyAll(
        const std::vector<unsigned char>& imageData
    );
    
    // 设置异步模式
    void setAsyncMode(bool async);
    bool isAsyncMode() const;
};
```

### 2.4 SceneClassifier 场景分类器

ONNX 模型实现，支持 7 种场景分类：

| 类别 | 中文 |
|------|------|
| 会议 | 会议、研讨会 |
| 采访 | 采访、访谈 |
| 活动 | 活动、典礼 |
| 人物 | 人物照片 |
| 风景 | 风景、自然 |
| 文档 | 文档、文件 |
| 其他 | 其他 |

### 2.5 ContentClassifier 内容分类器

ONNX 模型实现，支持 6 种内容分类：

| 类别 | 中文 |
|------|------|
| 人物 | 人物肖像 |
| 风景 | 自然风景 |
| 建筑 | 建筑景观 |
| 产品 | 商品物品 |
| 文档 | 文档资料 |
| 其他 | 其他 |

---

## 三、懒加载机制

### 3.1 工作原理

1. **启动时**：不加载任何 ONNX 模型，节省内存
2. **首次分类时**：自动调用 `loadModel()` 加载模型
3. **模型状态**：通过 `isModelLoaded()` 查询模型加载状态

### 3.2 代码示例

```cpp
auto sceneClassifier = std::make_shared<SceneClassifier>("models/scene.onnx");

// 模型未加载
if (!sceneClassifier->isModelLoaded()) {
    std::cout << "Model not loaded yet" << std::endl;
}

// 首次分类时自动加载模型
auto result = sceneClassifier->classify(imageData);

// 模型已加载
if (sceneClassifier->isModelLoaded()) {
    std::cout << "Model loaded, category: " << result.category << std::endl;
}

// 手动卸载模型
sceneClassifier->unloadModel();
```

---

## 四、ONNX 集成

### 4.1 CMake 配置

```cmake
option(ENABLE_ONNX "Enable ONNX Runtime" ON)

if(ENABLE_ONNX)
    find_package(ONNXRuntime)
    target_include_directories(oat_quick_start PRIVATE ${ONNX_INCLUDE_DIRS})
    target_link_libraries(oat_quick_start PRIVATE ${ONNX_LIBRARIES})
endif()
```

### 4.2 优雅降级

当 ONNX Runtime 不可用时：

```cpp
ClassificationResult SceneClassifier::classify(
    const std::vector<unsigned char>& imageData
) {
#if ENABLE_ONNX
    return runInference(imageData);
#else
    // 返回默认结果
    ClassificationResult result;
    result.category = "其他";
    result.confidence = 0.0f;
    return result;
#endif
}
```

---

## 五、数据库 Schema

### 5.1 photos 表扩展

```sql
ALTER TABLE photos ADD COLUMN scene_category TEXT;
ALTER TABLE photos ADD COLUMN content_category TEXT;
ALTER TABLE photos ADD COLUMN classification_status TEXT DEFAULT 'pending';
ALTER TABLE photos ADD COLUMN classification_dirty INTEGER DEFAULT 0;
ALTER TABLE photos ADD COLUMN classified_at TEXT;
ALTER TABLE photos ADD COLUMN schema_version TEXT DEFAULT '1.1';
```

### 5.2 classification_models 表

```sql
CREATE TABLE classification_models (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    model_name TEXT NOT NULL,
    model_path TEXT NOT NULL,
    model_type TEXT NOT NULL,  -- 'scene' 或 'content'
    is_active INTEGER DEFAULT 1,
    created_at TEXT NOT NULL
);
```

---

## 六、API 端点

### 6.1 获取分类结果

| 属性 | 值 |
|------|-----|
| HTTP 方法 | GET |
| 路由 | `/api/photos/{id}/classification` |
| 认证 | Bearer Token |
| 响应 | `{"photo_id": "...", "scene_category": "...", "content_category": "...", "confidence": 0.95, "classified_at": "..."}` |

### 6.2 手动触发分类

| 属性 | 值 |
|------|-----|
| HTTP 方法 | POST |
| 路由 | `/api/photos/{id}/classify` |
| 认证 | Bearer Token |
| 请求体 | `{"force": true}` (可选) |
| 响应 | 分类结果 JSON |

### 6.3 上传时自动分类

上传照片自动触发分类流程：

```
上传照片 → 保存文件 → 保存元数据 → 加载图像 → 执行分类 → 更新数据库
```

---

## 七、测试

### 7.1 单元测试

```bash
# 运行分类器测试
./build/image_classifier_test

# 测试覆盖
- ClassificationResult 结构测试
- ClassifierManager 单例测试
- 懒加载机制测试
- 错误处理测试
```

### 7.2 测试用例

| 测试组 | 测试数 | 覆盖内容 |
|--------|--------|----------|
| ClassificationResultTest | 5 | 结构体、toJson |
| ClassifierManagerTest | 7 | 单例、注册、获取 |
| LazyLoadingTest | 6 | 模型加载状态 |
| ErrorHandlingTest | 4 | 降级处理 |
| ImageClassifierInterfaceTest | 3 | 多态接口 |

---

## 八、配置

### 8.1 模型路径配置

在 `openspec/config.yaml` 中配置：

```yaml
classifier:
  scene_model_path: "models/scene_classifier.onnx"
  content_model_path: "models/content_classifier.onnx"
  lazy_load_enabled: true
  async_mode: false
```

### 8.2 图像预处理

模型输入要求：
- 尺寸: 224x224
- 通道: RGB (3通道)
- 格式: float32, 归一化到 [0, 1]