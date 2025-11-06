# TZZero-HTTP Examples

这个目录包含了使用 TZZero-HTTP 框架的各种示例代码。

## 示例列表

### 1. Hello World (`hello_world.cpp`)

最简单的 HTTP 服务器示例，演示基本的路由和响应。

```bash
# 编译
g++ -std=c++20 -I../include hello_world.cpp -L../build -ltzzero_lib -pthread -o hello_world

# 运行
./hello_world

# 测试
curl http://localhost:8080/
curl http://localhost:8080/api/hello
```

### 2. RESTful API (`rest_api.cpp`)

完整的 RESTful API 示例，实现了简单的用户管理功能。

**功能：**
- GET /api/users - 获取所有用户
- GET /api/user/{id} - 获取特定用户
- DELETE /api/user/{id} - 删除用户

```bash
# 编译
g++ -std=c++20 -I../include rest_api.cpp -L../build -ltzzero_lib -pthread -o rest_api

# 运行
./rest_api

# 测试
curl http://localhost:8080/api/users
curl http://localhost:8080/api/user/1
curl -X DELETE http://localhost:8080/api/user/2
```

### 3. 静态文件服务器 (`static_files.cpp`)

提供静态文件服务，支持多种文件类型。

**特性：**
- 自动识别文件类型
- 支持 index.html
- 防止目录遍历攻击

```bash
# 编译
g++ -std=c++20 -I../include static_files.cpp -L../build -ltzzero_lib -pthread -o static_files

# 运行（提供当前目录的文件）
./static_files .

# 运行（指定目录）
./static_files /path/to/your/web/root
```

### 4. WebSocket 示例 (`websocket_echo.cpp`)

WebSocket 回显服务器的基础框架（需要完整实现）。

```bash
# 编译
g++ -std=c++20 -I../include websocket_echo.cpp -L../build -ltzzero_lib -pthread -o websocket_echo

# 运行
./websocket_echo
```

## 构建所有示例

如果你想一次性构建所有示例，可以在 CMakeLists.txt 中添加示例目标，或使用以下脚本：

```bash
#!/bin/bash
cd examples

for example in hello_world rest_api static_files websocket_echo; do
    echo "Building ${example}..."
    g++ -std=c++20 -I../include ${example}.cpp -L../build -ltzzero_lib -pthread -o ${example}
done

echo "All examples built successfully!"
```

## 学习路径

建议按以下顺序学习这些示例：

1. **hello_world.cpp** - 理解基本的服务器创建和路由
2. **rest_api.cpp** - 学习如何构建 RESTful API
3. **static_files.cpp** - 了解文件服务和内容类型处理
4. **websocket_echo.cpp** - 探索高级特性（WebSocket）

## 注意事项

- 所有示例都需要先编译主库（在 build 目录中运行 `make`）
- 示例服务器默认监听 8080 端口
- 使用 Ctrl+C 停止服务器
- 生产环境使用前请添加适当的错误处理和安全检查

## 扩展示例

你可以基于这些示例创建自己的应用：

- **博客系统** - 基于 rest_api.cpp 扩展
- **API 网关** - 实现请求转发和负载均衡
- **文件上传服务** - 扩展 static_files.cpp 添加上传功能
- **实时聊天** - 完善 websocket_echo.cpp

## 性能测试

使用 wrk 或 ab 进行性能测试：

```bash
# 使用 wrk
wrk -t4 -c100 -d30s http://localhost:8080/

# 使用 Apache Bench
ab -n 10000 -c 100 http://localhost:8080/
```

## 贡献

欢迎提交新的示例！请确保：
- 代码清晰且有注释
- 包含编译和运行说明
- 更新此 README
