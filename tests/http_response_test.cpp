#include <gtest/gtest.h>
#include "tzzero/http/http_response.h"

using namespace tzzero::http;

class HttpResponseTest : public ::testing::Test {
protected:
    HttpResponse response;
};

TEST_F(HttpResponseTest, DefaultState) {
    EXPECT_EQ(response.get_status_code(), HttpStatusCode::OK);
    EXPECT_FALSE(response.close_connection());
    EXPECT_TRUE(response.get_body().empty());
}

TEST_F(HttpResponseTest, SetAndGetStatusCode) {
    response.set_status_code(HttpStatusCode::NOT_FOUND);
    EXPECT_EQ(response.get_status_code(), HttpStatusCode::NOT_FOUND);
    EXPECT_EQ(response.get_status_message(), "Not Found");

    response.set_status_code(HttpStatusCode::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.get_status_code(), HttpStatusCode::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.get_status_message(), "Internal Server Error");
}

TEST_F(HttpResponseTest, StatusMessages) {
    response.set_status_code(HttpStatusCode::OK);
    EXPECT_EQ(response.get_status_message(), "OK");

    response.set_status_code(HttpStatusCode::CREATED);
    EXPECT_EQ(response.get_status_message(), "Created");

    response.set_status_code(HttpStatusCode::BAD_REQUEST);
    EXPECT_EQ(response.get_status_message(), "Bad Request");

    response.set_status_code(HttpStatusCode::FORBIDDEN);
    EXPECT_EQ(response.get_status_message(), "Forbidden");
}

TEST_F(HttpResponseTest, SetAndGetHeaders) {
    response.add_header("Content-Type", "text/html");
    response.add_header("Server", "TZZero");

    EXPECT_TRUE(response.has_header("Content-Type"));
    EXPECT_TRUE(response.has_header("Server"));
    EXPECT_EQ(response.get_header("Content-Type"), "text/html");
    EXPECT_EQ(response.get_header("Server"), "TZZero");
}

TEST_F(HttpResponseTest, SetHeader) {
    response.add_header("X-Custom", "value1");
    EXPECT_EQ(response.get_header("X-Custom"), "value1");

    response.set_header("X-Custom", "value2");
    EXPECT_EQ(response.get_header("X-Custom"), "value2");
}

TEST_F(HttpResponseTest, RemoveHeader) {
    response.add_header("X-Test", "value");
    EXPECT_TRUE(response.has_header("X-Test"));

    response.remove_header("X-Test");
    EXPECT_FALSE(response.has_header("X-Test"));
}

TEST_F(HttpResponseTest, SetBody) {
    std::string body = "<html><body>Hello</body></html>";
    response.set_body(body);

    EXPECT_EQ(response.get_body(), body);
}

TEST_F(HttpResponseTest, AppendBody) {
    response.set_body("Hello");
    response.append_body(", ");
    response.append_body("World!");

    EXPECT_EQ(response.get_body(), "Hello, World!");
}

TEST_F(HttpResponseTest, ClearBody) {
    response.set_body("Some content");
    EXPECT_FALSE(response.get_body().empty());

    response.clear_body();
    EXPECT_TRUE(response.get_body().empty());
}

TEST_F(HttpResponseTest, ContentTypeHelpers) {
    response.set_json_content_type();
    EXPECT_EQ(response.get_header("Content-Type"), "application/json; charset=utf-8");

    response.set_html_content_type();
    EXPECT_EQ(response.get_header("Content-Type"), "text/html; charset=utf-8");

    response.set_text_content_type();
    EXPECT_EQ(response.get_header("Content-Type"), "text/plain; charset=utf-8");
}

TEST_F(HttpResponseTest, CustomContentType) {
    response.set_content_type("application/xml");
    EXPECT_EQ(response.get_header("Content-Type"), "application/xml");
}

TEST_F(HttpResponseTest, Redirect) {
    response.redirect("http://example.com/new-location");

    EXPECT_EQ(response.get_status_code(), HttpStatusCode::FOUND);
    EXPECT_EQ(response.get_header("Location"), "http://example.com/new-location");

    response.redirect("http://example.com/permanent", HttpStatusCode::MOVED_PERMANENTLY);
    EXPECT_EQ(response.get_status_code(), HttpStatusCode::MOVED_PERMANENTLY);
}

TEST_F(HttpResponseTest, CloseConnection) {
    response.set_close_connection(true);
    EXPECT_TRUE(response.close_connection());

    response.set_close_connection(false);
    EXPECT_FALSE(response.close_connection());
}

TEST_F(HttpResponseTest, Reset) {
    response.set_status_code(HttpStatusCode::NOT_FOUND);
    response.add_header("X-Custom", "value");
    response.set_body("Error message");
    response.set_close_connection(true);

    response.reset();

    EXPECT_EQ(response.get_status_code(), HttpStatusCode::OK);
    EXPECT_FALSE(response.has_header("X-Custom"));
    EXPECT_TRUE(response.get_body().empty());
    EXPECT_FALSE(response.close_connection());
}

TEST_F(HttpResponseTest, ToBuffer) {
    response.set_status_code(HttpStatusCode::OK);
    response.set_html_content_type();
    response.set_body("<html><body>Test</body></html>");

    std::string buffer = response.to_buffer();

    EXPECT_TRUE(buffer.find("HTTP/1.1 200 OK") != std::string::npos);
    EXPECT_TRUE(buffer.find("Content-Type: text/html") != std::string::npos);
    EXPECT_TRUE(buffer.find("Content-Length:") != std::string::npos);
    EXPECT_TRUE(buffer.find("<html><body>Test</body></html>") != std::string::npos);
}

TEST_F(HttpResponseTest, CopyConstructor) {
    response.set_status_code(HttpStatusCode::CREATED);
    response.add_header("X-Test", "value");
    response.set_body("Content");

    HttpResponse copied(response);

    EXPECT_EQ(copied.get_status_code(), HttpStatusCode::CREATED);
    EXPECT_EQ(copied.get_header("X-Test"), "value");
    EXPECT_EQ(copied.get_body(), "Content");
}

TEST_F(HttpResponseTest, MoveConstructor) {
    response.set_status_code(HttpStatusCode::ACCEPTED);
    response.set_body("Data");

    HttpResponse moved(std::move(response));

    EXPECT_EQ(moved.get_status_code(), HttpStatusCode::ACCEPTED);
    EXPECT_EQ(moved.get_body(), "Data");
}

TEST_F(HttpResponseTest, StreamId) {
    response.set_stream_id(456);
    EXPECT_EQ(response.get_stream_id(), 456);
}
