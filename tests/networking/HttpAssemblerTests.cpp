#include "networking/http/HttpAssembler.h"
#include "utils/Logger.h"
#include <gtest/gtest.h>

//*************************************************************************************
//**********************        POSITIVE TESTS       **********************************
//*************************************************************************************

TEST(HttpParserTest, SuccessCompleteMessage)
{
    pulse::net::HttpAssembler assembler;

    for (int i = 0; i < 3; i++)
    {
        char buffer[] = "GET /aaa HTTP/1.1\r\n"
                        "content-length: 26\r\n"
                        "content-type: application/json\r\n"
                        "user-agent: PostmanRuntime/7.51.1\r\n"
                        "connection:keep-alive\r\n"
                        "accept-encoding : gzip, deflate, br\r\n"
                        "accept : */*\r\n"
                        "host: 127.0.0.1\r\n\r\n"
                        "{\r\n    \"text\" : \"hello\"\r\n}";

        int length = sizeof(buffer) - 1;

        pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

        EXPECT_FALSE(result.error);
        EXPECT_EQ(length, 0); // Everything consumed
        EXPECT_EQ(result.messages.size(), 1);
    }
}

TEST(HttpParserTest, SuccessAfterError)
{
    pulse::net::HttpAssembler assembler;

    char invalid_request[] = "??? /uri";
    int length = sizeof(invalid_request) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, invalid_request, length, 8096, length);

    EXPECT_TRUE(result.error);
    EXPECT_EQ(length, 0); // Everything thrown and reset

    char valid_request[] = "GET /aaa HTTP/1.1\r\n"
                           "content-length: 26\r\n"
                           "content-type: application/json\r\n"
                           "user-agent: PostmanRuntime/7.51.1\r\n"
                           "connection:keep-alive\r\n"
                           "accept-encoding : gzip, deflate, br\r\n"
                           "accept : */*\r\n"
                           "host: 127.0.0.1\r\n\r\n"
                           "{\r\n    \"text\" : \"hello\"\r\n}";

    length = sizeof(valid_request) - 1;

    result = assembler.feed(1, valid_request, length, 8096, length);

    EXPECT_FALSE(result.error);
    EXPECT_EQ(result.messages.size(), 1);
    EXPECT_EQ(length, 0); // Everything consumed
}

TEST(HttpParserTest, ConsecutiveHttpRequestsAtOnce)
{
    pulse::net::HttpAssembler assembler;

    char consecutive_requests[] = "GET /aaa HTTP/1.1\r\n"
                                  "content-length: 26\r\n"
                                  "content-type: application/json\r\n"
                                  "user-agent: PostmanRuntime/7.51.1\r\n"
                                  "connection:keep-alive\r\n"
                                  "accept-encoding : gzip, deflate, br\r\n"
                                  "accept : */*\r\n"
                                  "host: 127.0.0.1\r\n\r\n"
                                  "{\r\n    \"text\" : \"hello\"\r\n}"

                                  "GET /aaa HTTP/1.1\r\n"
                                  "content-length: 26\r\n"
                                  "content-type: application/json\r\n"
                                  "user-agent: PostmanRuntime/7.51.1\r\n"
                                  "connection:keep-alive\r\n"
                                  "accept-encoding : gzip, deflate, br\r\n"
                                  "accept : */*\r\n"
                                  "host: 127.0.0.1\r\n\r\n"
                                  "{\r\n    \"text\" : \"hello\"\r\n}";

    int length = sizeof(consecutive_requests) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, consecutive_requests, length, 8096, length);

    EXPECT_FALSE(result.error);
    EXPECT_EQ(result.messages.size(), 2);
    EXPECT_EQ(length, 0); // Everything consumed
}

