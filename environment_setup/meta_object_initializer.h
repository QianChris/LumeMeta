#ifndef TEST_ENVIRONMENT_META_OBJECT_INITIALIZE_H
#define TEST_ENVIRONMENT_META_OBJECT_INITIALIZE_H

#include <core/log.h>

namespace environmentSetup {

void InitializeMetaObject(const char* const dllPath);
void InitializeMetaObject(const char* const dllPath, CORE_NS::ILogger::LogLevel logLevel);
void ShutdownMetaObject();

} // namespace environmentSetup

#endif
