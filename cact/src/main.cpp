#include <iostream>
#include <fstream>
#include <any>
#include "antlr4-runtime.h"
#include "HelloLexer.h"
#include "HelloParser.h"
#include "../include/Analysis.h"
#include <vector>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "tree/ErrorNode.h"
#include "../include/MIR.h"

MIRProgram generateMIRFromLLVM(const llvm::Module &module);
void generateRISCV(const MIRProgram &mir, std::ostream &out);

using namespace antlr4;
class AnalysisTree : public antlr4::tree::ParseTreeListener {
    public:
        virtual void enterEveryRule(ParserRuleContext* ctx) override {}
        virtual void visitTerminal(tree::TerminalNode* node) override {}
        virtual void visitErrorNode(tree::ErrorNode* node) override {
            std::cout << "1" << std::endl;
            std::ofstream outfile("output.txt", std::ios::app); 
            if (outfile.is_open()) {
                outfile << "1" << std::endl;
                outfile.close();
            } else {
                std::cerr << "无法打开output.txt文件" << std::endl;
            }
            exit(1);}
        virtual void exitEveryRule(ParserRuleContext* ctx) override {}
    };
    
int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./compiler <input_file>" << std::endl;
        return -1;
    }

    // 打开输入文件


    std::ifstream stream;
    stream.open(argv[1]);
    ANTLRInputStream   input(stream);
    HelloLexer         lexer(&input);
    CommonTokenStream  tokens(&lexer);
    HelloParser        parser(&tokens);
    tree::ParseTree *tree = parser.compUnit();
    std::cout << "-------" << std::endl;
    std::cout << tree->toStringTree(&parser) << std::endl;
    AnalysisTree listener;
    tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    
    std::cout << "0" << std::endl;
    try {
        llvm::LLVMContext context;
        llvm::Module module("main", context);
        llvm::IRBuilder<> builder(context);
        // 进行语法分析并生成IR
         // 显示尝试将 Any 类型转换为字符串
        Analysis visitor(context, module, builder);

        antlrcpp::Any result = visitor.visit(tree); // 调用访问器生成IR  //显式声明类型为 antlrcpp::Any
        // 生成 MIR 并写入文件
        try {
            std::string llvmIR = std::any_cast<std::string>(result);
            std::cout << "IR 结果类型：" << typeid(result).name() << std::endl;  // 输出类型信息
            std::cout << "IR 结果内容：" << llvmIR << std::endl;
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误: 无法将结果转换为字符串类型" << std::endl;
        }
        try {
            std::string llvmIR = std::any_cast<std::string>(result);

            // 写入IR文件
            std::ofstream irFile("output.ll");
            irFile << llvmIR;

            // 写入成功标志
            std::ofstream("output.txt", std::ios::app) << "0\n";
            

        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("生成的IR不是字符串类型");
        }
        MIRProgram mir = generateMIRFromLLVM(module);
        dumpMIR(mir, "output.mir");




        std::ofstream riscvOut("output.s");
        generateRISCV(mir, riscvOut);
        riscvOut.close();
    
        std::cout << "RISC-V assembly generated to output.s\n";
        return 0;


    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        std::ofstream("output.txt", std::ios::app) << "1\n";
        return -1;
    }
}







class RegisterPool {
    std::vector<std::string> allRegs{"s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"};
    std::unordered_set<std::string> used;

public:
    std::string allocate() {
        for (const auto &reg : allRegs) {
            if (used.find(reg) == used.end()) {
                used.insert(reg);
                return reg;
            }
        }
        throw std::runtime_error("No available registers");
    }

    void release(const std::string &reg) {
        used.erase(reg);
    }

};
class FloatRegisterPool {
    std::vector<std::string> allFloatRegs{
        "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11"};
    std::unordered_set<std::string> usedFloat;
public:
    std::string allocateFloat() {
        for (const auto &reg : allFloatRegs) {
            if (usedFloat.find(reg) == usedFloat.end()) {
                usedFloat.insert(reg);
                return reg;
            }
        }
        throw std::runtime_error("No available float registers");
    }
    void releaseFloat(const std::string &reg) {
        usedFloat.erase(reg);
    }
};


