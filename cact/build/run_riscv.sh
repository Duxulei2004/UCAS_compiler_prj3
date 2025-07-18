#!/bin/bash

# echo "[*] 正在编译..."
# riscv64-linux-gnu-gcc -static -o test_static output.s ./libcact_rt.a
# if [ $? -ne 0 ]; then
#     echo "[!] 编译失败"
#     exit 1
# fi

# echo "[*] 正在运行程序..."
# qemu-riscv64 ./test_static
# exit_code=$?

# echo "[*] 程序退出码：$exit_code"

#!/bin/bash

echo "[*] 正在编译..."
riscv32-unknown-linux-gnu-gcc -static -o test_static output.s ./libcact_rt.a
if [ $? -ne 0 ]; then
    echo "[!] 编译失败"
    exit 1
fi

echo "[*] 正在运行程序..."
spike pk ./test_static
exit_code=$?

echo "[*] 程序退出码：$exit_code"
