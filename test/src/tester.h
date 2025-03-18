/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#ifndef META_TEST_RUNNER
#define META_TEST_RUNNER

// Makes gtest aware of our custom types
#include <test_utils.h>

#include <core/intf_engine.h>
#include <core/log.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

namespace Test {
class LogLevelScope {
public:
    LogLevelScope(CORE_NS::ILogger* aLogger, CORE_NS::ILogger::LogLevel aLogLevel);
    ~LogLevelScope();

private:
    CORE_NS::ILogger& mLogger;
    CORE_NS::ILogger::LogLevel mOldLevel;
};

struct TestEnvironment {
    CORE_NS::IEngine* engine {};
};

TestEnvironment* getTestEnv();
} // namespace Test

META_END_NAMESPACE()

#endif