class RegisterAllocator {
    RegisterPool &regPool;
    std::unordered_map<std::string, std::string> &vregMap;

public:
    RegisterAllocator(RegisterPool &pool, std::unordered_map<std::string, std::string> &map)
        : regPool(pool), vregMap(map) {}

        std::string mapVReg(const std::string &vreg) {
            if (vreg.empty() || vreg[0] != 'v') {
                // 非虚拟寄存器，直接返回原值
                return vreg;
            }
        
            // 是虚拟寄存器，检查是否已映射
            if (vregMap.count(vreg)) {
                return vregMap[vreg];
            }
        
            // 分配真实寄存器并映射
            std::string realReg = regPool.allocate();
            vregMap[vreg] = realReg;
            return realReg;
        }
        

    void releaseVReg(const std::string &vreg) {
        printf("释放寄存器 %s\n", vreg.c_str());
        if (vregMap.count(vreg)) {
            regPool.release(vregMap[vreg]);
            vregMap.erase(vreg);
        }
    }
};
class FloatRegisterAllocator {
    FloatRegisterPool &regPool;
    std::unordered_map<std::string, std::string> &vregMap;
public:
    FloatRegisterAllocator(FloatRegisterPool &pool, std::unordered_map<std::string, std::string> &map)
        : regPool(pool), vregMap(map) {}

    std::string mapVReg(const std::string &vreg) {
        if (vreg.empty() || vreg[0] != 'v') {
            // 非虚拟寄存器，直接返回原值
            return vreg;
        }   
        // 是虚拟寄存器，检查是否已映射
        if (vregMap.count(vreg)) {
            return vregMap[vreg];
        }
        // 分配真实寄存器并映射
        std::string realReg = regPool.allocateFloat();
        vregMap[vreg] = realReg;
        return realReg;
    }
    void releaseVReg(const std::string &vreg) {
        printf("释放浮点寄存器 %s\n", vreg.c_str());
        if (vregMap.count(vreg)) {
            regPool.releaseFloat(vregMap[vreg]);
            vregMap.erase(vreg);
        }
    }
};



bool willBeUsedInFutureBlocks(
    const std::vector<MIRBlock> &blocks,
    const std::string &currentBlockLabel,
    int currentInstrIndex,
    const std::string &vreg)
{
    bool inCurrentBlock = false;
    for (const auto &block : blocks) {
        if (block.label == currentBlockLabel) {
            inCurrentBlock = true;
            for (size_t i = currentInstrIndex + 1; i < block.instructions.size(); ++i) {
                const auto &instr = block.instructions[i];
                if (instr.dst == vreg && (instr.op == MOpcode::LI || instr.op == MOpcode::LA || instr.op == MOpcode::CALL))
                    return false;
                if (instr.src1 == vreg || instr.src2 == vreg)
                    return true;
                for (const auto &extra : instr.extraData) {
                    if (extra.name == vreg)
                        return true;
                }
            }
        } else if (inCurrentBlock) {
            // 之后的所有 block 全部遍历
            for (const auto &instr : block.instructions) {
                if (instr.dst == vreg && (instr.op == MOpcode::LI || instr.op == MOpcode::LA || instr.op == MOpcode::CALL))
                    
                    return false;
                if (instr.src1 == vreg || instr.src2 == vreg){
                    printf("vreg %s will be used in future blocks\n", vreg.c_str());
                    return true;
                }
                for (const auto &extra : instr.extraData) {
                    if (extra.name == vreg)
                        return true;
                }
            }
        }
    }
    printf("vreg %s will not be used in future blocks\n", vreg.c_str());
    return false;
}




