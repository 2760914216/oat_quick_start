# Ciallo HTTP Server - 文件存储参考文档

## 一、存储架构概述

本项目基于 OATPP 框架构建，采用 **SQLite 数据库 + 本地文件系统** 的存储方案。

### 存储组件

| 组件 | 文件路径 | 功能 |
|------|----------|------|
| ResManager | `src/resmanage/ResManager.hpp/cpp` | 资源管理器（文件读写、目录管理） |
| PhotoMetadataDb | `src/db/PhotoMetadataDb.hpp/cpp` | SQLite 数据库包装类 |
| PhotoController | `src/controller/PhotoController.hpp` | 照片 API 控制器 |
| ConfigManager | `src/config/ConfigManager.hpp/cpp` | 配置管理 |

---

## 二、存储目录结构

### 2.1 资源存储目录

```
project_root/
├── res/                          # 资源根目录
│   ├── temp/                     # 临时上传目录（分块上传中间文件）
│   ├── Texture/                  # 纹理文件
│   ├── Model/                    # 模型文件
│   ├── Audio/                    # 音频文件
│   ├── Config/                   # 配置文件
│   ├── Shaders/                  # 着色器文件
│   ├── Font/                     # 字体文件
│   ├── Video/                    # 视频文件
│   └── Photo/                    # 照片文件
├── database/                     # 数据库目录
│   └── photo_metadata.db         # SQLite 元数据数据库
├── conf.json                     # 配置文件
└── ca/                           # SSL 证书目录
```

### 2.2 目录初始化

ResManager 在初始化时自动创建目录结构：

```cpp
void init_directories() {
    if (!fs::exists(basePath_)) {
        fs::create_directories(basePath_);
    }
    for (const auto& type : typeNames_) {
        fs::path dir = fs::path(basePath_) / type.second;
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
        }
    }
}
```

### 2.3 资源类型枚举

```cpp
enum class ResourceType {
    Texture = 1, Model, Audio, Config, Shaders, Font, Video, Photo
};
```

---

## 三、文件操作接口

### 3.1 ResManager 核心方法

| 方法 | 功能 |
|------|------|
| `get_base_path()` | 获取资源根目录路径 |
| `get_res_path(type)` | 获取指定类型的资源目录路径 |
| `resource_exists(type, filename)` | 检查资源是否存在 |
| `get_resource_path(type, filename)` | 获取资源完整路径 |
| `read_resource(type, filename)` | 读取二进制资源 |
| `read_resource_text(type, filename)` | 读取文本资源 |
| `write_resource(type, filename, data)` | 写入二进制资源 |
| `delete_resource(type, filename)` | 删除资源文件 |
| `list_resources(type)` | 列出指定类型的资源 |
| `get_file_size(type, filename)` | 获取文件大小 |
| `refresh()` | 刷新目录结构 |
| `generate_photo_id()` | 生成照片 ID |
| `create_temp_session(session_id)` | 创建分块上传临时会话 |
| `write_chunk(session_id, chunk_index, data)` | 写入分块文件 |
| `merge_chunks(session_id, output_path, chunk_count)` | 合并分块文件 |
| `cleanup_session(session_id)` | 清理临时会话目录 |
| `cleanup_expired_uploads(max_age_hours)` | 清理过期上传文件 |
| `start_cleanup_thread(interval_minutes)` | 启动清理后台线程 |
| `stop_cleanup_thread()` | 停止清理后台线程 |

---

## 四、照片 API 端点

| 方法 | 路径 | 功能 | 实现状态 |
|------|------|------|----------|
| GET | `/api/photos` | 列出照片 | ✅ 已完成 |
| POST | `/api/photos/upload` | 简单上传照片 | ✅ 已完成 |
| POST | `/api/photos/init` | 初始化分块上传 | ✅ 已完成 |
| POST | `/api/photos/chunk` | 上传分块 | ✅ 已完成 |
| POST | `/api/photos/complete` | 完成分块上传 | ✅ 已完成 |
| GET | `/api/photos/{id}` | 获取照片详情 | ✅ 已完成 |
| GET | `/api/photos/{id}/file` | 下载照片文件 | ✅ 已完成 |
| DELETE | `/api/photos/{id}` | 删除照片 | ✅ 已完成 |

---

## 五、存储过程详解

### 5.1 简单上传流程

```
┌─────────┐
│  客户端  │
└────┬────┘
     │ POST /api/photos/upload (multipart/form-data)
     ▼
┌─────────────────────────────────────────────────────────┐
│                    PhotoController                       │
│                    uploadPhoto()                         │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 1. verify_auth(request) - 验证 JWT Token                 │
│    └─ JwtHelper::verify_token(token)                     │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 2. 读取请求体数据                                        │
│    └─ request->transferBodyToStream(bodyStream)          │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 3. ResManager::generate_photo_id() - 生成照片 ID         │
│    └─ 时间戳(毫秒) + "_" + 随机数(0-9999)                │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 4. ResManager::write_resource() - 写入文件到磁盘          │
│    └─ res/Photo/{filename}                             │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 5. PhotoMetadataDb::insert_photo() - 写入元数据到 SQLite  │
│    └─ INSERT INTO photos (...)                          │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────┐
│ 返回响应 │
└─────────┘
```

