// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/VersionConstants.java
// 状态：阶段 3 实现
// 备注：JVM class file major version 常量。Java 1.0 = 45，Java 27 = 71。
//       与原版完全 1:1，对应 ClassFile.getVersionMajor()。

#pragma once

namespace jclassfile::model {

class VersionConstants
{
public:
    static constexpr int JAVA1  = 45;
    static constexpr int JAVA2  = 46;
    static constexpr int JAVA3  = 47;
    static constexpr int JAVA4  = 48;
    static constexpr int JAVA5  = 49;
    static constexpr int JAVA6  = 50;
    static constexpr int JAVA7  = 51;
    static constexpr int JAVA8  = 52;
    static constexpr int JAVA9  = 53;
    static constexpr int JAVA10 = 54;
    static constexpr int JAVA11 = 55;
    static constexpr int JAVA12 = 56;
    static constexpr int JAVA13 = 57;
    static constexpr int JAVA14 = 58;
    static constexpr int JAVA15 = 59;
    static constexpr int JAVA16 = 60;
    static constexpr int JAVA17 = 61;
    static constexpr int JAVA18 = 62;
    static constexpr int JAVA19 = 63;
    static constexpr int JAVA20 = 64;
    static constexpr int JAVA21 = 65;
    static constexpr int JAVA22 = 66;
    static constexpr int JAVA23 = 67;
    static constexpr int JAVA24 = 68;
    static constexpr int JAVA25 = 69;
    static constexpr int JAVA26 = 70;
    static constexpr int JAVA27 = 71;

    virtual ~VersionConstants() = default;
};

}   // namespace jclassfile::model
