// 原版：ref_samples/CAFED00D/core/src/main/java/software/coley/cafedude/classfile/Descriptor.java
// 状态：对齐上游 62788a14（游标式解析，更多畸形判 Illegal）

#include "jclassfile/model/descriptor.hpp"

#include <algorithm>
#include <stdexcept>

namespace jclassfile::model {

const Descriptor Descriptor::VOID(Kind::Primitive, "V");
const Descriptor Descriptor::BOOLEAN(Kind::Primitive, "Z");
const Descriptor Descriptor::BYTE(Kind::Primitive, "B");
const Descriptor Descriptor::CHAR(Kind::Primitive, "C");
const Descriptor Descriptor::SHORT(Kind::Primitive, "S");
const Descriptor Descriptor::INT(Kind::Primitive, "I");
const Descriptor Descriptor::FLOAT(Kind::Primitive, "F");
const Descriptor Descriptor::DOUBLE(Kind::Primitive, "D");
const Descriptor Descriptor::LONG(Kind::Primitive, "J");
const Descriptor Descriptor::OBJECT(Kind::Object, "Ljava/lang/Object;");

Descriptor::Descriptor(std::string text)
    : text_(std::move(text))
{
    // Parse from text to determine kind and arrayLevel
    *this = from(text_);
}

int Descriptor::getParameterSize() const noexcept
{
    int size = 0;
    if (kind_ == Kind::Method) {
        std::size_t current = 1;
        std::size_t max     = text_.find(')');
        while (current < max) {
            char c = text_[current];
            if (c == 'J' || c == 'D') {
                size += 2;
                current++;
            }
            else if (c == 'L') {
                size++;
                auto end = text_.find(';', current + 2);
                if (end == std::string::npos) return -1;
                current = end + 1;
            }
            else if (c == '[') {
                while (current < max && text_[++current] == '[');
                c = text_[current];
                if (c == 'L') {
                    size++;
                    auto end = text_.find(';', current + 2);
                    if (end == std::string::npos) return -1;
                    current = end + 1;
                }
                else {
                    size++;
                    current++;
                }
            }
            else {
                size++;
                current++;
            }
        }
        return size;
    }
    // Non-method: primitive J/D = 2 slots, everything else = 1
    if (kind_ == Kind::Primitive && !text_.empty()) {
        char c = text_[0];
        return (c == 'J' || c == 'D') ? 2 : 1;
    }
    return 1;
}

int Descriptor::getParameterCount() const noexcept
{
    if (kind_ != Kind::Method) return -1;
    int         count   = 0;
    std::size_t current = 1;
    std::size_t max     = text_.find(')');
    while (current < max) {
        char c = text_[current];
        if (isPrimitive(c)) {
            count++;
            current++;
        }
        else if (c == 'L') {
            count++;
            auto end = text_.find(';', current + 2);
            if (end == std::string::npos) return -1;
            current = end + 1;
        }
        else if (c == '[') {
            while (current < max && text_[++current] == '[');
            if (current >= max) return -1;
            c = text_[current];
            if (isPrimitive(c)) {
                current++;
                count++;
            }
            else if (c == 'L') {
                count++;
                auto end = text_.find(';', current + 2);
                if (end == std::string::npos) return -1;
                current = end + 1;
            }
            else {
                return -1;
            }
        }
        else {
            return -1;
        }
    }
    return count;
}

std::vector<Descriptor> Descriptor::getParameters() const
{
    if (kind_ != Kind::Method) return {*this};
    std::vector<Descriptor> list;
    std::size_t             current = 1;
    std::size_t             max     = text_.find(')');
    if (max == 1) return list;
    while (current < max) {
        char c = text_[current];
        if (isPrimitive(c)) {
            list.push_back(from(c));
            current++;
        }
        else if (c == 'L') {
            auto end = text_.find(';', current + 2);
            if (end == std::string::npos) return list;
            list.push_back(from(text_.substr(current, end - current + 1)));
            current = end + 1;
        }
        else if (c == '[') {
            auto start = current;
            while (current < max && text_[++current] == '[');
            if (current >= max) return list;
            c = text_[current];
            if (isPrimitive(c)) {
                current++;
            }
            else if (c == 'L') {
                auto end = text_.find(';', current + 2);
                if (end == std::string::npos) return list;
                current = end + 1;
            }
            else {
                return list;
            }
            list.push_back(from(text_.substr(start, current - start)));
        }
        else {
            return list;
        }
    }
    return list;
}

Descriptor Descriptor::getReturnDesc() const
{
    if (kind_ == Kind::Method) {
        auto endPos = text_.find(')');
        if (endPos != std::string::npos) {
            return from(text_.substr(endPos + 1));
        }
    }
    return *this;
}

Descriptor Descriptor::getElementDesc() const
{
    if (arrayLevel_ == 0) return *this;
    return from(text_.substr(arrayLevel_));
}

bool Descriptor::isWide() const noexcept
{
    if (kind_ == Kind::Primitive) {
        char c = text_[0];
        return c == 'J' || c == 'D';
    }
    return false;
}

bool Descriptor::isPrimitive(const std::string& desc)
{
    return desc.length() == 1 && isPrimitive(desc[0]);
}

bool Descriptor::isPrimitive(char desc)
{
    switch (desc) {
    case 'V':
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
    case 'F':
    case 'D':
    case 'J': return true;
    default: return false;
    }
}

Descriptor Descriptor::from(char desc)
{
    switch (desc) {
    case 'V': return VOID;
    case 'Z': return BOOLEAN;
    case 'B': return BYTE;
    case 'C': return CHAR;
    case 'S': return SHORT;
    case 'I': return INT;
    case 'F': return FLOAT;
    case 'D': return DOUBLE;
    case 'J': return LONG;
    default: return Descriptor(Kind::Illegal, std::string(1, desc));
    }
}

Descriptor Descriptor::from(const std::string& desc)
{
    // 上游 62788a14：游标式解析，整串必须被完整消费；'V' 仅在单独出现或作为
    // 方法返回值时合法；方法描述符顺序解析参数表而非只找第一个 ')'。
    // 此前被宽松接受的畸形（"[V"、"VX"、"Lx;;y"、方法尾随数据等）现判 Illegal。
    if (desc.empty()) return Descriptor(Kind::Illegal, desc);
    if (desc == "V") return VOID;
    if (desc[0] == '(') return parseMethodDescriptor(desc);
    std::size_t     cursor = 0;
    std::optional<Descriptor> parsed = parseFieldType(desc, cursor);
    if (!parsed || cursor != desc.size()) return Descriptor(Kind::Illegal, desc);
    return *parsed;
}

bool Descriptor::isValidBinaryClassName(const std::string& name)
{
    for (char c : name) {
        if (c == '.' || c == '[' || c == '<' || c == '>') return false;
    }
    return true;
}

std::optional<Descriptor> Descriptor::parseFieldType(const std::string& desc,
                                                     std::size_t&       cursor)
{
    if (cursor >= desc.size()) return std::nullopt;
    const char c = desc[cursor];
    if (c == 'L') {
        const std::size_t end = desc.find(';', cursor + 1);
        // 缺终止符或 L 与 ; 之间为空类名均非法。
        if (end == std::string::npos || end == cursor + 1) return std::nullopt;
        if (!isValidBinaryClassName(desc.substr(cursor + 1, end - cursor - 1)))
            return std::nullopt;
        const std::string objectText = desc.substr(cursor, end - cursor + 1);
        cursor                       = end + 1;
        return Descriptor(Kind::Object, objectText);
    }
    if (c == '[') {
        const std::size_t start = cursor;
        int               depth = 0;
        while (cursor < desc.size() && desc[cursor] == '[') {
            depth++;
            cursor++;
        }
        if (depth > 255) return std::nullopt;   // JVMS 数组维度上限
        auto component = parseFieldType(desc, cursor);
        if (!component) return std::nullopt;
        return Descriptor(Kind::Array, desc.substr(start, cursor - start), depth);
    }
    if (c == 'V') return std::nullopt;   // void 只能作方法返回值
    Descriptor primitive = from(c);
    if (primitive.getKind() == Kind::Illegal) return std::nullopt;
    cursor += 1;
    return primitive;
}

Descriptor Descriptor::parseMethodDescriptor(const std::string& desc)
{
    std::size_t cursor = 1;   // 跳过 '('
    int         units  = 0;
    while (true) {
        if (cursor >= desc.size()) return Descriptor(Kind::Illegal, desc);   // 缺 ')'
        if (desc[cursor] == ')') {
            cursor++;
            break;
        }
        auto param = parseFieldType(desc, cursor);
        if (!param) return Descriptor(Kind::Illegal, desc);
        units += param->isWide() ? 2 : 1;
        if (units > 255) return Descriptor(Kind::Illegal, desc);   // JVMS 参数槽位上限
    }
    if (cursor >= desc.size()) return Descriptor(Kind::Illegal, desc);   // 缺返回类型
    if (desc[cursor] == 'V') {
        cursor++;
    }
    else if (!parseFieldType(desc, cursor)) {
        return Descriptor(Kind::Illegal, desc);
    }
    if (cursor != desc.size()) return Descriptor(Kind::Illegal, desc);   // 尾随数据
    return Descriptor(Kind::Method, desc);
}

}   // namespace jclassfile::model
