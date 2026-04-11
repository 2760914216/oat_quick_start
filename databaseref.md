# Database Schema - 数据库模块参考文档

---

## 一、数据库概述

### 1.1 存储方案

采用 **SQLite** 作为元数据存储，文件存储作为二进制数据存储。

| 组件 | 技术 | 用途 |
|------|------|------|
| 元数据 | SQLite | 照片、编辑、缩略图、模型信息 |
| 文件存储 | 本地文件系统 | 原始图像、缩略图文件 |

### 1.2 数据库文件

```
database/
└── photo_metadata.db  # SQLite 数据库
```

---

## 二、数据库 Schema

### 2.1 photos 表 (扩展版)

```sql
CREATE TABLE IF NOT EXISTS photos (
    id TEXT PRIMARY KEY,                    -- 照片唯一 ID
    filename TEXT NOT NULL,                  -- 文件名
    filepath TEXT NOT NULL,                  -- 文件路径
    size INTEGER DEFAULT 0,                 -- 文件大小 (字节)
    owner TEXT NOT NULL,                     -- 所有者 (用户名)
    created_at TEXT NOT NULL,                -- 创建时间
    updated_at TEXT,                          -- 更新时间
    
    -- 分类相关字段
    scene_category TEXT,                     -- 场景分类
    content_category TEXT,                    -- 内容分类
    classification_status TEXT DEFAULT 'pending',  -- 分类状态
    classification_dirty INTEGER DEFAULT 0,  -- 分类脏标记
    classified_at TEXT,                       -- 分类完成时间
    
    -- Schema 版本
    schema_version TEXT DEFAULT '1.1',       -- 数据库 schema 版本
    
    -- 外键
    FOREIGN KEY (owner) REFERENCES users(username)
);
```

**分类状态值**:

| 状态 | 说明 |
|------|------|
| pending | 待分类 |
| processing | 分类中 |
| completed | 分类完成 |
| failed | 分类失败 |

### 2.2 photo_edits 表

```sql
CREATE TABLE IF NOT EXISTS photo_edits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,    -- 自增 ID
    photo_id TEXT NOT NULL,                  -- 照片 ID
    edit_params TEXT NOT NULL,               -- 编辑参数 (JSON 格式)
    created_at TEXT NOT NULL,                -- 创建时间
    updated_at TEXT NOT NULL,                -- 更新时间
    
    -- 外键
    FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
);

-- 索引
CREATE INDEX idx_photo_edits_photo_id ON photo_edits(photo_id);
```

**edit_params 格式示例**:

```json
{
  "cropX": 100,
  "cropY": 50,
  "cropWidth": 800,
  "cropHeight": 600,
  "rotation": 90,
  "targetWidth": 1920,
  "targetHeight": 1080,
  "brightness": 10,
  "contrast": 1.2,
  "filter": 2
}
```

### 2.3 photo_thumbnails 表

```sql
CREATE TABLE IF NOT EXISTS photo_thumbnails (
    id INTEGER PRIMARY KEY AUTOINCREMENT,    -- 自增 ID
    photo_id TEXT NOT NULL,                  -- 照片 ID
    thumbnail_path TEXT NOT NULL,            -- 缩略图文件路径
    size_type TEXT NOT NULL,                  -- 缩略图尺寸类型 ('list' 或 'detail')
    created_at TEXT NOT NULL,                -- 创建时间
    
    -- 外键
    FOREIGN KEY (photo_id) REFERENCES photos(id) ON DELETE CASCADE
);

-- 复合索引
CREATE INDEX idx_photo_thumbnails_photo_size 
    ON photo_thumbnails(photo_id, size_type);
```

**size_type 值**:

| 值 | 尺寸 | 用途 |
|------|------|------|
| list | 150x150 | 照片列表预览 |
| detail | 400x400 | 照片详情页预览 |

### 2.4 classification_models 表

```sql
CREATE TABLE IF NOT EXISTS classification_models (
    id INTEGER PRIMARY KEY AUTOINCREMENT,    -- 自增 ID
    model_name TEXT NOT NULL,                -- 模型名称
    model_path TEXT NOT NULL,                -- 模型文件路径
    model_type TEXT NOT NULL,                -- 模型类型 ('scene' 或 'content')
    is_active INTEGER DEFAULT 1,             -- 是否启用
    created_at TEXT NOT NULL                 -- 创建时间
);

-- 索引
CREATE INDEX idx_classification_models_type 
    ON classification_models(model_type);
CREATE INDEX idx_classification_models_active 
    ON classification_models(is_active);
```

---

## 三、数据结构

### 3.1 PhotoMetadata

```cpp
struct PhotoMetadata {
    std::string id;              // 照片 ID
    std::string filename;        // 文件名
    std::string filepath;       // 文件路径
    int64_t size = 0;            // 文件大小
    std::string owner;           // 所有者
    std::string created_at;      // 创建时间
    std::string updated_at;     // 更新时间
    
    // 分类字段
    std::string scene_category;          // 场景分类
    std::string content_category;        // 内容分类
    std::string classification_status;   // 分类状态
    int classification_dirty = 0;        // 脏标记
    std::string classified_at;           // 分类时间
    
    // Schema 版本
    std::string schema_version = "1.1";
};
```

### 3.2 PhotoEdit

```cpp
struct PhotoEdit {
    int id;
    std::string photo_id;
    std::string edit_params;   // JSON 格式
    std::string created_at;
    std::string updated_at;
};
```

### 3.3 PhotoThumbnail

