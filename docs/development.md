# 开发指南

本文档为 TZZero-HTTP 项目的贡献者提供开发指南。

## 开发环境

### 必需工具

- **编译器**: GCC 11+ 或 Clang 14+
- **构建工具**: CMake 3.20+, Make
- **版本控制**: Git
- **代码编辑**: 任何支持 C++ 的编辑器（推荐 VS Code, CLion）

### 可选工具

- **调试**: GDB, LLDB
- **性能分析**: perf, valgrind, gperftools
- **静态分析**: clang-tidy, cppcheck
- **格式化**: clang-format

## 代码风格

### 命名规范

```cpp
// 类名：PascalCase
class HttpServer { };

// 函数名：snake_case
void handle_request();

// 变量名：snake_case
int connection_count;

// 私有成员变量：trailing underscore
class MyClass {
private:
    int value_;
    std::string name_;
};

// 常量：kPascalCase
const int kDefaultPort = 8080;
static constexpr size_t kBufferSize = 4096;

// 枚举：PascalCase
enum class LogLevel {
    DEBUG,
    INFO,
    WARN
};
```

### 代码组织

```cpp
#pragma once  // 头文件保护使用 #pragma once

// 标准库头文件
#include <string>
#include <vector>

// 第三方库头文件
#include <openssl/ssl.h>

// 项目头文件
#include "tzzero/core/event_loop.h"
#include "tzzero/utils/logger.h"

namespace tzzero::http {  // 使用嵌套命名空间

class HttpServer {
public:
    // 公有类型定义
    using Callback = std::function<void()>;

    // 构造函数和析构函数
    HttpServer();
    ~HttpServer();

    // 拷贝和移动（根据需要）
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // 公有方法
    void start();
    void stop();

private:
    // 私有方法
    void init();

    // 私有成员变量
    int port_;
    std::string address_;
};

}  // namespace tzzero::http
```

### 注释规范

```cpp
/**
 * 类注释：简要描述类的功能
 *
 * 详细说明类的职责、使用方式等
 */
class MyClass {
public:
    /**
     * 方法注释：描述函数功能
     *
     * @param name 参数说明
     * @return 返回值说明
     * @throws std::runtime_error 异常说明
     */
    std::string process(const std::string& name);

private:
    int value_;  ///< 成员变量行内注释
};
```

## 编译和测试

### 开发构建

```bash
# Debug 模式（包含调试信息和 sanitizers）
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Release 模式（优化性能）
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 运行测试

```bash
# 运行所有测试
./tzzero_tests

# 运行特定测试套件
./tzzero_tests --gtest_filter=BufferTest.*

