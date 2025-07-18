#!/bin/bash

# 设置文件名
SRC_FILE="../test/testcases-main/functional/00.cact"
ASM_FILE="test.s"
BIN_FILE="test"
STATIC_LIB="./libcact_rt.a"

# 第一步：编译 .cact 为 .s 汇编文件（不再 include 不存在的头文件）
echo "[*] 正在编译 C 源码为汇编..."
riscv32-unknown-linux-gnu-gcc -O3 -S -o $ASM_FILE -x c $SRC_FILE
if [ $? -ne 0 ]; then
    echo "[!] 源码编译为汇编失败"
    exit 1
fi

# 第二步：编译 .s 和静态库，链接为可执行文件
echo "[*] 正在将汇编链接为可执行文件..."
riscv32-unknown-linux-gnu-gcc -O3 -static -o $BIN_FILE $ASM_FILE $STATIC_LIB
if [ $? -ne 0 ]; then
    echo "[!] 汇编链接失败"
    exit 1
fi

# 第三步：用 spike 执行
echo "[*] 使用 Spike 执行程序..."
spike pk ./$BIN_FILE
exit_code=$?

echo "[*] 程序退出码：$exit_code"
