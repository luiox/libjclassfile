#pragma once

// libjclassfile 测试门面：历史 test_helper API 形态（namespace test::、
// camelCase），内部委托 ca::test（#912 下沉的通用测试布局机制）——既有测试
// main 无需任何改动。仅保留本仓用到的内联子集。
//
// 使用约定：测试 target 须 set_rundir("$(projectdir)")，
// main 里先 test::setup("<子项目名>")。

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "libca/test/test.hpp"

namespace test {

inline void setup(const std::string& subproject)
{
    ca::test::setup(subproject);
}

inline std::filesystem::path getTopProjectPath()
{
    return ca::test::top_project_path();
}
inline std::filesystem::path getCurrentProjectPath()
{
    return ca::test::current_project_path();
}

inline std::filesystem::path getResourcePath(const std::string& filename)
{
    return ca::test::resource_path(filename);
}
inline bool hasResource(const std::string& filename)
{
    return ca::test::has_resource(filename);
}
inline std::vector<uint8_t> getResource(const std::string& filename)
{
    return ca::test::resource(filename);
}

}   // namespace test
