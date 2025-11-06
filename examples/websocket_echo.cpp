/*
 * WebSocket 回显服务器示例
 * 注意：这是一个简化示例，实际的 WebSocket 实现需要更复杂的协议处理
 */

#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include "tzzero/utils/logger.h"
#include <iostream>

using namespace tzzero::http;
using namespace tzzero::utils;

int main() {
    Logger::instance().set_level(LogLevel::INFO);

    HttpServer server("0.0.0.0", 8080);

    // WebSocket 握手处理
    server.route("/ws", [](const HttpRequest& req, HttpResponse& resp) {
        // 检查是否是 WebSocket 升级请求
        if (req.get_header("Upgrade") == "websocket" &&
            req.get_header("Connection").find("Upgrade") != std::string::npos) {

            LOG_INFO("WebSocket upgrade request received");

            // 注意：实际的 WebSocket 实现需要：
            // 1. 计算 Sec-WebSocket-Accept 响应头
            // 2. 处理 WebSocket 帧
            // 3. 维护长连接
            // 这里仅作演示

            resp.set_status_code(HttpStatusCode::SWITCHING_PROTOCOLS);
            resp.add_header("Upgrade", "websocket");
            resp.add_header("Connection", "Upgrade");
            // resp.add_header("Sec-WebSocket-Accept", computed_accept);

        } else {
            resp.set_status_code(HttpStatusCode::BAD_REQUEST);
            resp.set_text_content_type();
            resp.set_body("WebSocket upgrade required");
        }
    });

    // 提供测试页面
    server.route("/", [](const HttpRequest& req, HttpResponse& resp) {
        const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>WebSocket Echo Test</title>
</head>
<body>
    <h1>WebSocket Echo Server</h1>
    <p>This is a placeholder example.</p>
    <p>Full WebSocket implementation requires:</p>
    <ul>
        <li>WebSocket handshake (Sec-WebSocket-Accept calculation)</li>
        <li>Frame encoding/decoding</li>
        <li>Long-lived connection management</li>
        <li>Ping/Pong heartbeat</li>
    </ul>
    <script>
        // Example WebSocket client code:
        // const ws = new WebSocket('ws://localhost:8080/ws');
        // ws.onmessage = (event) => console.log('Received:', event.data);
        // ws.send('Hello, Server!');
    </script>
</body>
</html>
)";

        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body(html);
    });

    std::cout << "WebSocket Echo Server (Placeholder) starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Visit http://localhost:8080 for information" << std::endl;

    server.start();

    return 0;
}
