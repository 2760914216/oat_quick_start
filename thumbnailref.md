# Thumbnail Generation Service - 缩略图生成模块参考文档

---

## 一、模块概述

### 1.1 功能特性

| 特性 | 说明 |
|------|------|
| 多种尺寸 | 支持 LIST (150x150) 和 DETAIL (400x400) 两种尺寸 |
| 等比缩放 | 保持原始图像宽高比 |
| 自动生成 | 上传时自动生成缩略图 |
| 按需生成 | API 调用时按需生成 |
| JPEG 压缩 | 80% 质量压缩 |

### 1.2 目录结构

```
src/service/
├── ThumbnailGenerator.hpp   # 缩略图生成器头文件
└── ThumbnailGenerator.cpp   # 缩略图生成器实现
```

---

## 二、核心组件

### 2.1 ThumbnailGenerator 类

单例模式，提供静态方法：

```cpp
class ThumbnailGenerator {
public:
    // 删除拷贝构造和赋值
    ThumbnailGenerator() = delete;
    ThumbnailGenerator(const ThumbnailGenerator&) = delete;
    ThumbnailGenerator& operator=(const ThumbnailGenerator&) = delete;

    // 生成缩略图
    static std::optional<std::string> generateThumbnail(
        const std::string& photoId,
        const std::vector<unsigned char>& imageData,
        const std::string& sizeType  // "list" 或 "detail"
    );

    // 获取指定照片的所有缩略图
    static std::vector<PhotoThumbnail> getThumbnails(
        const std::string& photoId
    );

    // 删除指定照片的所有缩略图
    static bool deleteThumbnails(const std::string& photoId);
};
```

### 2.2 尺寸规格

| 尺寸类型 | 尺寸 | 用途 |
|----------|------|------|
| LIST | 150x150 | 照片列表预览 |
| DETAIL | 400x400 | 照片详情页预览 |

### 2.3 存储路径

```
res/Thumbnail/
├── {photo_id}_list.jpg    # 列表缩略图
└── {photo_id}_detail.jpg  # 详情缩略图
```

---

## 三、工作原理

### 3.1 生成流程

```
输入图像数据
    ↓
解码图像 (stb_image)
    ↓
计算等比缩放尺寸
    ↓
创建输出缓冲区
    ↓
执行缩放 (双线性插值)
    ↓
编码为 JPEG (80% 质量)
    ↓
保存到文件系统
    ↓
写入数据库记录
```

### 3.2 等比缩放算法

```cpp
std::pair<int, int> calculateAspectFit(
    int srcW, int srcH, 
    int maxW, int maxH
) {
    double ratio = std::min(
        (double)maxW / srcW, 
        (double)maxH / srcH
    );
    
    int targetW = static_cast<int>(srcW * ratio);
    int targetH = static_cast<int>(srcH * ratio);
    
    return {targetW, targetH};
}
```

---

## 四、数据库 Schema

### 4.1 photo_thumbnails 表

```sql
CREATE TABLE photo_thumbnails (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    photo_id TEXT NOT NULL,
    thumbnail_path TEXT NOT NULL,
    size_type TEXT NOT NULL,  -- 'list' 或 'detail'
    created_at TEXT NOT NULL,
    FOREIGN KEY (photo_id) REFERENCES photos(id)
);

CREATE INDEX idx_photo_size ON photo_thumbnails(photo_id, size_type);
```

### 4.2 数据结构

```cpp
struct PhotoThumbnail {
    int id;
    std::string photo_id;
    std::string thumbnail_path;
    std::string size_type;  // "list" 或 "detail"
    std::string created_at;
};
```

---

## 五、API 端点

### 5.1 获取缩略图

| 属性 | 值 |
|------|-----|
| HTTP 方法 | GET |
| 路由 | `/api/photos/{id}/thumbnail` |
| 认证 | Bearer Token |
| 查询参数 | `size=list\|detail` (默认: list) |
| 响应 | JPEG 图像文件 |

### 5.2 请求示例

```bash
# 获取列表缩略图
curl -H "Authorization: Bearer <token>" \
     http://localhost:8080/api/photos/123456/thumbnail

# 获取详情缩略图
curl -H "Authorization: Bearer <token>" \
     http://localhost:8080/api/photos/123456/thumbnail?size=detail
```

### 5.3 响应

- **成功**: 返回 JPEG 图像，Content-Type: `image/jpeg`
- **404**: 照片或缩略图不存在
- **401**: 未认证

---

## 六、自动生成集成

### 6.1 上传时自动生成

在 `uploadPhoto` 端点中添加自动生成逻辑：

```cpp
// 上传成功后自动生成缩略图
try {
    auto imageData = ImageLoader::load(metadata.filepath);
    if (imageData) {
        // 生成列表缩略图
        auto listThumbId = ThumbnailGenerator::generateThumbnail(
            photoId, *imageData, "list"
        );
        
        // 生成详情缩略图
        auto detailThumbId = ThumbnailGenerator::generateThumbnail(
            photoId, *imageData, "detail"
        );
    }
} catch (...) {
    // 缩略图生成失败不影响上传成功
}
```

### 6.2 按需生成

在 `getPhotoThumbnail` 端点中：

```cpp
// 如果缩略图不存在，按需生成
auto thumbnails = ThumbnailGenerator::getThumbnails(photoId);
if (thumbnails.empty()) {
    // 加载原图并生成缩略图
    auto imageData = ImageLoader::load(photo.filepath);
    if (imageData) {
        ThumbnailGenerator::generateThumbnail(photoId, *imageData, sizeType);
    }
}
```

---

## 七、错误处理

### 7.1 常见错误

| 错误 | 原因 | 处理 |
|------|------|------|
| 图像解码失败 | 损坏的图像文件 | 返回 500 错误 |
| 缩放失败 | 内存不足或参数错误 | 返回空值 |
| 文件写入失败 | 权限或磁盘空间 | 返回 500 错误 |
| 数据库错误 | SQLite 错误 | 返回错误日志 |

### 7.2 降级策略

- 缩略图生成失败不影响主业务 (上传、分类)
- 缩略图不存在时返回 404 而不是 500
- 上传时生成失败静默处理，不抛出异常

---

## 八、测试

### 8.1 测试用例

```bash
# 运行图像服务测试
./build/image_service_test
```

### 8.2 测试覆盖

| 测试组 | 内容 |
|--------|------|
| ThumbnailGeneratorTest | 尺寸计算、路径生成 |
| AspectRatioTest | 等比缩放算法 |
| StorageTest | 文件存储验证 |