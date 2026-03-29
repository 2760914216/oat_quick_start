# Ciallo HTTP Server API Reference

基于 OATPP 框架的 C++ HTTP 服务器，使用 JWT 进行身份认证。

## 一、API 路由总览

### 控制器列表

| 控制器 | 文件路径 |
|--------|----------|
| AuthController | `src/controller/AuthController.hpp` |
| UserController | `src/controller/UserController.hpp` |
| PhotoController | `src/controller/PhotoController.hpp` |
| MyController | `src/controller/MyController.hpp` |

### 公共端点 (无需认证)

- `GET /health`
- `POST /api/auth/login`
- `POST /api/auth/refresh`

---

## 二、详细 API 端点

### 1. AuthController (认证控制器)

#### 1.1 健康检查

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/health` |
| **功能描述** | 服务器健康检查端点 |
| **是否需要认证** | 否 |
| **请求参数** | 无 |
| **成功响应** | `200 OK` |
| **响应示例** | `{"statusCode": 200, "message": "OK"}` |

---

#### 1.2 用户登录

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/auth/login` |
| **功能描述** | 用户登录，验证用户名密码并返回 JWT token |
| **是否需要认证** | 否 |
| **请求头** | `Content-Type: application/json` |
| **请求体 (JSON)** | |
| ```json | |
| {
  "username": "string",
  "password": "string"
} |
| **成功响应** | `200 OK` |
| **响应体** | `LoginResponseDto` |
| **响应示例** | `{"access_token": "jwt...", "refresh_token": "jwt...", "expires_in": 3600, "username": "admin", "role": "admin"}` |
| **错误响应** | `400 Bad Request` - 用户名密码为空<br>`401 Unauthorized` - 无效凭据 |

---

#### 1.3 刷新 Token

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/auth/refresh` |
| **功能描述** | 使用 refresh_token 刷新 access_token |
| **是否需要认证** | 否 |
| **请求头** | `Content-Type: application/json` |
| **请求体 (JSON)** | |
| ```json | |
| {
  "refresh_token": "string"
} |
| **成功响应** | `200 OK` |
| **响应体** | `RefreshResponseDto` |
| **响应示例** | `{"access_token": "jwt...", "expires_in": 3600}` |
| **错误响应** | `400 Bad Request` - refresh_token 为空<br>`401 Unauthorized` - 无效或过期的 refresh_token |

---

#### 1.4 用户登出

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/auth/logout` |
| **功能描述** | 用户登出，使当前 token 失效 |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Authorization: Bearer <token>` |
| **请求体** | 无 |
| **成功响应** | `200 OK` |
| **响应体** | `LogoutResponseDto` |
| **响应示例** | `{"message": "Logged out successfully"}` |
| **错误响应** | `401 Unauthorized` - 无效或缺失认证头 |

---

### 2. UserController (用户管理控制器)

#### 2.1 获取用户列表

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/api/users` |
| **功能描述** | 获取所有用户列表 (仅管理员可访问) |
| **是否需要认证** | 是 (Bearer Token, 需 admin 角色) |
| **请求头** | `Authorization: Bearer <token>` |
| **请求参数** | 无 |
| **成功响应** | `200 OK` |
| **响应体** | `UserListDto` |
| **响应示例** | `{"users": [{"username": "admin", "created_at": "2024-01-01"}]}` |
| **错误响应** | `403 Forbidden` - 非管理员用户 |

---

#### 2.2 创建用户

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/users` |
| **功能描述** | 创建新用户 (仅管理员可访问) |
| **是否需要认证** | 是 (Bearer Token, 需 admin 角色) |
| **请求头** | `Authorization: Bearer <token>`<br>`Content-Type: application/json` |
| **请求体 (JSON)** | |
| ```json | |
| {
  "username": "string",
  "password": "string"
} |
| **成功响应** | `201 Created` |
| **响应体** | `UserDto` |
| **响应示例** | `{"username": "newuser", "created_at": null}` |
| **错误响应** | `400 Bad Request` - 用户名或密码为空<br>`403 Forbidden` - 非管理员<br>`409 Conflict` - 用户已存在<br>`500 Internal Server Error` - 创建失败 |

---

#### 2.3 删除用户

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `DELETE` |
| **路由路径** | `/api/users/{username}` |
| **功能描述** | 删除指定用户 (仅管理员可访问，不能删除管理员自身) |
| **是否需要认证** | 是 (Bearer Token, 需 admin 角色) |
| **请求头** | `Authorization: Bearer <token>` |
| **路径参数** | `username` - 要删除的用户名 |
| **成功响应** | `200 OK` |
| **响应体** | `LogoutResponseDto` |
| **响应示例** | `{"message": "User deleted successfully"}` |
| **错误响应** | `400 Bad Request` - 尝试删除管理员用户<br>`403 Forbidden` - 非管理员<br>`404 Not Found` - 用户不存在 |

