#include "networking/http/HttpMessageAssembler.h"
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
    pulse::net::HttpMessageAssembler assembler;
    char buffer[] = "??? /uri";
    int length = sizeof(buffer) - 1;

    pulse::net::HttpMessageAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, UriTooLong)
{
    pulse::net::HttpMessageAssembler assembler;
    char buffer[] = "GET /123567";
    int length = sizeof(buffer) - 1;

    assembler.setMaxRequestLineLength(4);
    pulse::net::HttpMessageAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);

    EXPECT_TRUE(result.error);
}

TEST(HttpParserTest, TotalHeaderBytesTooLong)
{
    pulse::net::HttpMessageAssembler assembler;
    char buffer[] = "GET /index.html HTTP/1.1\r\nContent-length: 23\r\nTooLarge: jasdajdasjd\r\nTooLarge: "
                    "jasdajdasjd\r\nTooLarge: jasdajdasjd\r\n";
    int length = sizeof(buffer) - 1;

    pulse::net::HttpMessageAssembler::AssemblingResult result = assembler.feed(1, buffer, length, 8096, length);
    EXPECT_FALSE(result.error);

    assembler.setMaxRequestHeaderBytes(sizeof(buffer) - 30);

    result = assembler.feed(2, buffer, length, 8096, length);
    EXPECT_TRUE(result.error);
}
