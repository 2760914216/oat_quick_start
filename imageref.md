# Image Loader - 图像加载模块参考文档

---

## 一、模块概述

### 1.1 功能特性

| 特性 | 说明 |
|------|------|
| 多格式支持 | JPEG, PNG, WebP, BMP, GIF, TIFF |
| 内存加载 | 加载到 std::vector<unsigned char> |
| 保存功能 | 保存为 JPEG/PNG 格式 |
| 无依赖 | 使用 stb_image 库，无需额外依赖 |

### 1.2 目录结构

```
src/image/
├── ImageLoader.hpp      # 图像加载器头文件
├── ImageLoader.cpp      # 图像加载器实现
├── stb_image.h          # 图像解码库 (单头文件)
└── stb_image_write.h    # 图像编码库 (单头文件)
```

---

## 二、核心组件

### 2.1 ImageLoader 类

```cpp
class ImageLoader {
public:
    // 禁止实例化
    ImageLoader() = delete;

    // ============ 加载图像 ============
    
    // 从文件加载图像
    static std::optional<ImageData> load(const std::string& filepath);
    
    // 从内存加载图像
    static std::optional<ImageData> loadFromMemory(
        const std::vector<unsigned char>& data
    );

    // ============ 保存图像 ============
    
    // 保存为 JPEG
    static bool saveAsJpeg(
        const std::string& filepath,
        const std::vector<unsigned char>& data,
        int width, int height, int channels,
        int quality = 90
    );
    
    // 保存为 PNG
    static bool saveAsPng(
        const std::string& filepath,
        const std::vector<unsigned char>& data,
        int width, int height, int channels
    );

    // ============ 辅助方法 ============
    
    // 获取图像信息
    static std::optional<ImageInfo> getInfo(const std::string& filepath);
    
    // 检查是否为有效图像
    static bool isValidImage(const std::string& filepath);
};
```

### 2.2 ImageData 数据结构

```cpp
struct ImageData {
    std::vector<unsigned char> data;  // 像素数据 (RGB/RGBA)
    int width;                         // 宽度 (像素)
    int height;                        // 高度 (像素)
    int channels;                      // 通道数 (3=RGB, 4=RGBA)
};

struct ImageInfo {
    int width;
    int height;
    int channels;
    std::string format;  // "JPEG", "PNG", etc.
};
```

---

## 三、使用示例

### 3.1 加载图像

```cpp
// 从文件加载
auto image = ImageLoader::load("photo.jpg");
if (image) {
    std::cout << "Loaded: " << image->width << "x" << image->height 
              << " (" << image->channels << " channels)" << std::endl;
    
    // 访问像素数据
    auto& data = image->data;
    // data[0] = R, data[1] = G, data[2] = B (如果是 RGB)
}

// 加载失败
if (!image) {
    std::cerr << "Failed to load image" << std::endl;
}
```

### 3.2 保存图像

```cpp
// 保存为 JPEG (90% 质量)
bool success = ImageLoader::saveAsJpeg(
    "output.jpg",
    imageData.data,
    imageData.width,
    imageData.height,
    imageData.channels,
    90  // 质量 0-100
);

// 保存为 PNG
success = ImageLoader::saveAsPng(
    "output.png",
    imageData.data,
    imageData.width,
    imageData.height,
    imageData.channels
);
```

### 3.3 获取图像信息

```cpp
auto info = ImageLoader::getInfo("photo.jpg");
if (info) {
    std::cout << "Format: " << info->format << std::endl;
    std::cout << "Size: " << info->width << "x" << info->height << std::endl;
}
```

---

## 四、支持的格式

### 4.1 解码格式

| 格式 | 扩展名 | 说明 |
|------|--------|------|
| JPEG | .jpg, .jpeg | 有损压缩，最常用 |
| PNG | .png | 无损压缩，支持透明 |
| WebP | .webp | 现代格式，压缩率高 |
| BMP | .bmp | 无压缩，位图 |
| GIF | .gif | 动画支持 |
| TIFF | .tiff | 高质量，无压缩 |

### 4.2 编码格式

| 格式 | 质量参数 | 透明度 |
|------|----------|--------|
| JPEG | 0-100 (默认 90) | 不支持 |
| PNG | 无 (无损) | 支持 |

---

## 五、stb_image 集成

### 5.1 头文件定义

```cpp
// 在一个 cpp 文件中定义一次
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
```

### 5.2 特点

- **单头文件**: 无需链接库，仅需头文件
- **轻量级**: 整个库约 10KB
- **无依赖**: 纯 C 实现，无外部依赖
- **跨平台**: 支持 Windows/Linux/macOS

---

## 六、像素格式

### 6.1 数据排列

图像数据按行存储，每像素连续排列：

```
RGB 格式: [R,G,B,R,G,B,R,G,B,...]
RGBA 格式: [R,G,B,A,R,G,B,A,...]
```

### 6.2 内存计算

```cpp
size_t dataSize = width * height * channels;  // 字节数

// 例如: 1920x1080 RGB 图像
size_t size = 1920 * 1080 * 3;  // 6,220,800 字节 ≈ 6MB
```

---

## 七、错误处理

### 7.1 错误类型

| 错误 | 原因 | 处理 |
|------|------|------|
| 文件不存在 | 路径错误 | 返回 std::nullopt |
| 格式不支持 | 非图像文件 | 返回 std::nullopt |
| 文件损坏 | 解码失败 | 返回 std::nullopt |
| 内存不足 | 图像太大 | 返回 std::nullopt |

### 7.2 使用 std::optional

```cpp
auto image = ImageLoader::load(filepath);
if (!image) {
    // 处理错误
    return Error;
}

// 安全使用
processImage(*image);
```

---

## 八、性能考虑

### 8.1 内存使用

- 图像数据存储在堆内存中
- 大图像可能占用大量内存
- 使用后及时释放或利用智能指针

### 8.2 加载速度

| 格式 | 加载速度 | 说明 |
|------|----------|------|
| JPEG | 快 | 有损压缩，解码快 |
| PNG | 中 | 无损压缩，解码较慢 |
| BMP | 最快 | 无压缩，直接读取 |
| WebP | 慢 | 解码较复杂 |

### 8.3 建议

- 加载前检查文件是否存在
- 大图像考虑分块加载
- 加载失败时提供具体错误信息