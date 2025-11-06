#include <gtest/gtest.h>
#include "tzzero/http/http_request.h"

using namespace tzzero::http;

class HttpRequestTest : public ::testing::Test {
protected:
    HttpRequest request;
};

TEST_F(HttpRequestTest, DefaultState) {
    EXPECT_EQ(request.get_method(), HttpMethod::INVALID);
    EXPECT_EQ(request.get_version(), HttpVersion::UNKNOWN);
    EXPECT_TRUE(request.get_path().empty());
    EXPECT_TRUE(request.get_query().empty());
    EXPECT_TRUE(request.get_body().empty());
}

TEST_F(HttpRequestTest, SetAndGetMethod) {
    request.set_method(HttpMethod::GET);
    EXPECT_EQ(request.get_method(), HttpMethod::GET);
    EXPECT_EQ(request.get_method_string(), "GET");

    request.set_method(HttpMethod::POST);
    EXPECT_EQ(request.get_method(), HttpMethod::POST);
    EXPECT_EQ(request.get_method_string(), "POST");
}

TEST_F(HttpRequestTest, SetAndGetPath) {
    request.set_path("/index.html");
    EXPECT_EQ(request.get_path(), "/index.html");
}

TEST_F(HttpRequestTest, SetAndGetQuery) {
    request.set_query("key=value&foo=bar");
    EXPECT_EQ(request.get_query(), "key=value&foo=bar");
}

TEST_F(HttpRequestTest, SetAndGetVersion) {
    request.set_version(HttpVersion::HTTP_1_1);
    EXPECT_EQ(request.get_version(), HttpVersion::HTTP_1_1);
    EXPECT_EQ(request.get_version_string(), "HTTP/1.1");

    request.set_version(HttpVersion::HTTP_1_0);
    EXPECT_EQ(request.get_version(), HttpVersion::HTTP_1_0);
    EXPECT_EQ(request.get_version_string(), "HTTP/1.0");
}

TEST_F(HttpRequestTest, AddHeader) {
    request.add_header("Content-Type", "application/json");
    request.add_header("Accept", "text/html");

    EXPECT_TRUE(request.has_header("Content-Type"));
    EXPECT_TRUE(request.has_header("Accept"));
    EXPECT_EQ(request.get_header("Content-Type"), "application/json");
    EXPECT_EQ(request.get_header("Accept"), "text/html");
}

TEST_F(HttpRequestTest, SetHeader) {
    request.add_header("Host", "example.com");
    EXPECT_EQ(request.get_header("Host"), "example.com");

    request.set_header("Host", "newhost.com");
    EXPECT_EQ(request.get_header("Host"), "newhost.com");
}

TEST_F(HttpRequestTest, RemoveHeader) {
    request.add_header("X-Custom", "value");
    EXPECT_TRUE(request.has_header("X-Custom"));

    request.remove_header("X-Custom");
    EXPECT_FALSE(request.has_header("X-Custom"));
}

TEST_F(HttpRequestTest, GetNonExistentHeader) {
    EXPECT_TRUE(request.get_header("NonExistent").empty());
}

TEST_F(HttpRequestTest, SetAndGetBody) {
    std::string body = "{\"key\": \"value\"}";
    request.set_body(body);

    EXPECT_EQ(request.get_body(), body);
}

TEST_F(HttpRequestTest, MoveBody) {
    std::string body = "Large body content";
    request.set_body(std::move(body));

    EXPECT_EQ(request.get_body(), "Large body content");
}

TEST_F(HttpRequestTest, ContentLength) {
    request.set_body("Test");
    request.add_header("Content-Length", "4");

    EXPECT_EQ(request.get_content_length(), 4);
}

TEST_F(HttpRequestTest, KeepAlive) {
    // HTTP/1.1 默认keep-alive
    request.set_version(HttpVersion::HTTP_1_1);
    EXPECT_TRUE(request.keep_alive());

    // 显式 Connection: close
    request.add_header("Connection", "close");
    EXPECT_FALSE(request.keep_alive());

    // HTTP/1.0 默认不keep-alive
    HttpRequest req2;
    req2.set_version(HttpVersion::HTTP_1_0);
    EXPECT_FALSE(req2.keep_alive());

    // HTTP/1.0 但显式 Connection: keep-alive
    req2.add_header("Connection", "keep-alive");
    EXPECT_TRUE(req2.keep_alive());
}

TEST_F(HttpRequestTest, ParseState) {
    EXPECT_EQ(request.get_parse_state(), HttpRequest::PARSE_REQUEST_LINE);

    request.set_parse_state(HttpRequest::PARSE_HEADERS);
    EXPECT_EQ(request.get_parse_state(), HttpRequest::PARSE_HEADERS);

    request.set_parse_state(HttpRequest::PARSE_COMPLETE);
    EXPECT_EQ(request.get_parse_state(), HttpRequest::PARSE_COMPLETE);
}

TEST_F(HttpRequestTest, Reset) {
    request.set_method(HttpMethod::POST);
    request.set_path("/api/data");
    request.add_header("Content-Type", "application/json");
    request.set_body("{\"test\": true}");

    request.reset();

    EXPECT_EQ(request.get_method(), HttpMethod::INVALID);
    EXPECT_TRUE(request.get_path().empty());
    EXPECT_FALSE(request.has_header("Content-Type"));
    EXPECT_TRUE(request.get_body().empty());
}

TEST_F(HttpRequestTest, CopyConstructor) {
    request.set_method(HttpMethod::GET);
    request.set_path("/test");
    request.add_header("Host", "example.com");

    HttpRequest copied(request);

    EXPECT_EQ(copied.get_method(), HttpMethod::GET);
    EXPECT_EQ(copied.get_path(), "/test");
    EXPECT_EQ(copied.get_header("Host"), "example.com");
}

TEST_F(HttpRequestTest, MoveConstructor) {
    request.set_method(HttpMethod::POST);
    request.set_path("/submit");
    request.set_body("data");

    HttpRequest moved(std::move(request));

    EXPECT_EQ(moved.get_method(), HttpMethod::POST);
    EXPECT_EQ(moved.get_path(), "/submit");
    EXPECT_EQ(moved.get_body(), "data");
}

TEST_F(HttpRequestTest, StreamId) {
    request.set_stream_id(123);
    EXPECT_EQ(request.get_stream_id(), 123);
}