**函数调用顺序**:
1. `verify_auth(request)` → 验证 JWT
2. `generate_photo_id()` → 生成 ID
3. `write_resource(type, filename, data)` → 保存文件
4. `insert_photo(metadata)` → 保存元数据

---

### 5.2 分块上传流程

```
┌─────────┐
│  客户端  │
└────┬────┘
     │ 1. POST /api/photos/init (JSON)
     ▼
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              initChunkUpload()                          │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. ResManager::create_temp_session(sessionId)           │
│    └─ 创建 res/temp/{sessionId}/ 目录                   │
│ 3. 返回 token (sessionId)                               │
└─────────────────────────────────────────────────────────┘
          │
          ▼ 2. POST /api/photos/chunk (JSON + X-Upload-Token)
          │
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              uploadChunk()                              │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. ResManager::write_chunk(sessionId, chunkIndex, data)  │
│    └─ 保存到 res/temp/{sessionId}/chunk_{index}        │
│ 3. 返回 chunkIndex                                     │
└─────────────────────────────────────────────────────────┘
          │ ... 重复上传所有分块 ...
          │
          ▼ 3. POST /api/photos/complete (JSON)
          │
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              completeChunkUpload()                      │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. ResManager::merge_chunks(sessionId, outputPath, n)   │
│    └─ 合并 res/temp/{sessionId}/chunk_*                │
│    └─ 输出到 res/Photo/{filename}                       │
│ 3. ResManager::cleanup_session(sessionId)               │
│    └─ 删除 res/temp/{sessionId}/                       │
│ 4. ResManager::get_file_size() - 获取文件大小           │
│ 5. PhotoMetadataDb::insert_photo() - 保存元数据         │
└─────────────────────────────────────────────────────────┘
```

**函数调用顺序**:

**阶段一：初始化**
1. `verify_auth(request)` → 验证 JWT
2. `create_temp_session(sessionId)` → 创建临时目录

**阶段二：上传分块**
3. `write_chunk(sessionId, chunkIndex, data)` → 重复 N 次

**阶段三：完成上传**
4. `merge_chunks(sessionId, outputPath, chunkCount)` → 合并分块
5. `cleanup_session(sessionId)` → 清理临时文件
6. `get_file_size(type, filename)` → 获取文件大小
7. `insert_photo(metadata)` → 保存元数据

---

### 5.3 列出照片流程

```
┌─────────┐
│  客户端  │
└────┬────┘
     │ GET /api/photos
     ▼
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              listPhotos()                               │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. PhotoMetadataDb::get_photos_by_owner(username)       │
│    └─ SELECT * FROM photos WHERE owner = ?             │
│ 3. 返回 PhotoInfoDto 列表                               │
└─────────────────────────────────────────────────────────┘
```

**函数调用顺序**:
1. `verify_auth(request)` → 验证 JWT
2. `get_photos_by_owner(username)` → 查询元数据

---

### 5.4 获取照片详情流程

```
┌─────────┐
│  客户端  │
└────┬────┘
     │ GET /api/photos/{id}
     ▼
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              getPhoto()                                 │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. PhotoMetadataDb::get_photo_by_id(id)                │
│    └─ SELECT * FROM photos WHERE id = ?                │
│ 3. 权限检查 (owner 或 admin)                            │
│ 4. 返回 PhotoInfoDto                                   │
└─────────────────────────────────────────────────────────┘
```

**函数调用顺序**:
1. `verify_auth(request)` → 验证 JWT
2. `get_photo_by_id(id)` → 查询元数据
3. 权限检查

---

### 5.5 删除照片流程

```
┌─────────┐
│  客户端  │
└────┬────┘
     │ DELETE /api/photos/{id}
     ▼
┌─────────────────────────────────────────────────────────┐
│              PhotoController                            │
│              deletePhoto()                              │
│                                                         │
│ 1. verify_auth(request)                                │
│ 2. PhotoMetadataDb::get_photo_by_id(id)                │
│ 3. 权限检查 (owner 或 admin)                            │
│ 4. ResManager::delete_resource(type, filename)          │
│    └─ 删除 res/Photo/{filename}                        │
│ 5. PhotoMetadataDb::delete_photo(id)                    │
│    └─ DELETE FROM photos WHERE id = ?                  │
└─────────────────────────────────────────────────────────┘
```

**函数调用顺序**:
1. `verify_auth(request)` → 验证 JWT
2. `get_photo_by_id(id)` → 查询元数据
3. 权限检查
4. `delete_resource(type, filename)` → 删除文件
5. `delete_photo(id)` → 删除元数据

---

### 5.6 自动清理流程

