// MIR.h
#ifndef MIR_H
#define MIR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include <fstream>

enum class MOpcode {
    // 函数与标签
    LABEL,
    EXTERN_FUNC,
    VAR_GLOBAL,

    // 数据传输（加载/存储）
    LOAD_INT,   STORE_INT,     // int: lw / sw
    LOAD_FLOAT, STORE_FLOAT,   // float: flw / fsw
    LOAD_CHAR,  STORE_CHAR,    // char: lb / sb

    // 算术运算（类型特化）
    ADD_INT, SUB_INT, MUL_INT, DIV_INT,
    ADD_FLOAT, SUB_FLOAT, MUL_FLOAT, DIV_FLOAT,

    // 比较操作（整数）
    ICMP_EQ_INT, ICMP_NE_INT,
    ICMP_LT_INT, ICMP_LE_INT,
    ICMP_GT_INT, ICMP_GE_INT,

    // 比较操作（浮点） — 可选（如果你计划支持）
    ICMP_EQ_FLOAT, ICMP_NE_FLOAT,
    ICMP_LT_FLOAT, ICMP_LE_FLOAT,
    ICMP_GT_FLOAT, ICMP_GE_FLOAT,

    // 控制流
    CALL,
    RET,
    JMP,     // 无条件跳转
    BR,      // 条件跳转（配合 BEQ/BNE）

    // 条件跳转伪指令（控制流）
    BEQ, BNE, BGT, BLT, BGE, BLE,  // 可以扩展

    // 其他伪操作
    MOV_INT,       // int 类型寄存器赋值 -> mv
    MOV_FLOAT,     // float 类型寄存器赋值 -> fmv.s
    MOV_CHAR,      // char 类型（本质是 int8） -> mv + 截断（可选）
    GETPTR,       // 获取地址（GEP 等）
    LA ,             // Load Address（加载地址）
    LI ,            // Load Immediate（加载立即数）
    STATIC_FLOAT, // 静态浮点数
    SREM       // 取模（整数）
};

struct ExtraDatum {
    std::string name;
    bool isFloat;

    ExtraDatum(const std::string &name, bool isFloat)
        : name(name), isFloat(isFloat) {}
};

struct MInstruction {
    MOpcode op;
    std::string dst;
    std::string src1;
    std::string src2;
    std::vector<ExtraDatum> extraData;  // 用于全局变量初值
    bool isfloat;

    MInstruction(MOpcode op,
                 const std::string &dst = "",
                 const std::string &src1 = "",
                 const std::string &src2 = "",
                 const std::vector<ExtraDatum> &extraData = {},
                 bool isfloat = false)
        : op(op), dst(dst), src1(src1), src2(src2), extraData(extraData), isfloat(isfloat) {}
};


struct MIRBlock {
    std::string label;
    std::vector<MInstruction> instructions;

    void addInstruction(const MInstruction& inst) {
        instructions.push_back(inst);
    }
};

struct MIRFunction {
    std::string name;
    bool returnFloat; // 是否返回浮点数
    std::vector<MIRBlock> blocks;
};

struct MIRfloat {
    std::string symbol;  // 符号名，如 "@.float_const1"
    double value;        // 浮点数值
};

struct MIRProgram {
    std::vector<MIRFunction> functions;
    std::vector<MInstruction> globalInstructions; // 建议新增：保存全局变量和外部函数声明
    std::vector<MIRfloat> staticFloats; // 可选：保存静态浮点数
};



// 可选：将 MOpcode 映射为字符串
inline std::string opcodeToString(MOpcode op) {
    switch (op) {
        case MOpcode::MOV_CHAR: return "MOV_CHAR";
        case MOpcode::MOV_FLOAT: return "MOV_FLOAT";
        case MOpcode::MOV_INT: return "MOV_INT";
        case MOpcode::ADD_INT: return "ADD_INT";
        case MOpcode::SUB_INT: return "SUB_INT";
        case MOpcode::MUL_INT: return "MUL_INT";        
        case MOpcode::DIV_INT: return "DIV_INT";
        case MOpcode::ADD_FLOAT: return "ADD_FLOAT";
        case MOpcode::SUB_FLOAT: return "SUB_FLOAT";
        case MOpcode::MUL_FLOAT: return "MUL_FLOAT";
        case MOpcode::DIV_FLOAT: return "DIV_FLOAT";
        case MOpcode::ICMP_EQ_INT: return "ICMP_EQ_INT";
        case MOpcode::ICMP_NE_INT: return "ICMP_NE_INT";
        case MOpcode::ICMP_LT_INT: return "ICMP_LT_INT";
        case MOpcode::ICMP_LE_INT: return "ICMP_LE_INT";
        case MOpcode::ICMP_GT_INT: return "ICMP_GT_INT";
        case MOpcode::ICMP_GE_INT: return "ICMP_GE_INT";
        case MOpcode::ICMP_EQ_FLOAT: return "ICMP_EQ_FLOAT";
        case MOpcode::ICMP_NE_FLOAT: return "ICMP_NE_FLOAT";
        case MOpcode::ICMP_LT_FLOAT: return "ICMP_LT_FLOAT";
        case MOpcode::ICMP_LE_FLOAT: return "ICMP_LE_FLOAT";
        case MOpcode::ICMP_GT_FLOAT: return "ICMP_GT_FLOAT";
        case MOpcode::ICMP_GE_FLOAT: return "ICMP_GE_FLOAT";
        case MOpcode::LOAD_INT: return "LOAD_INT";
        case MOpcode::STORE_INT: return "STORE_INT";
        case MOpcode::LOAD_FLOAT: return "LOAD_FLOAT";
        case MOpcode::STORE_FLOAT: return "STORE_FLOAT";
        case MOpcode::LOAD_CHAR: return "LOAD_CHAR";
        case MOpcode::STORE_CHAR: return "STORE_CHAR";
        case MOpcode::CALL: return "CALL";
        case MOpcode::RET: return "RET";
        case MOpcode::JMP: return "JMP";
        case MOpcode::BEQ: return "BEQ";
        case MOpcode::BNE: return "BNE";
        case MOpcode::BGT: return "BGT";
        case MOpcode::BLT: return "BLT";
        case MOpcode::BGE: return "BGE";
        case MOpcode::BLE: return "BLE";
        case MOpcode::LABEL: return "LABEL";
        case MOpcode::EXTERN_FUNC: return "EXTERN_FUNC";
        case MOpcode::VAR_GLOBAL: return "VAR_GLOBAL";  
        case MOpcode::GETPTR: return "GETPTR";
        case MOpcode::LA: return "LA";
        case MOpcode::LI: return "LI";
        case MOpcode::STATIC_FLOAT: return "STATIC_FLOAT";
        case MOpcode::SREM: return "SREM";
        default: return "UNKNOWN";
    }
}
void dumpMIR(const MIRProgram& program, const std::string& filename);


#endif // MIR_H