---

### 3. PhotoController (照片管理控制器)

#### 3.1 获取照片列表

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/api/photos` |
| **功能描述** | 获取用户照片列表 |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Authorization: Bearer <token>` |
| **请求参数** | 无 |
| **成功响应** | `200 OK` |
| **响应体** | `PhotoListDto` |
| **响应示例** | `{"photos": [{"id": "1", "filename": "photo.jpg", "size": 1024, "created_at": "2024-01-01"}]}` |
| **错误响应** | `401 Unauthorized` - 未认证 |

---

#### 3.2 上传照片

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/photos/upload` |
| **功能描述** | 上传照片 (当前为 stub 实现) |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Authorization: Bearer <token>` |
| **请求体** | 待定 (文件上传) |
| **成功响应** | `201 Created` |
| **响应体** | `UploadResponseDto` |
| **响应示例** | `{"id": "uuid", "filename": "photo.jpg", "message": "Photo upload endpoint (stub)"}` |

---

#### 3.3 获取单个照片

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/api/photos/{id}` |
| **功能描述** | 获取指定照片详情 (尚未实现) |
| **是否需要认证** | 是 (Bearer Token) |
| **路径参数** | `id` - 照片 ID |
| **成功响应** | `501 Not Implemented` |

---

#### 3.4 删除照片

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `DELETE` |
| **路由路径** | `/api/photos/{id}` |
| **功能描述** | 删除指定照片 (当前为 stub 实现) |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Authorization: Bearer <token>` |
| **路径参数** | `id` - 照片 ID |
| **成功响应** | `200 OK` |
| **响应体** | `DeleteResponseDto` |
| **响应示例** | `{"message": "Photo delete endpoint (stub)"}` |

---

### 4. MyController (示例/杂项控制器)

#### 4.1 欢迎页面

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/hello` |
| **功能描述** | 返回欢迎消息 |
| **是否需要认证** | 否 |
| **请求参数** | 无 |
| **成功响应** | `200 OK` |
| **响应体** | `MessageDto` |
| **响应示例** | `{"statusCode": 200, "message": "Ciallo World!", "CRC": "35c9bb"}` |

---

#### 4.2 提交公钥 (RSA)

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/submit-sensitive-info` |
| **功能描述** | 提交 RSA 公钥 (当前为 stub 实现) |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Content-Type: application/json` |
| **请求体 (JSON)** | `PublicKeyDto` |
| ```json | |
| {
  "owner": "string",
  "key": "string",
  "algorithm": "RSA"
} |
| **成功响应** | `200 OK` |
| **响应体** | `PublicKeyResponseDto` |
| **响应示例** | `{"key": "..."}` |
| **错误响应** | `400 Bad Request` - 不支持的算法 |

---

