cmake_minimum_required(VERSION 3.18)
project(MetaAPITestRunnerOHOS)

get_filename_component(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../" ABSOLUTE)

# Need to add the actual source cmakelists.txt via this, as otherwise deveco project view will not be properly populated.
# Root cmakelistfile with "unit_tests = off" so that we don't add both api and src tests
add_subdirectory(${BASE_DIR} LumeMeta)

# Add API unit tests
enable_testing()
add_subdirectory(${BASE_DIR}/test/unittest/api_unit_test LumeMetaAPITest)
add_test(NAME LumeMetaAPI COMMAND LumeMetaAPITestRunner)