```cpp
struct PhotoThumbnail {
    int id;
    std::string photo_id;
    std::string thumbnail_path;
    std::string size_type;     // "list" 或 "detail"
    std::string created_at;
};
```

### 3.4 ClassificationModel

```cpp
struct ClassificationModel {
    int id;
    std::string model_name;
    std::string model_path;
    std::string model_type;    // "scene" 或 "content"
    bool is_active = true;
    std::string created_at;
};
```

---

## 四、PhotoMetadataDb 接口

### 4.1 单例模式

```cpp
class PhotoMetadataDb {
public:
    static PhotoMetadataDb& get_instance();
    
private:
    PhotoMetadataDb();
    ~PhotoMetadataDb();
};
```

### 4.2 照片操作

```cpp
// 插入照片
bool insert_photo(const PhotoMetadata& photo);

// 更新照片
bool update_photo(const PhotoMetadata& photo);

// 删除照片
bool delete_photo(const std::string& photo_id);

// 获取照片
std::optional<PhotoMetadata> get_photo_by_id(const std::string& photo_id);

// 获取用户的所有照片
std::vector<PhotoMetadata> get_photos_by_owner(const std::string& owner);

// 获取所有照片
std::vector<PhotoMetadata> get_all_photos();
```

### 4.3 编辑操作

```cpp
// 插入/更新编辑参数
bool insert_edit(const PhotoEdit& edit);
bool update_edit(const PhotoEdit& edit);

// 删除编辑参数
bool delete_edit(const std::string& photo_id);

// 获取编辑参数
std::optional<PhotoEdit> get_edit_by_photo_id(const std::string& photo_id);
```

### 4.4 缩略图操作

```cpp
// 插入缩略图
bool insert_thumbnail(const PhotoThumbnail& thumb);

// 获取照片的所有缩略图
std::vector<PhotoThumbnail> get_thumbnails_by_photo_id(const std::string& photo_id);

// 删除缩略图
bool delete_thumbnails(const std::string& photo_id);
```

### 4.5 模型操作

```cpp
// 插入模型
bool insert_model(const ClassificationModel& model);

// 获取活动模型
std::optional<ClassificationModel> get_active_model(const std::string& model_type);

// 更新模型状态
bool update_model_status(int model_id, bool is_active);
```

---

## 五、初始化流程

### 5.1 数据库初始化

```cpp
void PhotoMetadataDb::initialize(const std::string& dbPath) {
    // 1. 打开数据库连接
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    
    // 2. 创建表
    create_tables();
    
    // 3. 执行迁移 (如果需要)
    run_migrations();
}
```

### 5.2 表创建

```cpp
void PhotoMetadataDb::create_tables() {
    // 1. 创建 photos 表
    execute("CREATE TABLE IF NOT EXISTS photos (...)");
    
    // 2. 添加新列 (如果不存在)
    execute("ALTER TABLE photos ADD COLUMN scene_category TEXT");
    execute("ALTER TABLE photos ADD COLUMN content_category TEXT");
    execute("ALTER TABLE photos ADD COLUMN classification_status TEXT DEFAULT 'pending'");
    execute("ALTER TABLE photos ADD COLUMN classification_dirty INTEGER DEFAULT 0");
    execute("ALTER TABLE photos ADD COLUMN classified_at TEXT");
    execute("ALTER TABLE photos ADD COLUMN schema_version TEXT DEFAULT '1.1'");
    
    // 3. 创建其他表
    execute("CREATE TABLE IF NOT EXISTS photo_edits (...)");
    execute("CREATE TABLE IF NOT EXISTS photo_thumbnails (...)");
    execute("CREATE TABLE IF NOT EXISTS classification_models (...)");
}
```

---

## 六、迁移策略

### 6.1 Schema 版本

- 当前版本: `1.1`
- 存储在 `photos.schema_version` 字段

### 6.2 迁移检查

```cpp
void run_migrations() {
    auto photo = get_photo_by_id("dummy");
    if (photo && photo->schema_version < CURRENT_VERSION) {
        // 执行迁移
        migrate_to_1_1();
    }
}
```

### 6.3 迁移记录

| 版本 | 迁移内容 | 日期 |
|------|----------|------|
| 1.0 → 1.1 | 添加分类字段、编辑表、缩略图表、模型表 | 项目创建时 |

---

## 七、错误处理

### 7.1 常见错误

| 错误 | 原因 | 处理 |
|------|------|------|
| SQL 语法错误 | SQL 语句错误 | 返回 false，记录日志 |
| 约束违反 | 外键/唯一约束 | 返回 false |
| 数据库锁定 | 并发访问 | 重试或返回错误 |
| 磁盘空间不足 | 磁盘满 | 返回错误 |

### 7.2 事务支持

```cpp
bool execute_with_transaction(std::function<bool()> func) {
    execute("BEGIN TRANSACTION");
    try {
        if (func()) {
            execute("COMMIT");
            return true;
        }
    } catch (...) {
        execute("ROLLBACK");
    }
    return false;
}
```

---

## 八、性能优化

### 8.1 索引

| 表 | 索引 | 用途 |
|------|------|------|
| photos | (id) | 主键查询 |
| photos | (owner) | 用户照片查询 |
| photo_edits | (photo_id) | 编辑查询 |
| photo_thumbnails | (photo_id, size_type) | 缩略图查询 |
| classification_models | (model_type, is_active) | 模型查询 |

### 8.2 查询优化

- 使用预处理语句 (prepared statements)
- 避免 SELECT *
- 使用 LIMIT 限制结果集
- 定期 VACUUM 整理数据库