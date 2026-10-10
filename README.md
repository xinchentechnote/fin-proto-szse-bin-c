# fin-proto-szse-bin-c

深交所 Binary 交易数据接口规范 Ver1.29 的 C11 消息库,基于
[fin-proto-runtime-bin-c](https://github.com/xinchentechnote/fin-proto-runtime-bin-c) 运行时。
对应 Rust 版本:[fin-proto-szse-bin-rs](https://github.com/xinchentechnote/fin-proto-szse-bin-rs)。

`src/` 下的全部 `.h/.c` 文件由 [fin-protoc](https://github.com/xinchentechnote/fin-protoc)
从 `proto/szse_bin_v1.29.pdsl` 生成(`-C` 指定输出目录),请勿手改:

```bash
make compile    # fin-protoc -f proto/szse_bin_v1.29.pdsl -C src/
```

## 线上格式(全部大端)

```
SzseBinary:
┌───────────────┬────┬────────────────────────────────────────────────┐
│ MsgType u32   │ 4B │ 消息类型,决定 Body 的具体消息                     │
│ BodyLength u32│ 4B │ Body 字节数(不含头部与校验和)                    │
│ Body          │变长│ 14 种 Body 消息之一(部分 MsgType 为多值列表键),   │
│               │    │ 字段全部定长                                     │
│ Checksum i32  │ 4B │ SZSE_BIN 算法(i32 字节和 mod 256)               │
└───────────────┴────┴────────────────────────────────────────────────┘
```

NewOrder、ExecutionConfirm、ExecutionReport 内部还按 `ApplID` 字符串 match 选择
ExtendNNNNNN 扩展体(字符串多态)。

C 版以 `kind` 枚举 + 匿名 union 表达 match 多态(嵌套的 ApplExtend 同理):

```c
szse_binary_szse_binary msg;
memset(&msg, 0, sizeof(msg));
msg.body_tag = SZSE_BINARY_BODY_LOGON;
msg.body.logon.sender_comp_id = FC_STR("SENDER");
```

## 使用

```bash
make test      # 生成代码自带的 round-trip 测试(每个消息及扩展体一个)
make bench     # 与 Rust/Zig 同口径的编解码基准
```

与 fin-proto-szse-bin-rs 逐字节兼容(相同输入编码出相同报文)。解码零拷贝:
字符串借用输入缓冲,仅 `repeat` 字段分配外层数组,`<msg>_free` 负责释放。

## 依赖

编译时引用运行时头文件与源文件(可用 `RUNTIME=` 覆盖路径):

```make
RUNTIME ?= ../fin-proto-runtime-bin-c
$(CC) $(CFLAGS) -I$(RUNTIME)/include -Isrc src/*.c $(RUNTIME)/src/*.c ...
```