```
┌─────────────────────────────────────────────────────────┐
│              ResManager                                 │
│              start_cleanup_thread()                      │
│                                                         │
│ 后台线程每 60 分钟执行一次:                              │
│                                                         │
│ 1. cleanup_expired_uploads(24)                          │
│    └─ 遍历 res/temp/ 目录                               │
│    └─ 删除超过 24 小时的临时会话目录                      │
└─────────────────────────────────────────────────────────┘
```

**启动时调用**:
```cpp
// App.cpp
res::ResManager::get_instance().start_cleanup_thread(60);
```

**停止时调用**:
```cpp
// App.cpp
res::ResManager::get_instance().stop_cleanup_thread();
```

---

## 六、元数据数据库

### 6.1 数据库初始化

```cpp
// App.cpp
db::PhotoMetadataDb::get_instance().initialize("database/photo_metadata.db");
```

### 6.2 表结构

```sql
CREATE TABLE photos (
    id TEXT PRIMARY KEY,
    filename TEXT NOT NULL,
    filepath TEXT NOT NULL,
    size INTEGER DEFAULT 0,
    owner TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT,
    FOREIGN KEY (owner) REFERENCES users(username)
);

CREATE INDEX idx_photos_owner ON photos(owner);
```

### 6.3 元数据结构

```cpp
struct PhotoMetadata {
    std::string id;           // 照片 ID (主键)
    std::string filename;     // 文件名
    std::string filepath;    // 完整文件路径
    int64_t size;            // 文件大小(字节)
    std::string owner;       // 所有者用户名
    std::string created_at;   // 创建时间
    std::string updated_at;   // 更新时间
};
```

### 6.4 PhotoMetadataDb 方法

| 方法 | 功能 |
|------|------|
| `initialize(db_path)` | 初始化数据库连接 |
| `insert_photo(photo)` | 插入照片元数据 |
| `update_photo(photo)` | 更新照片元数据 |
| `delete_photo(id)` | 删除照片元数据 |
| `get_photo_by_id(id)` | 按 ID 查询 |
| `get_photos_by_owner(owner)` | 按所有者查询 |
| `get_all_photos()` | 查询所有照片 |
| `photo_exists(id)` | 检查照片是否存在 |
| `close()` | 关闭数据库连接 |

---

## 七、照片 ID 生成机制

照片 ID 使用 **时间戳 + 随机数** 组合生成：

```cpp
std::string generate_photo_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string id = std::to_string(timestamp) + "_" + std::to_string(rand() % 10000);
    return id;
}
```

**格式**: `{毫秒时间戳}_{0-9999随机数}`

**示例**: `1712345678900_1234`

---

## 八、配置存储

### 8.1 配置文件格式

项目使用 `conf.json` 存储配置：

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 1145
  },
  "jwt": {
    "secret": "your-256-bit-secret-key",
    "access_token_expire": 3600,
    "refresh_token_expire": 604800
  },
  "admin": {
    "username": "admin",
    "password_hash": "..."
  },
  "users": []
}
```

### 8.2 配置结构

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `server.host` | string | `0.0.0.0` | 监听地址 |
| `server.port` | int | `1145` | 监听端口 |
| `jwt.secret` | string | - | JWT 密钥 |
| `jwt.access_token_expire` | int | `3600` | Access Token 过期时间(秒) |
| `jwt.refresh_token_expire` | int | `604800` | Refresh Token 过期时间(秒) |

---

## 九、实现状态汇总

| 功能模块 | 状态 | 备注 |
|----------|------|------|
| 目录结构创建 | ✅ 已完成 | ResManager 自动创建 |
| 资源读取 | ✅ 已完成 | 支持二进制/文本 |
| 资源写入 | ✅ 已完成 | write_resource() |
| 资源删除 | ✅ 已完成 | delete_resource() |
| 照片列表 API | ✅ 已完成 | 从 SQLite 查询 |
| 照片上传 API | ✅ 已完成 | 简单上传 |
| 照片删除 API | ✅ 已完成 | 删除文件和元数据 |
| 分块上传 | ✅ 已完成 | init/chunk/complete 三阶段 |
| 元数据存储 | ✅ 已完成 | SQLite 数据库 |
| 文件清理 | ✅ 已完成 | 后台线程自动清理 |
| 下载照片 | ✅ 已完成 | /api/photos/{id}/file |

---

## 十、相关文件路径

| 类别 | 文件路径 |
|------|----------|
| **控制器** | `src/controller/PhotoController.hpp` |
| **资源管理** | `src/resmanage/ResManager.hpp/cpp` |
| **数据库** | `src/db/PhotoMetadataDb.hpp/cpp` |
| **DTO** | `src/dto/PhotoDTOs.hpp` |
| **DTO** | `src/dto/DTOs.hpp` |
| **配置管理** | `src/config/ConfigManager.hpp/cpp` |
| **应用组件** | `src/AppComponent.hpp` |
| **主程序** | `src/App.cpp` |
| **配置文件** | `conf.json` |
| **构建配置** | `CMakeLists.txt` |
