#include <gtest/gtest.h>
#include "tzzero/http/http_parser.h"
#include "tzzero/utils/buffer.h"

using namespace tzzero::http;
using namespace tzzero::utils;

class HttpParserTest : public ::testing::Test {
protected:
    HttpParser parser;
    Buffer buffer;
    HttpRequest request;
};

TEST_F(HttpParserTest, ParseSimpleGetRequest) {
    std::string http_request =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: Test\r\n"
        "\r\n";

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_FALSE(parser.has_error());
    EXPECT_EQ(request.get_method(), HttpMethod::GET);
    EXPECT_EQ(request.get_path(), "/index.html");
    EXPECT_EQ(request.get_version(), HttpVersion::HTTP_1_1);
    EXPECT_EQ(request.get_header("Host"), "example.com");
    EXPECT_EQ(request.get_header("User-Agent"), "Test");
}

TEST_F(HttpParserTest, ParseGetRequestWithQuery) {
    std::string http_request =
        "GET /search?q=test&lang=en HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_EQ(request.get_method(), HttpMethod::GET);
    EXPECT_EQ(request.get_path(), "/search");
    EXPECT_EQ(request.get_query(), "q=test&lang=en");
}

TEST_F(HttpParserTest, ParsePostRequestWithBody) {
    std::string http_request =
        "POST /api/data HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "{\"key\": \"value\"}";

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_FALSE(parser.has_error());
    EXPECT_EQ(request.get_method(), HttpMethod::POST);
    EXPECT_EQ(request.get_path(), "/api/data");
    EXPECT_EQ(request.get_header("Content-Type"), "application/json");
    EXPECT_EQ(request.get_body(), "{\"key\": \"value\"}");
}

TEST_F(HttpParserTest, ParsePartialRequest) {
    std::string partial1 = "GET /test HTTP/1.1\r\n";
    buffer.append(partial1);

    bool complete = parser.parse_request(buffer, request);
    EXPECT_FALSE(complete);

    std::string partial2 = "Host: example.com\r\n\r\n";
    buffer.append(partial2);

    complete = parser.parse_request(buffer, request);
    EXPECT_TRUE(complete);
    EXPECT_EQ(request.get_method(), HttpMethod::GET);
    EXPECT_EQ(request.get_path(), "/test");
}

TEST_F(HttpParserTest, ParseDifferentMethods) {
    struct MethodTest {
        std::string method_str;
        HttpMethod method_enum;
    };

    std::vector<MethodTest> tests = {
        {"GET", HttpMethod::GET},
        {"POST", HttpMethod::POST},
        {"PUT", HttpMethod::PUT},
        {"DELETE", HttpMethod::DELETE},
        {"HEAD", HttpMethod::HEAD},
        {"OPTIONS", HttpMethod::OPTIONS},
        {"PATCH", HttpMethod::PATCH}
    };

    for (const auto& test : tests) {
        HttpParser p;
        Buffer buf;
        HttpRequest req;

        std::string http_req = test.method_str + " / HTTP/1.1\r\n\r\n";
        buf.append(http_req);

        bool complete = p.parse_request(buf, req);

        EXPECT_TRUE(complete);
        EXPECT_EQ(req.get_method(), test.method_enum);
    }
}

TEST_F(HttpParserTest, ParseHttp10Request) {
    std::string http_request =
        "GET /index.html HTTP/1.0\r\n"
        "Host: example.com\r\n"
        "\r\n";

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_EQ(request.get_version(), HttpVersion::HTTP_1_0);
}

TEST_F(HttpParserTest, ParseMultipleHeaders) {
    std::string http_request =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "Accept-Language: en-US\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_EQ(request.get_header("Host"), "example.com");
    EXPECT_EQ(request.get_header("User-Agent"), "TestClient/1.0");
    EXPECT_EQ(request.get_header("Accept"), "text/html");
    EXPECT_EQ(request.get_header("Accept-Language"), "en-US");
    EXPECT_EQ(request.get_header("Connection"), "keep-alive");
}

TEST_F(HttpParserTest, Reset) {
    std::string http_request = "GET / HTTP/1.1\r\n\r\n";
    buffer.append(http_request);

    parser.parse_request(buffer, request);

    parser.reset();
    EXPECT_FALSE(parser.has_error());
}

TEST_F(HttpParserTest, ParseRequestWithLargeBody) {
    std::string body(1000, 'X');
    std::string http_request =
        "POST /upload HTTP/1.1\r\n"
        "Content-Length: 1000\r\n"
        "\r\n" + body;

    buffer.append(http_request);

    bool complete = parser.parse_request(buffer, request);

    EXPECT_TRUE(complete);
    EXPECT_EQ(request.get_body().size(), 1000);
    EXPECT_EQ(request.get_body(), body);
}