void generateRISCV(const MIRProgram &mir, std::ostream &out) {
    // 1. Print global variables and extern functions
    bool hasDataSection = false;
    for (const auto &global : mir.globalInstructions) {
        switch (global.op) {
            case MOpcode::VAR_GLOBAL: {
                if (!hasDataSection) {
                    out << ".data\n";
                    hasDataSection = true;
                }

                const std::string &name = global.dst;
                const std::string &type = global.src1;
                const std::string &size = global.src2;
                std::vector<std::string> values;
                for(const auto &extra : global.extraData) {
                   values.push_back(extra.name);
                }

                out << ".size " << name << ", " << size << "\n";
                out << name << ":\n";

                std::string directive;
                if (type == "int" || type == "int_array") {
                    directive = ".word";
                } else if (type == "float" || type == "float_array") {
                    directive = ".float";
                } else if (type == "double") {
                    directive = ".double";
                } else {
                    directive = ".word"; // 默认情况
                }

                for (const auto &val : values) {
                    out << "  " << directive << " " << val << "\n";
                }

                break;
            }
            case MOpcode::EXTERN_FUNC:
                out << ".extern " << global.dst << "\n";
                break;
            default:
                break;
        }
    }

    for (const auto &flt : mir.staticFloats) {
        if(!hasDataSection) {
            out << ".data\n";
            hasDataSection = true;
        }
        const std::string &name = flt.symbol;
        const double value = flt.value;
        out << ".size " << name << ", 4\n"; // 假设静态浮点数大小为4字节
        out << name << ":\n";
        out << "  .float " << value << "\n";

    }

    out << "\n.text\n";

    // 2. Emit functions
    for (const auto &func : mir.functions) {
        out << ".globl " << func.name << "\n";
        out << func.name << ":\n";

        RegisterPool regPool;
        FloatRegisterPool floatRegPool;
        std::unordered_map<std::string, std::string> vregMap;
        std::unordered_map<std::string, std::string> floatVregMap;
        RegisterAllocator regAlloc(regPool, vregMap);
        FloatRegisterAllocator floatRegAlloc(floatRegPool, floatVregMap);

        for (const auto &block : func.blocks) {
            out << block.label << ":\n";

            const auto &instructions = block.instructions;
            for (size_t i = 0; i < instructions.size(); ++i) {
                const auto &instr = instructions[i];
                std::string dst, src1, src2;
                bool isfloat = false;

                if(instr.op == MOpcode::ADD_FLOAT || instr.op == MOpcode::SUB_FLOAT ||
                    instr.op == MOpcode::MUL_FLOAT || instr.op == MOpcode::DIV_FLOAT ||
                    instr.op == MOpcode::MOV_FLOAT || instr.op == MOpcode::STORE_FLOAT ||
                    instr.op == MOpcode::LOAD_FLOAT || instr.op == MOpcode::ICMP_EQ_FLOAT ||
                    instr.op == MOpcode::ICMP_NE_FLOAT || instr.op == MOpcode::ICMP_LT_FLOAT ||
                    instr.op == MOpcode::ICMP_LE_FLOAT || instr.op == MOpcode::ICMP_GT_FLOAT ||
                    instr.op == MOpcode::ICMP_GE_FLOAT
                 ) {
                    isfloat = true;
                }
                if(instr.isfloat){
                    isfloat = true; // 如果指令标记为浮点数，则设置 isfloat 为 true
                }
                if(isfloat && ( instr.op == MOpcode::STORE_FLOAT ||instr.op == MOpcode::LOAD_FLOAT )){
                    dst = floatRegAlloc.mapVReg(instr.dst);
                    src1 = instr.src1.empty() ? "" : regAlloc.mapVReg(instr.src1);
                    src2 = instr.src2.empty() ? "" : regAlloc.mapVReg(instr.src2);

                }else if(isfloat) {
                    dst = floatRegAlloc.mapVReg(instr.dst);
                    src1 = instr.src1.empty() ? "" : floatRegAlloc.mapVReg(instr.src1);
                    src2 = instr.src2.empty() ? "" : floatRegAlloc.mapVReg(instr.src2);

                }else {
                    dst = regAlloc.mapVReg(instr.dst);
                    src1 = instr.src1.empty() ? "" : regAlloc.mapVReg(instr.src1);
                    src2 = instr.src2.empty() ? "" : regAlloc.mapVReg(instr.src2);
                }
                

                switch (instr.op) {
                    case MOpcode::ADD_INT:
                        out << "  add " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::SUB_FLOAT:
                        out << "  fsub.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ADD_FLOAT:
                        out << "  fadd.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::SUB_INT:
                        out << "  sub " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    
                    case MOpcode::LOAD_INT:
                        out << "  lw " << dst << ", " << src1 << "(" << src2 << ")\n";
                        break;
                    case MOpcode::STORE_INT:
                        out << "  sw " << dst << ", " << src1 << "(" << src2 << ")\n";
                        if (!willBeUsedInFutureBlocks(func.blocks, block.label, i, instr.dst))
                            regAlloc.releaseVReg(instr.dst);
                        break;
                    case MOpcode::LOAD_FLOAT:
                        out << "  flw " << dst << ", " << src1 << "(" << src2 << ")\n";
                        break;
                    case MOpcode::STORE_FLOAT:
                        out << "  fsw " << dst << ", " << src1 << "(" << src2 << ")\n";
                        if (!willBeUsedInFutureBlocks(func.blocks, block.label, i, instr.dst))
                            floatRegAlloc.releaseVReg(instr.dst);
                        break;
                    case MOpcode::LOAD_CHAR:
                        out << "  lb " << dst << ", " << src1 << "(" << src2 << ")\n";
                        break;
                    case MOpcode::STORE_CHAR:
                        out << "  sb " << dst << ", " << src1 << "(" << src2 << ")\n";
                        if (!willBeUsedInFutureBlocks(func.blocks, block.label, i, instr.dst))
                            regAlloc.releaseVReg(instr.dst);
                        break;
                    case MOpcode::LI:
                        out << "  li " << dst << ", " << instr.src1 << "\n";
                        break;
                    case MOpcode::LA:
                        out << "  la " << dst << ", " << instr.src1 << "\n";
                        break;
                    case MOpcode::RET:
                        out << "  ret\n";
                        break;
                    case MOpcode::MOV_INT:
                        out << "  mv " << dst << ", " << src1 << "\n";
                        break;
                    case MOpcode::MOV_FLOAT:
                        out << "  fmv.s " << dst << ", " << src1 << "\n";
                        break;
                    case MOpcode::MOV_CHAR:
                        out << "  mv " << dst << ", " << src1 << "\n";
                        out << "  andi " << dst << ", " << dst << ", 0xFF\n"; // 截断为8位
                        break;
                    case MOpcode::MUL_INT:
                        out << "  mul " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::DIV_INT:
                        out << "  div " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::DIV_FLOAT:
                        out << "  fdiv.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::MUL_FLOAT:    
                        out << "  fmul.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::SREM:
                        out << "  rem " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    
                    case MOpcode::CALL: {
                        // src1: 返回值寄存器（如果无返回值则为空）
                        // src2: 函数名
                        // extra: 参数寄存器列表（std::vector<std::string>）

                        // 先将参数寄存器装载到约定的a0~a7寄存器
                        // 一般RISC-V前8个整型参数用a0-a7传递
                        const auto &argRegs = instr.extraData; // 假设extra是存储参数寄存器名的vector

                        // 参数个数最多8个，超过部分一般在栈上（这里暂不处理）
                        for (size_t i = 0; i < argRegs.size() && i < 8; ++i) {
                            if(argRegs[i].isFloat) {
                                // 如果是浮点参数，使用浮点寄存器
                                std::string argRegs_f = floatRegAlloc.mapVReg(argRegs[i].name);
                                out << "  fmv.s fa" << i << ", " << argRegs_f << "\n";
                            } else {
                                // 整型参数使用整数寄存器
                                std::string argRegs_i = regAlloc.mapVReg(argRegs[i].name);
                                out << "  mv a" << i << ", " << argRegs_i << "\n";
                            }
                        }

                        // 调用函数
                        out << "  call " << src1 << "\n";

                        // 返回值写回目标寄存器
                        // 如果有返回寄存器名
                        if (!dst.empty()) {
                            out << "  mv " << dst << ", a0\n";
                        }
                        break;
                    }
                    case MOpcode::JMP:
                        out << "  j " << instr.dst << "\n";
                        break;
                    case MOpcode::BEQ:
                        out << "  beq " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    case MOpcode::BNE:
                        out << "  bne " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    case MOpcode::BGT:
                        out << "  bgt " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    case MOpcode::BLT:
                        out << "  blt " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    case MOpcode::BGE:
                        out << "  bge " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    case MOpcode::BLE:
                        out << "  ble " << src1 << ", " << src2 << ", " << instr.dst << "\n";
                        break;
                    // === ICMP（整数或浮点比较） ===
                    case MOpcode::ICMP_EQ_INT:
                        out << "  sub " << dst << ", " << src1 << ", " << src2 << "\n";
                        out << "  seqz " << dst << ", " << dst << "\n"; // 相等：减法后为0，seqz判断
                        break;
                    case MOpcode::ICMP_NE_INT:
                        out << "  sub " << dst << ", " << src1 << ", " << src2 << "\n";
                        out << "  snez " << dst << ", " << dst << "\n"; // 不等
                        break;
                    case MOpcode::ICMP_LT_INT:
                        out << "  slt " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ICMP_LE_INT:
                        out << "  sgt " << dst << ", " << src1 << ", " << src2 << "\n";
                        out << "  xori " << dst << ", " << dst << ", 1\n"; // dst = !(src1 > src2)
                        break;
                    case MOpcode::ICMP_GT_INT:
                        out << "  sgt " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ICMP_GE_INT:
                        out << "  slt " << dst << ", " << src1 << ", " << src2 << "\n";
                        out << "  xori " << dst << ", " << dst << ", 1\n"; // dst = !(src1 < src2)
                        break;

                    // === 浮点数比较（使用浮点比较伪指令）===
                    case MOpcode::ICMP_EQ_FLOAT:
                        out << "  feq.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ICMP_NE_FLOAT:
                        out << "  feq.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        out << "  xori " << dst << ", " << dst << ", 1\n"; // 取反
                        break;
                    case MOpcode::ICMP_LT_FLOAT:
                        out << "  flt.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ICMP_LE_FLOAT:
                        out << "  fle.s " << dst << ", " << src1 << ", " << src2 << "\n";
                        break;
                    case MOpcode::ICMP_GT_FLOAT:
                        out << "  flt.s " << dst << ", " << src2 << ", " << src1 << "\n"; // 交换顺序
                        break;
                    case MOpcode::ICMP_GE_FLOAT:
                        out << "  fle.s " << dst << ", " << src2 << ", " << src1 << "\n"; // 交换顺序
                        break;

                    default:
                        out << "  # unknown opcode\n";
                        break;
                }
                

                // 寄存器释放策略
                
                if (!willBeUsedInFutureBlocks(func.blocks, block.label, i, instr.src1)) {
                        floatRegAlloc.releaseVReg(instr.src1);
                        regAlloc.releaseVReg(instr.src1);
                }
                
                if (!willBeUsedInFutureBlocks(func.blocks, block.label, i, instr.src2)){
                        floatRegAlloc.releaseVReg(instr.src2);
                        regAlloc.releaseVReg(instr.src2);
                    
                } 
            }
        }
    }
}