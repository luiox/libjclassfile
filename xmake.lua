-- libjclassfile：对齐 CAFED00D core 行为的 JVM classfile 读写库（独立构建根）。
--
-- 依赖：
--   - libca（luiox-repo git 直连包）：core/str 为运行依赖，test 供测试门面
--     （test_helper 头委托 ca::test）；版本锁固定 0.0.13，升级须显式换版。
--   - libjstr（luiox-repo git 直连包）：MUTF-8 字符串层。
--   - gtest（xrepo，仅 --with_tests=y）。
--
-- 仓库注册：本文件不声明 add_repositories——包沙箱会继承宿主的仓库注册，
-- 在工程内重复注册同名仓库会导致嵌套包构建时 remote 路径错乱。独立开发/CI
-- 先执行：
--   xmake repo --add luiox-repo https://github.com/luiox/luiox-repo.git

set_languages("cxx17")
add_rules("mode.debug", "mode.release")

LUIOX_LIBCA_VERSION = "0.0.13"
add_requires("libca " .. LUIOX_LIBCA_VERSION, {configs = {modules = "core,str,log,crypto,zip,test"}})
add_requires("libjstr 0.0.1")
add_requireconfs("libjstr.libca", {version = LUIOX_LIBCA_VERSION})

option("with_tests")
    set_default(false)
    set_showmenu(true)
    set_description("Enable unit tests (pulls gtest)")
option_end()

if has_config("with_tests") then
    add_requires("gtest")
end

target("libjclassfile")
    set_kind("static")
    add_packages("libjstr", {public = true})
    add_files("src/jclassfile/io/*.cpp")
    add_files("src/jclassfile/transform/*.cpp")
    add_files("src/jclassfile/model/*.cpp")
    add_files("src/jclassfile/model/**/*.cpp")
    add_files("src/jclassfile/util/*.cpp")
    add_headerfiles("src/(jclassfile/**.hpp)")
    add_includedirs("src", {public = true})
    if is_plat("windows") then
        add_cxflags("/utf-8", {tools = "cl"})
    end

if has_config("with_tests") then

-- 测试门面：test::setup/getResource* 内联委托 ca::test（见 test_helper/）。
target("libjclassfile_unittest")
    set_kind("binary")
    set_default(false)
    add_deps("libjclassfile")
    add_packages("libjstr")
    add_packages("libca")
    add_packages("gtest")
    add_includedirs("test_helper")
    add_files("test/unittest/main.cpp")
    add_files("test/unittest/**/*_test.cpp")
    add_files("test/unittest/equality_io_test.cpp")
    add_files("test/unittest/desc_test.cpp")
    add_files("test/unittest/size_test.cpp")
    add_files("test/unittest/crasher_patching_test.cpp")
    add_files("test/unittest/oak_patching_test.cpp")
    add_files("test/unittest/regression_tests.cpp")
    add_files("test/unittest/model/signature_ast_test.cpp")
    add_includedirs("src")
    set_rundir("$(projectdir)")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
    if is_plat("windows") then
        add_cxflags("/utf-8", {tools = "cl"})
    end

target("libjclassfile_origintest")
    set_kind("binary")
    set_default(false)
    add_deps("libjclassfile")
    add_packages("libjstr")
    add_packages("libca")
    add_packages("gtest")
    add_includedirs("test_helper")
    add_files("test/origin/main.cpp")
    add_files("test/origin/*_test.cpp")
    add_files("test/origin/regression_tests.cpp")
    add_includedirs("src")
    set_rundir("$(projectdir)")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
    if is_plat("windows") then
        add_cxflags("/utf-8", {tools = "cl"})
    end

target("libjclassfile_roundtriptest")
    set_kind("binary")
    set_default(false)
    add_deps("libjclassfile")
    add_packages("libjstr")
    add_packages("libca")
    add_packages("gtest")
    add_includedirs("test_helper")
    add_files("test/roundtrip/main.cpp")
    add_files("test/roundtrip/*_test.cpp")
    add_includedirs("src")
    set_rundir("$(projectdir)")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
    if is_plat("windows") then
        add_cxflags("/utf-8", {tools = "cl"})
    end

end
