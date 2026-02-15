#include "networking/Utils.h"
#include <gtest/gtest.h>

TEST(NewtworkUtilsTest, TestReadToken)
{
    bool contains = pulse::net::Utils::containsToken("gzip,chunked,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("gzip, chunked ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("gzip,   chunked   ,  fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("  gzip  ,chunked  ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("  chunked ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("  fss, gzip,  chunked  ", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("   chunked ,fss    ", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("   gzip ,  chunked ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("gzip, chunkedd,fss", "chunked");
    EXPECT_FALSE(contains);
}

TEST(NewtworkUtilsTest, TestReadQuotedToken)
{
    bool contains = pulse::net::Utils::containsToken("gzip,\"chunked\",fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("gzip,\" chunked\",fss", "chunked");
    EXPECT_FALSE(contains);

    contains = pulse::net::Utils::containsToken("\"gzip\",\"chunked\",fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("  \"gzip\"  ,  \"chunked\"  ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("\"gzip\",chunked,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("\"gzip\"  ,  chunked  ,fss", "chunked");
    EXPECT_TRUE(contains);

    contains = pulse::net::Utils::containsToken("\"gzip\"chunked,fss", "chunked");
    EXPECT_FALSE(contains);
}
