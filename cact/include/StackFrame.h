// StackFrame.h
 
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include "MIR.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <cassert>
#include <memory>


#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"


struct Variable {
    std::string name;
    bool operator<(const Variable& other) const {
        return name < other.name;
    }
};

struct Register {
    std::string name;
    bool operator==(const Register& other) const {
        return name == other.name;
    }
};

struct StackFrame {
    int frameSize = 0;
    std::map<Variable, int> localVarOffsets;
    std::vector<Register> calleeSavedRegs;

    // 可选：打印调试信息
    void debugPrint() const;
};

void StackFrame::debugPrint() const {
    std::cout << "Frame size: " << frameSize << " bytes\n";
    std::cout << "Local variable offsets:\n";
    for (const auto& [var, offset] : localVarOffsets) {
        std::cout << "  " << var.name << ": " << offset << "(sp)\n";
    }
    std::cout << "Callee-saved registers:\n";
    for (const auto& reg : calleeSavedRegs) {
        std::cout << "  " << reg.name << "\n";
    }
}

class Stack {
    private:
        std::vector<StackFrame> frames;
    
    public:
        void push(const StackFrame& frame) {
            frames.push_back(frame);
        }
    
        void pop() {
            if (frames.empty()) throw std::runtime_error("Stack underflow");
            frames.pop_back();
        }
    
        StackFrame& top() {
            if (frames.empty()) throw std::runtime_error("Stack is empty");
            return frames.back();
        }
    
        const StackFrame& top() const {
            if (frames.empty()) throw std::runtime_error("Stack is empty");
            return frames.back();
        }
    
        bool empty() const {
            return frames.empty();
        }
    
        size_t size() const {
            return frames.size();
        }
    };

//这个是没有映射但是使用过的寄存器池，使得在代码中不会有一个寄存器名称不能释放
class TempVRegStack {
    std::vector<std::string> stack;

public:
    void push(const std::string &vreg) {
        stack.push_back(vreg);
    }

    size_t size() const {
        return stack.size();
    }
};


const Register SP = {"sp"};

void insertPrologue(const StackFrame &frame, std::vector<MInstruction> &mir) {
    // sp = sp - frameSize
    mir.emplace_back(MOpcode::ADD_INT, SP.name, SP.name, std::to_string(-frame.frameSize));

    int offset = frame.frameSize - 4;
    for (const auto &reg : frame.calleeSavedRegs) {
        // STORE_INT reg -> [sp + offset]
        // 格式：STORE_INT src_reg, offset, base_reg
        if(reg.name[0] == 'f') {
            // 如果是浮点寄存器，使用 STORE_FLOAT
            mir.emplace_back(MOpcode::STORE_FLOAT, reg.name, std::to_string(offset), SP.name);
        } else {
            // 否则使用 STORE_INT
            mir.emplace_back(MOpcode::STORE_INT, reg.name, std::to_string(offset), SP.name);
        }
        
        offset -= 4;
    }
}

void insertEpilogue(const StackFrame &frame, std::vector<MInstruction> &mir) {
    int offset = frame.frameSize - 4;
    for (const auto &reg : frame.calleeSavedRegs) {
        // LOAD_INT reg <- [sp + offset]
        if(reg.name[0] == 'f') {
            // 如果是浮点寄存器，使用 LOAD_FLOAT
            mir.emplace_back(MOpcode::LOAD_FLOAT, reg.name, std::to_string(offset), SP.name);
        } else {
            // 否则使用 LOAD_INT
            mir.emplace_back(MOpcode::LOAD_INT, reg.name, std::to_string(offset), SP.name);
        }
        offset -= 4;
    }

    // sp = sp + frameSize
    mir.emplace_back(MOpcode::ADD_INT, SP.name, SP.name, std::to_string(frame.frameSize));
}



std::vector<Register> getCalleeSavedRegisters(const llvm::Function &func) {
    std::vector<Register> result;
    result.push_back({"ra"});
    // 假设 main 函数不需要保存任何寄存器，其他函数保存 s0 和 s1
    if (func.getName() != "main") {

        result.push_back({"s0"});
        result.push_back({"s1"});
        result.push_back({"s2"});
        result.push_back({"s3"});
        result.push_back({"s4"});
        result.push_back({"s5"});
        result.push_back({"s6"});
        result.push_back({"s7"});
        result.push_back({"s8"});
        result.push_back({"s9"});  
        result.push_back({"s10"});
        result.push_back({"s11"});
        result.push_back({"fs1"});
        result.push_back({"fs2"});
        result.push_back({"fs3"});
        result.push_back({"fs4"});
        result.push_back({"fs5"});
        result.push_back({"fs6"});
        result.push_back({"fs7"});
        result.push_back({"fs8"});
        result.push_back({"fs9"});
        result.push_back({"fs10"});
        result.push_back({"fs11"});
    }

    return result;
}


int calculateLocalVarSize(const llvm::Function &func, StackFrame &frame) {
    const llvm::DataLayout &DL = func.getParent()->getDataLayout();

    int offset = 0;


    for (const auto &bb : func) {
        for (const auto &inst : bb) {
            if (const llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(&inst)) {
                llvm::Type *ty = alloca->getAllocatedType();

                // 使用DataLayout获得准确大小
                uint64_t size = DL.getTypeAllocSize(ty);

                // 4字节对齐（根据目标平台调整）
                if (size % 4 != 0)
                    size = ((size + 3) / 4) * 4;

                // 存储偏移，建议用 Value* 作为 key
                Variable var{ alloca->getName().str() };
                frame.localVarOffsets[var] = offset;

                offset += size;
            }
        }
    }
    offset += static_cast<int>(frame.calleeSavedRegs.size()) * 4;
    frame.frameSize = offset;
    return offset;
}

void allocateRegisters(const llvm::Function &func, std::unordered_map<const llvm::Value*, std::string> &valueToRegister) {
    int vregCounter = 0;

    for (const auto &bb : func) {
        for (const auto &inst : bb) {
            // 跳过 alloca 指令，因为它应该放到栈上管理
            if (llvm::isa<llvm::AllocaInst>(inst))
                continue;

            // 其他有返回值的指令为其分配虚拟寄存器
            if (!inst.getType()->isVoidTy()) {
                if (!valueToRegister.count(&inst)) {
                    valueToRegister[&inst] = "v" + std::to_string(vregCounter++);
                }
            }
        }
    }
}


void allocateParamRegisters(const llvm::Function &func, std::unordered_map<const llvm::Value*, std::string> &valueToRegister) {
    int vregCounter = 0;
    for (auto &arg : func.args()) {
        valueToRegister[&arg] = "a" + std::to_string(vregCounter++);  // 比如 a0, a1 用作参数寄存器
    }
}



void collectFloatConstants(
    const llvm::Module &module,
    std::vector<MIRfloat> &floatConstants)
{
    std::set<double> seen; // 避免重复

    int floatCounter = 0;

    // 遍历所有函数，找函数中使用的浮点常量
    for (const auto &func : module) {
        for (const auto &bb : func) {
            for (const auto &inst : bb) {
                // 你可以根据指令类型过滤，比如只看浮点指令里的浮点常量操作数
                for (unsigned i = 0; i < inst.getNumOperands(); ++i) {
                    if (auto *constFP = llvm::dyn_cast<llvm::ConstantFP>(inst.getOperand(i))) {
                        double val = constFP->getValueAPF().convertToDouble();

                        if (seen.insert(val).second) {
                            std::string sym = ".LC" + std::to_string(floatCounter++);
                            floatConstants.push_back({sym, val});
                        }
                    }
                }
            }
        }
    }
}

