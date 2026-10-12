# libclassfile unittest

本目录存放 libclassfile 单元测试与回归测试。

当前覆盖包含：
- 基础 parse/write/roundtrip。
- 结构化 Code 属性读写。
- 非法/保留 opcode 兼容 hook 与 ClassFile 级 transform。
- CAFED00D 回归集：`crasher-asm`（48）、`crasher-dude`（3）、`oak`（2）。
