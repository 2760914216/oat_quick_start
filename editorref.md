# Image Editor Service - 图像编辑模块参考文档

---

## 一、模块概述

### 1.1 功能特性

| 功能 | 说明 |
|------|------|
| 裁剪 | 指定坐标和尺寸裁剪图像 |
| 旋转 | 支持 90°/180°/270° 旋转 |
| 缩放 | 指定目标尺寸缩放 |
| 亮度调整 | 范围: -20 到 +20 |
| 对比度调整 | 范围: 0.5 到 2.0 |
| 滤镜效果 | 灰度、复古、怀旧 |
| 非破坏性 | 存储编辑参数，不修改原图 |
| JSON 序列化 | 支持编辑参数序列化和反序列化 |

### 1.2 目录结构

```
src/service/
├── ImageEditor.hpp   # 图像编辑器头文件
└── ImageEditor.cpp   # 图像编辑器实现
```

---

## 二、核心组件

### 2.1 EditParams 数据结构

```cpp
struct EditParams {
    // 裁剪参数
    int cropX = 0;
    int cropY = 0;
    int cropWidth = 0;
    int cropHeight = 0;
    
    // 旋转角度 (0, 90, 180, 270)
    int rotation = 0;
    
    // 缩放目标尺寸
    int targetWidth = 0;
    int targetHeight = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    
    // 增强参数
    int brightness = 0;      // -20 到 +20
    float contrast = 1.0f;    // 0.5 到 2.0
    
    // 滤镜 (0=NONE, 1=GRAYSCALE, 2=SEPIA, 3=VINTAGE)
    int filter = 0;
    
    // 辅助方法
    bool hasEdits() const;
    std::string toJson() const;
    static EditParams fromJson(const std::string& json);
};
```

### 2.2 ImageEditor 类

静态方法提供所有编辑功能：

```cpp
class ImageEditor {
public:
    // 禁止实例化
    ImageEditor() = delete;
    
    // ============ 裁剪操作 ============
    
    // 裁剪图像
    static std::optional<std::vector<unsigned char>> crop(
        const std::vector<unsigned char>& imageData,
        int width, int height, int channels,
        int x, int y, int cropW, int cropH
    );
    
    // ============ 旋转操作 ============
    
    // 顺时针旋转 90°
    static std::vector<unsigned char> rotate90cw(
        const std::vector<unsigned char>& data,
        int w, int h, int channels
    );
    
    // 逆时针旋转 90°
    static std::vector<unsigned char> rotate90ccw(
        const std::vector<unsigned char>& data,
        int w, int h, int channels
    );
    
    // 旋转 180°
    static std::vector<unsigned char> rotate180(
        const std::vector<unsigned char>& data,
        int w, int h, int channels
    );
    
    // ============ 缩放操作 ============
    
    // 缩放图像
    static std::optional<std::vector<unsigned char>> resize(
        const std::vector<unsigned char>& data,
        int srcW, int srcH, int channels,
        int targetW, int targetH
    );
    
    // ============ 增强操作 ============
    
    // 调整亮度
    static std::vector<unsigned char> adjustBrightness(
        const std::vector<unsigned char>& data,
        int size, int delta
    );
    
    // 调整对比度
    static std::vector<unsigned char> adjustContrast(
        const std::vector<unsigned char>& data,
        int size, float factor
    );
    
    // ============ 滤镜操作 ============
    
    static std::vector<unsigned char> applyGrayscale(
        const std::vector<unsigned char>& data, int size
    );
    
    static std::vector<unsigned char> applySepia(
        const std::vector<unsigned char>& data, int size
    );
    
    static std::vector<unsigned char> applyVintage(
        const std::vector<unsigned char>& data, int size
    );
    
    // ============ 综合编辑 ============
    
    // 应用编辑参数到图像
    static std::optional<std::vector<unsigned char>> applyEdits(
        const std::vector<unsigned char>& imageData,
        int width, int height, int channels,
        const EditParams& params
    );
};
```

---

## 三、编辑操作详解

### 3.1 裁剪 (Crop)

```cpp
// 裁剪示例
auto result = ImageEditor::crop(
    imageData,    // 原始图像数据
    1920, 1080, 3, // 原始尺寸和通道数
    100, 50,      // 裁剪起点 x, y
    800, 600      // 裁剪宽度和高度
);
```

**参数验证**:
- `x + cropW <= srcW`
- `y + cropH <= srcH`
- 所有参数必须 >= 0

### 3.2 旋转 (Rotate)

```cpp
// 顺时针旋转 90°
auto rotated = ImageEditor::rotate90cw(imageData, w, h, channels);

// 逆时针旋转 90°
auto rotated = ImageEditor::rotate90ccw(imageData, w, h, channels);

// 旋转 180°
auto rotated = ImageEditor::rotate180(imageData, w, h, channels);
```

**注意**: 旋转后宽高会互换 (w↔h)

### 3.3 缩放 (Resize)

```cpp
// 缩放示例
auto resized = ImageEditor::resize(
    imageData,
    1920, 1080, 3,    // 原始尺寸
    800, 600          // 目标尺寸
);
```

**算法**: 双线性插值

### 3.4 亮度调整 (Brightness)

