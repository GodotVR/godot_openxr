// Facebook Technologies, LLC Proprietary and Confidential.

#include <OVR_LogUtils.h>

#include <gtest/gtest.h>

namespace OVR {
namespace {

TEST(LogUtilsTest, FatalTest) {
    EXPECT_DEATH(OVR_FAIL("death message"), "death message");
}

} // namespace
} // namespace OVR