#### 4.3 提交对称密钥

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `POST` |
| **路由路径** | `/api/submit-sensitive-info` |
| **功能描述** | 提交对称加密密钥 |
| **是否需要认证** | 是 (Bearer Token) |
| **请求头** | `Content-Type: application/json` |
| **请求体 (JSON)** | `SymmetricKeyDto` |
| ```json | |
| {
  "key": "string"
} |
| **成功响应** | `200 OK` |
| **响应体** | `SymmetricKeyResponseDto` |
| **响应示例** | `{"key": "..."}` |

---

#### 4.4 获取服务器公钥

| 属性 | 值 |
|------|-----|
| **HTTP 方法** | `GET` |
| **路由路径** | `/api/get-public-key` |
| **功能描述** | 获取服务器的公钥 (当前为 stub 实现) |
| **是否需要认证** | 是 (Bearer Token) |
| **成功响应** | `200 OK` |
| **响应体** | `PublicKeyDto` |
| **响应示例** | `{"owner": "server", "key": null, "algorithm": null}` |

---

## 三、数据结构定义 (DTO)

### AuthDTOs.hpp

| DTO 类名 | 字段 | 类型 | 描述 |
|----------|------|------|------|
| `LoginRequestDto` | username | String | 用户名 |
| | password | String | 密码 |
| `LoginResponseDto` | access_token | String | JWT 访问令牌 |
| | refresh_token | String | JWT 刷新令牌 |
| | expires_in | Int32 | 过期时间(秒) |
| | username | String | 用户名 |
| | role | String | 用户角色 |
| `RefreshRequestDto` | refresh_token | String | 刷新令牌 |
| `RefreshResponseDto` | access_token | String | 新的访问令牌 |
| | expires_in | Int32 | 过期时间(秒) |
| `LogoutResponseDto` | message | String | 消息 |
| `ErrorResponseDto` | error | String | 错误码 |
| | message | String | 错误消息 |
| `UserDto` | username | String | 用户名 |
| | created_at | String | 创建时间 |
| `UserListDto` | users | Vector\<UserDto\> | 用户列表 |
| `CreateUserRequestDto` | username | String | 用户名 |
| | password | String | 密码 |

---

### PhotoDTOs.hpp

| DTO 类名 | 字段 | 类型 | 描述 |
|----------|------|------|------|
| `PhotoInfoDto` | id | String | 照片 ID |
| | filename | String | 文件名 |
| | size | Int64 | 文件大小 |
| | created_at | String | 创建时间 |
| `PhotoListDto` | photos | Vector\<PhotoInfoDto\> | 照片列表 |
| `UploadResponseDto` | id | String | 上传后 ID |
| | filename | String | 文件名 |
| | message | String | 消息 |
| `DeleteResponseDto` | message | String | 消息 |

---

### DTOs.hpp

| DTO 类名 | 字段 | 类型 | 描述 |
|----------|------|------|------|
| `MessageDto` | statusCode | Int32 | 状态码 |
| | message | String | 消息 |
| | CRC | String | CRC 校验值 |
| `PublicKeyDto` | owner | String | 所有者 |
| | key | String | 公钥内容 |
| | algorithm | String | 算法类型 |
| `PublicKeyResponseDto` | key | String | 公钥内容 |
| `SymmetricKeyDto` | key | String | 对称密钥 |
| `SymmetricKeyResponseDto` | key | String | 对称密钥 |
| `UploadInitRequestDto` | filename | String | 文件名 |
| | filetype | String | 文件类型 |
| | filesize | Int64 | 文件大小 |
| | chunksize | Int64 | 块大小 |
| | chunkcount | Int64 | 块数量 |
| | encryptedSymKey | String | 加密的对称密钥 |
| `UploadInitResponseDto` | token | String | 上传令牌 |
| | chunkSize | Int64 | 建议分块大小 |
| | message | String | 消息 |
| `UploadChunkRequestDto` | chunkIndex | Int32 | 分块索引 |
| | encryptedChunk | String | 加密分块数据 |
| | chunkHash | String | 分块哈希 |
| `UploadChunkResponseDto` | chunkIndex | Int32 | 分块索引 |
| `UploadCompleteResponseDto` | fileId | String | 文件 ID |
| | message | String | 消息 |

---

## 四、认证与授权

### JWT Token 结构

```cpp
struct TokenPayload {
    std::string username;  // 用户名
    std::string role;      // 用户角色 (如 "admin", "user")
    int64_t iat;           // 签发时间
    int64_t exp;           // 过期时间
};
```

### 认证流程

1. 客户端调用 `/api/auth/login` 获取 access_token 和 refresh_token
2. 后续请求在 `Authorization` 头中携带 `Bearer <token>`
3. `AuthInterceptor` 中间件验证 token 有效性
4. 使用 `/api/auth/refresh` 刷新过期 token
5. 使用 `/api/auth/logout` 使 token 失效

### 公共端点 (无需认证)

- `GET /health`
- `POST /api/auth/login`
- `POST /api/auth/refresh`

---

## 五、API 汇总表

| # | HTTP 方法 | 路由路径 | 功能 | 认证 | 管理员 |
|---|-----------|----------|------|------|--------|
| 1 | GET | `/health` | 健康检查 | 否 | - |
| 2 | POST | `/api/auth/login` | 用户登录 | 否 | - |
| 3 | POST | `/api/auth/refresh` | 刷新令牌 | 否 | - |
| 4 | POST | `/api/auth/logout` | 用户登出 | 是 | - |
| 5 | GET | `/api/users` | 获取用户列表 | 是 | 是 |
| 6 | POST | `/api/users` | 创建用户 | 是 | 是 |
| 7 | DELETE | `/api/users/{username}` | 删除用户 | 是 | 是 |
| 8 | GET | `/api/photos` | 获取照片列表 | 是 | - |
| 9 | POST | `/api/photos/upload` | 上传照片 | 是 | - |
| 10 | GET | `/api/photos/{id}` | 获取照片详情 | 是 | - |
| 11 | DELETE | `/api/photos/{id}` | 删除照片 | 是 | - |
| 12 | GET | `/hello` | 欢迎消息 | 否 | - |
| 13 | POST | `/api/submit-sensitive-info` | 提交公钥/对称密钥 | 是 | - |
| 14 | GET | `/api/get-public-key` | 获取服务器公钥 | 是 | - |