```cpp
// 调整亮度 -20 到 +20
auto brightened = ImageEditor::adjustBrightness(
    imageData,
    dataSize,
    10  // +10 变亮
);

auto darkened = ImageEditor::adjustBrightness(
    imageData,
    dataSize,
    -10  // -10 变暗
);
```

**实现**: `pixel = clamp(pixel + delta, 0, 255)`

### 3.5 对比度调整 (Contrast)

```cpp
// 调整对比度 0.5 到 2.0
auto contrasted = ImageEditor::adjustContrast(
    imageData,
    dataSize,
    1.5f  // 增加对比度
);

// 降低对比度
auto lowContrast = ImageEditor::adjustContrast(
    imageData,
    dataSize,
    0.7f  // 降低对比度
);
```

**实现**: `pixel = clamp((pixel - 128) * factor + 128, 0, 255)`

### 3.6 滤镜 (Filters)

```cpp
// 灰度滤镜
auto gray = ImageEditor::applyGrayscale(imageData, dataSize);

// 复古滤镜 (棕褐色)
auto sepia = ImageEditor::applySepia(imageData, dataSize);

// 怀旧滤镜 (淡黄 + 柔化)
auto vintage = ImageEditor::applyVintage(imageData, dataSize);
```

**滤镜效果**:

| 滤镜 | 效果描述 |
|------|----------|
| GRAYSCALE | 将图像转换为灰度 |
| SEPIA | 棕褐色调，温暖复古感 |
| VINTAGE | 泛黄褪色效果，怀旧风格 |

---

## 四、非破坏性编辑

### 4.1 原理

不直接修改原图，而是：
1. 记录编辑参数 (JSON 格式)
2. 存储到数据库
3. 用户请求时实时应用编辑

### 4.2 工作流程

```
用户上传照片
    ↓
用户提交编辑参数 (裁剪 + 亮度 + 滤镜)
    ↓
保存编辑参数到 photo_edits 表
    ↓
用户请求编辑后的照片
    ↓
读取原图 + 应用编辑参数 → 返回处理后的图像
```

### 4.3 编辑参数存储

```cpp
// 编辑参数序列化为 JSON
EditParams params;
params.cropX = 100;
params.cropY = 50;
params.brightness = 10;
params.filter = 2;  // SEPIA

std::string json = params.toJson();
// {"cropX":100,"cropY":50,"cropWidth":0,"cropHeight":0,
//  "rotation":0,"targetWidth":0,"targetHeight":0,
//  "originalWidth":0,"originalHeight":0,
//  "brightness":10,"contrast":1,"filter":2}
```

---

## 五、数据库 Schema

### 5.1 photo_edits 表

```sql
CREATE TABLE photo_edits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    photo_id TEXT NOT NULL,
    edit_params TEXT NOT NULL,  -- JSON 格式的编辑参数
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (photo_id) REFERENCES photos(id)
);

CREATE INDEX idx_photo_edits_photo_id ON photo_edits(photo_id);
```

### 5.2 数据结构

```cpp
struct PhotoEdit {
    int id;
    std::string photo_id;
    std::string edit_params;  // JSON
    std::string created_at;
    std::string updated_at;
};
```

---

## 六、API 端点

### 6.1 应用编辑

| 属性 | 值 |
|------|-----|
| HTTP 方法 | POST |
| 路由 | `/api/photos/{id}/edit` |
| 认证 | Bearer Token |
| 请求体 | `{"cropX": 100, "cropY": 50, "brightness": 10, "filter": "SEPIA"}` |
| 响应 | 保存的编辑参数 |

### 6.2 获取编辑参数

| 属性 | 值 |
|------|-----|
| HTTP 方法 | GET |
| 路由 | `/api/photos/{id}/edit` |
| 认证 | Bearer Token |
| 响应 | `{"edit_params": {...}}` 或 `{"edit_params": {}}` |

### 6.3 撤销编辑

| 属性 | 值 |
|------|-----|
| HTTP 方法 | POST |
| 路由 | `/api/photos/{id}/revert` |
| 认证 | Bearer Token |
| 响应 | `{"success": true}` |

---

## 七、测试

### 7.1 测试用例

```bash
# 运行图像编辑测试
./build/image_service_test
```

### 7.2 测试覆盖

| 测试组 | 覆盖内容 |
|--------|----------|
| CropTest | 裁剪坐标、尺寸验证 |
| RotateTest | 90°/180°/270° 旋转 |
| ResizeTest | 缩放算法验证 |
| BrightnessTest | 亮度范围、边界值 |
| ContrastTest | 对比度范围、边界值 |
| FilterTest | 灰度、复古、怀旧效果 |
| JsonTest | 序列化/反序列化 |

---

## 八、性能优化

### 8.1 内存管理

- 图像数据使用 `std::vector<unsigned char>` 管理
- 避免不必要的内存拷贝
- 操作完成后及时释放临时缓冲区

### 8.2 算法优化

- 裁剪: 直接内存拷贝，不经过额外处理
- 旋转: 原地变换，减少内存分配
- 缩放: 双线性插值，平衡质量和性能

### 8.3 错误处理

- 所有方法返回 `std::optional` 或检查返回值
- 参数验证在操作前完成
- 内存分配失败返回空值