TEST(HttpParserTest, SuccessStreamedMessage)
{
    pulse::net::HttpAssembler assembler;

    char buffer[] = "POST /aaa HTTP/1.1\r\n"
                    "content-length: 26\r\n"
                    "content-type: application/json\r\n"
                    "user-agent: PostmanRuntime/7.51.1\r\n"
                    "connection:keep-alive\r\n"
                    "accept-encoding : gzip, deflate, br\r\n"
                    "accept : */*\r\n"
                    "host: 127.0.0.1\r\n\r\n"
                    "{\r\n    \"text\" : \"hello\"\r\n}";

    int length = sizeof(buffer) - 1;

    for (int i = 0; i < length; i++)
    {
        int current_length = i + 1;
        int before_feeding_lenght = current_length;

        pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, current_length, 8096, 1);

        if (i < length - 1)
        {
            EXPECT_FALSE(result.error);
            EXPECT_EQ(current_length, before_feeding_lenght);
            EXPECT_EQ(result.messages.size(), 0);
        }
        else
        {
            EXPECT_FALSE(result.error);
            EXPECT_EQ(result.messages.size(), 1);
            EXPECT_EQ(current_length, 0);
        }
    }
}

//*************************************************************************************
//**********************        NEGATIVE TESTS       **********************************
//*************************************************************************************

TEST(HttpParserTest, InvalidMethod)
{
    pulse::net::HttpAssembler assembler;
    char buffer[] = "??? /uri";
    int length = sizeof(buffer) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, EmptyHeaderFieldName)
{
    pulse::net::HttpAssembler assembler;
    char buffer1[] = "GET / HTTP/1.1\r\n: chunked\r\n\r\nhello";
    int length1 = sizeof(buffer1) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer1, length1, 8096, length1);

    EXPECT_TRUE(result.error);

    char buffer2[] = "GET / HTTP/1.1\r\n: chunked\r\n\r\nhello";
    int length2 = sizeof(buffer2) - 1;

    result = assembler.feed(2, buffer2, length2, 8096, length2);

    EXPECT_TRUE(result.error);

    char buffer3[] = "GET / HTTP/1.1\r\n  : chunked\r\n\r\nhello";
    int length3 = sizeof(buffer3) - 1;

    result = assembler.feed(3, buffer3, length3, 8096, length3);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, EmptyHeaderFieldValue)
{
    pulse::net::HttpAssembler assembler;
    char buffer1[] = "GET / HTTP/1.1\r\nContent-Length:  \r\n\r\nhello";
    int length1 = sizeof(buffer1) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer1, length1, 8096, length1);

    EXPECT_TRUE(result.error);

    char buffer2[] = "GET / HTTP/1.1\r\nContent-Length: \r\n\r\nhello";
    int length2 = sizeof(buffer2) - 1;

    result = assembler.feed(2, buffer2, length2, 8096, length2);

    EXPECT_TRUE(result.error);

    char buffer3[] = "GET / HTTP/1.1\r\nContent-Length:\r\n\r\nhello";
    int length3 = sizeof(buffer3) - 1;

    result = assembler.feed(3, buffer3, length3, 8096, length3);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, UriTooLong)
{
    pulse::net::HttpAssembler assembler;
    char buffer[] = "GET /123567";
    int length = sizeof(buffer) - 1;

    assembler.setMaxRequestLineLength(4);
    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, TotalHeaderBytesTooLong)
{
    pulse::net::HttpAssembler assembler;
    char buffer[] = "GET /index.html HTTP/1.1\r\nContent-length: 23\r\nTooLarge: jasdajdasjd\r\nTooLarge: "
                    "jasdajdasjd\r\nTooLarge: jasdajdasjd\r\n";
    int length = sizeof(buffer) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);
    EXPECT_FALSE(result.error);

    assembler.setMaxRequestHeaderBytes(sizeof(buffer) - 30);

    result = assembler.feed(2, buffer, length, 8096, length);
    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, BodyTooLong)
{
    pulse::net::HttpAssembler assembler;

    char buffer[] = "GET /index.html HTTP/1.1\r\nContent-length: 9\r\n\r\n"
                    "Too large";
    int length = sizeof(buffer) - 1;

    pulse::net::HttpAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

    EXPECT_FALSE(result.error);

    assembler.setMaxBodySize(6);
    length = sizeof(buffer) - 1;

    result = assembler.feed(2, buffer, length, 8096, length);
    EXPECT_TRUE(result.error);
}
