// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Accessible.java
// 状态：阶段 4 实现
// 备注：原版 sealed interface permits ClassFile, ClassMember。
//       C++ 抽象基类 + protected 构造表达 sealed 语义。
//       项目禁用 RTTI（无 dynamic_cast / typeid），不依赖运行时类型查询。
//       isXxx() marker 模式（参见 CpEntry）按需由派生类覆盖。

#pragma once

namespace jclassfile::model {

class Attribute;

class Accessible
{
public:
    virtual ~Accessible() = default;

    [[nodiscard]] virtual int getAccess() const     = 0;
    virtual void              setAccess(int access) = 0;

protected:
    Accessible() = default;
};

}   // namespace jclassfile::model
