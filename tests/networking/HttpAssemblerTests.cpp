#include "networking/http/HttpAssembler.h"
#include "utils/Logger.h"
#include <gtest/gtest.h>

//*************************************************************************************
//**********************        POSITIVE TESTS       **********************************
//*************************************************************************************

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

    result = assembler.feed(2, buffer, length, 8096, length);
    EXPECT_TRUE(result.error);
}