# 生成测试报告
./tzzero_tests --gtest_output=xml:test_report.xml
```

### 代码覆盖率

```bash
# 使用 gcov 生成覆盖率报告
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make
./tzzero_tests
gcov -r src/*.cpp
```

## 添加新功能

### 1. 创建分支

```bash
git checkout -b feature/my-new-feature
```

### 2. 实现功能

#### 添加头文件

在 `include/tzzero/` 相应目录下创建头文件：

```cpp
// include/tzzero/http/my_feature.h
#pragma once

namespace tzzero::http {

class MyFeature {
public:
    MyFeature();
    void do_something();
};

}  // namespace tzzero::http
```

#### 实现文件

在 `src/` 相应目录下创建实现文件：

```cpp
// src/http/my_feature.cpp
#include "tzzero/http/my_feature.h"
#include "tzzero/utils/logger.h"

namespace tzzero::http {

MyFeature::MyFeature() {
    LOG_INFO("MyFeature created");
}

void MyFeature::do_something() {
    // 实现逻辑
}

}  // namespace tzzero::http
```

#### 更新 CMakeLists.txt

```cmake
set(TZZERO_SOURCES
    # ... existing sources ...
    src/http/my_feature.cpp
)
```

### 3. 编写测试

在 `tests/` 目录创建测试文件：

```cpp
// tests/my_feature_test.cpp
#include <gtest/gtest.h>
#include "tzzero/http/my_feature.h"

using namespace tzzero::http;

class MyFeatureTest : public ::testing::Test {
protected:
    MyFeature feature;
};

TEST_F(MyFeatureTest, BasicFunctionality) {
    EXPECT_NO_THROW(feature.do_something());
}

TEST_F(MyFeatureTest, EdgeCase) {
    // 测试边界情况
}
```

更新 CMakeLists.txt 添加测试文件：

```cmake
add_executable(tzzero_tests
    # ... existing tests ...
    tests/my_feature_test.cpp
)
```

### 4. 文档

更新相关文档：
- `docs/api.md` - 添加 API 说明
- `README.md` - 如果是重要功能，更新主文档
- `examples/` - 添加使用示例

### 5. 提交代码

```bash
# 确保代码格式正确
clang-format -i src/http/my_feature.cpp include/tzzero/http/my_feature.h

# 运行测试
make test

# 提交
git add .
git commit -m "feat: add MyFeature for XYZ functionality"
git push origin feature/my-new-feature
```

## 调试技巧

### 使用 GDB

```bash
# 编译 Debug 版本
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 启动 GDB
gdb ./tzzero-http

# GDB 命令
(gdb) break HttpServer::start  # 设置断点
(gdb) run --port 8080          # 运行
(gdb) print variable_name      # 打印变量
(gdb) backtrace                # 查看调用栈
```

### 使用日志

```cpp
// 在关键位置添加日志
LOG_DEBUG("Connection state: " << connection->state());
LOG_INFO("Processing request: " << request.get_path());
LOG_WARN("High memory usage: " << memory_usage << " MB");
LOG_ERROR("Failed to parse request: " << error);
```

### Address Sanitizer

```bash
# CMake 已配置在 Debug 模式启用 ASan
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
./tzzero-http  # 自动检测内存错误
```

### Valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./tzzero-http
```

## 性能分析

### 使用 perf

```bash
# 记录性能数据
perf record -g ./tzzero-http

# 分析报告
perf report

# 火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### 使用 gperftools

```cpp
#include <gperftools/profiler.h>

int main() {
    ProfilerStart("profile.prof");

    // 运行服务器
    server.start();

    ProfilerStop();
    return 0;
}
```

```bash
# 分析性能数据
google-pprof --text ./tzzero-http profile.prof
```

## 常见问题

### 编译错误

**问题**: 找不到头文件
```
fatal error: tzzero/http/http_server.h: No such file or directory
```

**解决**: 确保使用 `-I` 包含正确的目录
```bash
g++ -I./include your_code.cpp
```

### 链接错误

**问题**: undefined reference
```
undefined reference to `tzzero::http::HttpServer::start()'
```

**解决**: 链接 tzzero_lib
```bash
g++ your_code.cpp -L./build -ltzzero_lib
```

### 运行时错误

**问题**: Segmentation fault

**调试步骤**:
1. 使用 GDB 查看崩溃位置
2. 检查是否有空指针解引用
3. 使用 ASan 检测内存错误
4. 查看日志输出

## 代码审查清单

在提交 PR 前，确保：

- [ ] 代码遵循项目风格规范
- [ ] 所有新功能都有对应的测试
- [ ] 测试全部通过
- [ ] 没有编译警告
- [ ] 更新了相关文档
- [ ] 提交信息清晰描述了更改
- [ ] 代码通过静态分析工具检查

## 提交规范

使用语义化提交信息：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**类型**:
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建或辅助工具

**示例**:
```
feat(http): add middleware support

Implement middleware system for HTTP server allowing users to
register pre/post request handlers.

Closes #123
```

## 社区

- **GitHub Issues**: 报告 bug 或建议新功能
- **Pull Requests**: 贡献代码
- **Discussions**: 技术讨论

## 许可证

所有贡献的代码将采用 MIT 许可证发布。

---

感谢你对 TZZero-HTTP 的贡献！
