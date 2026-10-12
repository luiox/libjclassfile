# libjclassfile

对齐 CAFED00D core 行为的 JVM classfile 读写库（C++17）：目标是稳定解析并
写回真实世界样本（含畸形样本），为上层字节码工具提供清晰可复用的 IO/模型/
变换接口。

- `src/jclassfile/io` — Reader/Writer 链路（含畸形容错、class builder）
- `src/jclassfile/model` — classfile 数据模型与 signature AST
- `src/jclassfile/transform` — 结构变换（如 illegal attribute stripping）
- `src/jclassfile/util` — 内部工具

## 构建

依赖 [libca](https://github.com/luiox/libca) 与
[libjstr](https://github.com/luiox/libjstr)（[luiox-repo](https://github.com/luiox/luiox-repo)
git 直连包，首次使用先注册包仓）：

```sh
xmake repo --add luiox-repo https://github.com/luiox/luiox-repo.git
xmake f --with_tests=y
xmake build -y libjclassfile_unittest
xmake run libjclassfile_unittest --gtest_brief=1
```

## License

MIT
