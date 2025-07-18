// generateMachineIR.cpp
#include "../include/MIR.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <regex>
#include <string>
#include <cassert>
#include "../include/StackFrame.h"
// 类型声明
MIRProgram generateMIRFromLLVM(const llvm::Module &module);
TempVRegStack TEMPReg;

// 函数声明
MIRFunction processFunction(const llvm::Function &func,MIRProgram &program);
void processInstruction(const llvm::Instruction &inst,
    MIRBlock &mirBlock,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);

void handleBinaryOp(const llvm::BinaryOperator *bin,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
void handleReturnInst(const llvm::ReturnInst *ret,
    MIRBlock &mirBlock,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
void handleCallInst(const llvm::CallInst *call,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
void handleBranchInst(const llvm::BranchInst *br,
    const llvm::Function &func,
    std::vector<MInstruction> &mir // 可选：frame/register 映射
    ,const StackFrame &frame,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,MIRProgram &program
    );  
void handleICmpInst(const llvm::ICmpInst *icmp,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
void handleLoadInst(const llvm::LoadInst *load,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
void handleStoreInst(const llvm::StoreInst *store,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);   
void handleGetElementPtrInst(const llvm::GetElementPtrInst *gep,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program);
std::string getOperandStr(
    const llvm::Value *val,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,
    std::vector<MInstruction> &mir,
    MIRProgram &program
);

// 可选：工具函数声明

std::string typeToString(const llvm::Type *type) {

    if (type->isVoidTy()) return "void";
    if (type->isIntegerTy(32)) return "int";
    if (type->isIntegerTy(8))  return "char";
    if (type->isFloatTy())     return "float";
    if (type->isDoubleTy())    return "double";

    // 其他类型，指针、数组等你可以扩展
    return "unknown";
}

std::vector<ExtraDatum> wrapAsExtraData(const std::vector<std::string> &args, bool isFloat = false) {
    std::vector<ExtraDatum> result;
    for (const auto &s : args) {
        result.emplace_back(s, isFloat);
    }
    return result;
}


MIRProgram generateMIRFromLLVM(const llvm::Module &module) {
    MIRProgram program;

    //collectFloatConstants(module, program.staticFloats);

    // 1. 处理全局变量
    for (const auto &globalVar : module.globals()) {
        std::string name = globalVar.getName().str();
        std::string sizeStr = "4"; // 默认字节数
        std::vector<ExtraDatum> initialValues;
        std::string typeHint = "int"; // 默认为整数类型
        bool isFloat = false;
        if (globalVar.hasInitializer()) {
            const llvm::Constant *init = globalVar.getInitializer();
            llvm::errs() << "Global var: " << name << " has initializer: ";
            init->print(llvm::errs());
            llvm::errs() << "\n";
            llvm::errs() << "Initializer type: " << *init->getType() << "\n";
  
            // 整数常量
            if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(init)) {
                llvm::SmallString<16> strBuf;
                CI->getValue().toString(strBuf, 10, true, false);
                initialValues.push_back({std::string(strBuf.begin(), strBuf.end()), isFloat});
                typeHint = "int";
                sizeStr = "4";
            }
    
            else if (auto *CFP = llvm::dyn_cast<llvm::ConstantFP>(init)) {
                llvm::APFloat apf = CFP->getValueAPF();
                std::string strVal;
                isFloat = CFP->getType()->isFloatTy();  // ←判断是否是 float 类型
            
                llvm::SmallVector<char, 32> strBuf;
            
                if (isFloat) {
                    // 使用 float 精度（IEEE single）转换为字符串
                    llvm::APFloat apf_f(apf); // 拷贝
                    bool losesInfo;
                    apf_f.convert(llvm::APFloat::IEEEsingle(), llvm::APFloat::rmNearestTiesToEven, &losesInfo);
                    apf_f.toString(strBuf, 6);  // 保留6位小数，够 float 精度
                    typeHint = "float";
                    sizeStr = "4";
                } else {
                    // double 类型，直接保留精度输出
                    apf.toString(strBuf, 17);  // double 精度最大是17位十进制有效数字
                    typeHint = "double";
                    sizeStr = "8";
                }
            
                strVal = std::string(strBuf.begin(), strBuf.end());
            
                initialValues.push_back({strVal, isFloat});
            }
            
    
            // 整数数组（如 .word 0,1,2,...）
            else if (auto *CA = llvm::dyn_cast<llvm::ConstantArray>(init)) {
                bool isFloatArray = false;
                for (unsigned i = 0; i < CA->getNumOperands(); ++i) {
                    const llvm::Constant *elem = CA->getOperand(i);
    
                    if (auto *elemInt = llvm::dyn_cast<llvm::ConstantInt>(elem)) {
                        llvm::SmallString<16> strBuf;
                        elemInt->getValue().toString(strBuf, 10, true, false);
                        initialValues.push_back({std::string(strBuf.begin(), strBuf.end()), isFloat});
                        typeHint = "int_array";
                    } else if (auto *elemFloat = llvm::dyn_cast<llvm::ConstantFP>(elem)) {
                        isFloat = true;
                        llvm::SmallVector<char, 16> strBuf;
                        elemFloat->getValueAPF().toString(strBuf, 6);
                        initialValues.push_back({std::string(strBuf.begin(), strBuf.end()), isFloat});
                        typeHint = "float_array";
                        isFloatArray = true;
                    }
                }
                sizeStr = std::to_string((isFloatArray ? 4 : 4) * initialValues.size());
            }
            else if (auto *CDA = llvm::dyn_cast<llvm::ConstantDataArray>(init)) {
                if (CDA->isString()) {
                    // 是字符串常量，比如 "hello"
                    std::string str = CDA->getAsCString().str();
                    for (char ch : str) {
                        initialValues.push_back({std::to_string(static_cast<int>(ch)), isFloat});
                    }
                    typeHint = "char_array";
                    sizeStr = std::to_string(str.size());
                } else if (CDA->getElementType()->isIntegerTy(32)) {
                    for (unsigned i = 0; i < CDA->getNumElements(); ++i) {
                        initialValues.push_back({std::to_string(CDA->getElementAsInteger(i)), isFloat});
                    }
                    typeHint = "int_array";
                    sizeStr = std::to_string(4 * CDA->getNumElements());
                } else if (CDA->getElementType()->isFloatTy()) {
                    isFloat = true;
                    for (unsigned i = 0; i < CDA->getNumElements(); ++i) {
                        float val = CDA->getElementAsFloat(i);
                        initialValues.push_back({std::to_string(val), isFloat});
                    }
                    typeHint = "float_array";
                    sizeStr = std::to_string(4 * CDA->getNumElements());
                }
            }
            
        } else {
            if(globalVar.getType()->isFloatingPointTy()) {
                isFloat = true; // 如果没有初始值，默认设置为浮点数
            }
            initialValues.push_back({"0", isFloat}); // 如果没有初始值，默认设置为0
        }
    
        // 将 typeHint 填入 src1，用于标识类型（例如 "int", "float"）
        program.globalInstructions.emplace_back(
            MOpcode::VAR_GLOBAL,
            name,
            typeHint,     // src1: 类型标识
            sizeStr,      // src2: 大小
            initialValues,
            isFloat       // 是否为浮点数
        );
    }
    
    // 2. 处理函数声明和定义
    for (const auto &func : module) {
        if (func.isDeclaration()) {
            // 外部函数声明
            std::string name = func.getName().str();
            std::string retType = typeToString(func.getReturnType());
            std::string argTypes = "";
            for (auto &arg : func.args()) {
                if (!argTypes.empty()) argTypes += ",";
                argTypes += typeToString(arg.getType());
            }
            program.globalInstructions.emplace_back(
                MOpcode::EXTERN_FUNC,
                name,
                retType,
                argTypes
            );
        } else {
            // 函数定义
            MIRFunction mirFunc = processFunction(func,program);
            program.functions.push_back(std::move(mirFunc));
        }
    }

    return program;
}



MIRFunction processFunction(const llvm::Function &func,MIRProgram &program) {
    
    
    MIRFunction mirFunc;
    mirFunc.name = func.getName().str();
    mirFunc.returnFloat = func.getReturnType()->isFloatingPointTy(); // 检查返回类型是否为浮点数
    std::unordered_map<const llvm::Value*, std::string> valueToRegister;
    allocateRegisters(func, valueToRegister); 

    // 1. 构造栈帧
    StackFrame frame;

    frame.calleeSavedRegs = getCalleeSavedRegisters(func);
    int localVarSize = calculateLocalVarSize(func,frame);
    frame.frameSize = localVarSize;

    frame.debugPrint();

    int argIndex = 0;
    for (const llvm::Argument &arg : func.args()) {
        std::string argReg;
        if(arg.getType()->isFloatingPointTy()) {
            argReg = "fa" + std::to_string(argIndex++);
        } else {
            argReg = "a" + std::to_string(argIndex++);
        }
        valueToRegister[&arg] = argReg;
    }
    
    // 2. 创建函数入口块，并插入 Prologue
    MIRBlock entryBlock;
    entryBlock.label = mirFunc.name + "_enterpoint";
    insertPrologue(frame, entryBlock.instructions);
    mirFunc.blocks.push_back(std::move(entryBlock));

    static int bbCounter = 0;
    // 3. 处理函数体的每个基本块
    for (const auto &bb : func) {
        // 生成基本块的 MIRBlock
        MIRBlock mirBlock;
        std::string bbName = bb.getName().str();
        bbName = mirFunc.name + "_" + bbName;
        mirBlock.label = bbName;

        // 这里假设你有个 processInstruction 函数往 mirBlock.instructions 里添加指令
        for (const auto &inst : bb) {
            processInstruction(inst, mirBlock,
                valueToRegister, frame,program);
        }
        mirFunc.blocks.push_back(std::move(mirBlock));
    }

    // 4. 创建函数结束块，插入 Epilogue 和 Ret
    bool hasReturn = false;
    // 遍历基本块和指令，检查是否存在 ReturnInst
    for (const auto &bb : func) {
        for (const auto &inst : bb) {
            if (llvm::isa<llvm::ReturnInst>(inst)) {
                hasReturn = true;
                break;
            }
        }
        if (hasReturn) break;
    }
    
    if (!hasReturn) {
        MIRBlock exitBlock;
        exitBlock.label = mirFunc.name + "_exit";
        insertEpilogue(frame, exitBlock.instructions);
        exitBlock.instructions.emplace_back(MOpcode::RET);
        mirFunc.blocks.push_back(std::move(exitBlock));
    }
    
    return mirFunc;
}


void processInstruction(const llvm::Instruction &inst,
    MIRBlock &mirBlock,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program){
        if (const auto *bin = llvm::dyn_cast<llvm::BinaryOperator>(&inst)) {
            handleBinaryOp(bin, mirBlock.instructions, valueToRegister, frame,program);
        } else if (const auto *ret = llvm::dyn_cast<llvm::ReturnInst>(&inst)) {
            handleReturnInst(ret, mirBlock, valueToRegister, frame,program);
        } 
        else if (const auto *call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
            handleCallInst(call, mirBlock.instructions, valueToRegister, frame,program);
        } 
        else if (const auto *br = llvm::dyn_cast<llvm::BranchInst>(&inst)) {
            handleBranchInst(br, *inst.getFunction(), mirBlock.instructions,frame,valueToRegister,program);  // frame/register 映射可选传入
        } 
        else if (const auto *icmp = llvm::dyn_cast<llvm::ICmpInst>(&inst)) {
            handleICmpInst(icmp, mirBlock.instructions, valueToRegister, frame,program);
        } 
        else if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst)) {
            handleLoadInst(load, mirBlock.instructions, valueToRegister, frame,program);
        } 
        else if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst)) {
            handleStoreInst(store, mirBlock.instructions, valueToRegister, frame,program);
        } 
        else if (const auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
            handleGetElementPtrInst(gep, mirBlock.instructions, valueToRegister, frame,program);
        }
}

bool parseStackOffset(const std::string &addrStr, std::string &offset, std::string &baseReg) {
    static const std::regex pattern(R"((\d+)\((\w+)\))"); // 匹配数字(字母数字)
    std::smatch match;
    if (std::regex_match(addrStr, match, pattern)) {
        offset = match[1].str();
        baseReg = match[2].str();
        return true;
    }
    return false;
}


void handleBinaryOp(const llvm::BinaryOperator *bin,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program) {

    using namespace llvm;

    const Value *lhs = bin->getOperand(0);
    const Value *rhs = bin->getOperand(1);
    const Type *ty = bin->getType();

    MOpcode opcode;

    // 判断类型是 int / float / char（这里只区分 int32, float, int8）
    bool isInt = ty->isIntegerTy(32);
    bool isChar = ty->isIntegerTy(8);
    bool isFloat = ty->isFloatTy();

    switch (bin->getOpcode()) {
        case Instruction::Add:
            opcode = isFloat ? MOpcode::ADD_FLOAT :
                     isInt   ? MOpcode::ADD_INT :
                               MOpcode::ADD_INT;  // char也当作int处理
            break;
        case Instruction::Sub:
            opcode = isFloat ? MOpcode::SUB_FLOAT :
                     isInt   ? MOpcode::SUB_INT :
                               MOpcode::SUB_INT;
            break;
        case Instruction::Mul:
            opcode = isFloat ? MOpcode::MUL_FLOAT :
                     isInt   ? MOpcode::MUL_INT :
                               MOpcode::MUL_INT;
            break;
        case Instruction::SDiv:
        case Instruction::UDiv:
            opcode = isFloat ? MOpcode::DIV_FLOAT :
                     isInt   ? MOpcode::DIV_INT :
                               MOpcode::DIV_INT;
            break;
        case Instruction::SRem:
            opcode =  MOpcode::SREM ;
            break;
        default:
            std::cerr << "[MIR] 未支持的 BinaryOperator 类型: " << bin->getOpcodeName() << "\n";
            return;
    }


    std::string dst  = getOperandStr(bin, valueToRegister, frame, mir,program);
    std::string src1 = getOperandStr(lhs, valueToRegister, frame, mir,program);
    std::string src2 = getOperandStr(rhs, valueToRegister, frame, mir,program);

    if (llvm::isa<llvm::GlobalVariable>(lhs)) {
        std::cout << "Global variable detected in lhs: " << lhs->getName().str() << std::endl;
        std::string loadedLhs = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(loadedLhs); // 将临时寄存器加入栈
        if(lhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, loadedLhs, "0", src1);
        } else if(lhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, loadedLhs, "0", src1,                 
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if(lhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, loadedLhs, "0", src1);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, loadedLhs, "0", src1);
        }
        //valueToRegister[lhs] = loadedLhs; // 更新映射
        valueToRegister.erase(lhs); // 删除原有映射
        src1 = loadedLhs;
    }
    
    if (llvm::isa<llvm::GlobalVariable>(rhs)) {
        std::string loadedRhs = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(loadedRhs); // 将临时寄存器加入栈
        if(rhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, loadedRhs, "0", src2);
        } else if(rhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, loadedRhs, "0", src2,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if(rhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, loadedRhs, "0", src2);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, loadedRhs, "0", src2);
        }
        //valueToRegister[rhs] = loadedRhs; // 更新映射
        valueToRegister.erase(rhs); // 删除原有映射
        src2 = loadedRhs;
    }

    llvm::Type* type1 = lhs->getType()->isPointerTy() ? lhs->getType()->getPointerElementType() : lhs->getType();
    llvm::Type* type2 = rhs->getType()->isPointerTy() ? rhs->getType()->getPointerElementType() : rhs->getType();
    
    std::string typeStr1;
    llvm::raw_string_ostream rso1(typeStr1);
    type1->print(rso1);
    rso1.flush();
    
    std::string typeStr2;
    llvm::raw_string_ostream rso2(typeStr2);
    type2->print(rso2);
    rso2.flush();
    
    std::cout << "type of src1: " << typeStr1 << std::endl;
    std::cout << "type of src2: " << typeStr2 << std::endl;
    

    
    // 判断字符串是否是栈偏移地址格式，调用你已有的 parseStackOffset
    std::string offset, baseReg;

    // 处理 src1
    if (parseStackOffset(src1, offset, baseReg)) {
        int vregCounter = valueToRegister.size();
        std::string tmpReg1 = "v" + std::to_string(vregCounter + TEMPReg.size());
        if(lhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg1, offset, baseReg);
        }else if(lhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tmpReg1, offset, baseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if(lhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            // 如果是8位整型，加载后可能需要扩展
            mir.emplace_back(MOpcode::LOAD_CHAR, tmpReg1, offset, baseReg);
            // TODO: 如果后续操作是32位整数加法，可能需要生成扩展指令
        } else {
            // 默认当整数处理
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg1, offset, baseReg);
        }
        
        src1 = tmpReg1;
        //valueToRegister[lhs] = tmpReg1;
    }
    offset = "0";
    baseReg = "0";
    // 处理 src2
    if (parseStackOffset(src2, offset, baseReg)) {
        int vregCounter = valueToRegister.size();
        std::string tmpReg2 = "v" + std::to_string(vregCounter + TEMPReg.size());
        if(rhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg2, offset, baseReg);
        }else if(rhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tmpReg2, offset, baseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if(rhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, tmpReg2, offset, baseReg);
            // TODO: 如果后续操作是32位整数加法，可能需要生成扩展指令
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg2, offset, baseReg);
        }
        src2 = tmpReg2;
        //valueToRegister[rhs] = tmpReg2;
    }

    // 处理 dst
    bool dstIsMemory = false;
    std::string dstOffset, dstBaseReg;
    if (parseStackOffset(dst, dstOffset, dstBaseReg)) {
        // dst 是内存地址，不能直接作为运算结果寄存器
        dstIsMemory = true;
        int vregCounter = valueToRegister.size();
        std::string tmpDstReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(tmpDstReg); // 将临时寄存器加入栈

        mir.emplace_back(opcode, tmpDstReg, src1, src2);

        // 将结果存回内存地址 dst
        if (ty->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::STORE_INT, tmpDstReg, dstOffset, dstBaseReg);
        } else if (ty->isFloatTy()) {
            mir.emplace_back(MOpcode::STORE_FLOAT, tmpDstReg, dstOffset, dstBaseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else {
            mir.emplace_back(MOpcode::STORE_INT, tmpDstReg, dstOffset, dstBaseReg);
        }

        // 注册临时寄存器映射（如果后续需要）
        //valueToRegister[bin] = tmpDstReg;

        // 处理结束，直接return
        return;
    }

    mir.emplace_back(opcode, dst, src1, src2,
        std::vector<ExtraDatum>(), // 没有额外数据
        isFloat // 是否为浮点数
    );

    // 记得更新寄存器映射
    valueToRegister[bin] = dst;
}

void handleReturnInst(const llvm::ReturnInst *retInst, MIRBlock &mirBlock, 
                      std::unordered_map<const llvm::Value*, std::string> &valueToRegister, 
                      const StackFrame &frame,MIRProgram &program) {
    const llvm::Value *retVal = retInst->getReturnValue();
    // 4. 创建函数结束块，插入 Epilogue 和 Ret                 

    std::cout << "[MIR] 处理 Return 指令: " << (retVal ? retVal->getName().str() : "void") << "\n";
    if (!retVal) {
        insertEpilogue(frame, mirBlock.instructions);
        mirBlock.instructions.emplace_back(MOpcode::RET);
        return; // 如果没有返回值，直接返回
    }

    std::string src = getOperandStr(retVal, valueToRegister, frame, mirBlock.instructions,program);
    if (llvm::isa<llvm::GlobalVariable>(retVal)) {
        std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(tempReg); // 将临时寄存器加入栈
        if(retVal->getType()->getPointerElementType()->isIntegerTy(32)) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_INT, tempReg, "0", src);
        } else if(retVal->getType()->getPointerElementType()->isFloatTy()) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_FLOAT, tempReg, "0", src,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if(retVal->getType()->getPointerElementType()->isIntegerTy(8)) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_CHAR, tempReg, "0", src);
        } else {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_INT, tempReg, "0", src);
        }
        //valueToRegister[retVal] = tempReg; // 更新映射
        valueToRegister.erase(retVal); // 删除原有映射
        src = tempReg;
    }


    llvm::Type *retType = retVal->getType();
    std::string dstOffset, dstBaseReg;
    if (parseStackOffset(src, dstOffset, dstBaseReg)) {
        // 如果 src 是栈偏移地址格式，先加载到临时寄存器
        int vregCounter = valueToRegister.size();
        std::string tmpReg = "v" + std::to_string(vregCounter+ TEMPReg.size());
        TEMPReg.push(tmpReg); // 将临时寄存器加入栈
        
        if (retType->isIntegerTy(32)) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_INT, tmpReg, dstOffset, dstBaseReg);
        } else if (retType->isFloatTy()) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_FLOAT, tmpReg, dstOffset, dstBaseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true);
        } else if (retType->isIntegerTy(8)) {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_CHAR, tmpReg, dstOffset, dstBaseReg);
        } else {
            mirBlock.instructions.emplace_back(MOpcode::LOAD_INT, tmpReg, dstOffset, dstBaseReg);
        }
        valueToRegister[retVal] = tmpReg; // 更新寄存器映射
        src = tmpReg; // 更新 src 为临时寄存器
    }

    if (retType->isIntegerTy(32)) {
        mirBlock.instructions.emplace_back(MOpcode::MOV_INT, "a0", src);
    } else if (retType->isFloatTy()) {
        mirBlock.instructions.emplace_back(MOpcode::MOV_FLOAT, "fa0", src,"",
            std::vector<ExtraDatum>(), // 没有初始值
            true); // 标记为浮点数
    } else if (retType->isIntegerTy(8)) {
        // char 类型
        mirBlock.instructions.emplace_back(MOpcode::MOV_CHAR, "a0", src);
    } else {
        // 默认当整型处理
        mirBlock.instructions.emplace_back(MOpcode::MOV_INT, "a0", src);
    }
    insertEpilogue(frame, mirBlock.instructions);
    mirBlock.instructions.emplace_back(MOpcode::RET);

}


void handleStoreInst(
    const llvm::StoreInst *store,
    std::vector<MInstruction> &mir,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,MIRProgram &program) 
{
    const llvm::Value *val = store->getValueOperand();
    const llvm::Value *ptr = store->getPointerOperand();

    std::string valStr = getOperandStr(val, valueToRegister, frame, mir,program);

    std::string offset_val = "0";
    std::string baseReg_val = "0";
    const llvm::Type *valType = val->getType();
    if(parseStackOffset(valStr, offset_val, baseReg_val)) {
        if(valType->isIntegerTy(32)){
            // 如果是32位整数
            valStr = "v" + std::to_string(valueToRegister.size()+ TEMPReg.size());
            mir.emplace_back(MOpcode::LOAD_INT, valStr, offset_val, baseReg_val);
        } else if(valType->isFloatTy()) {
            // 如果是浮点数
            valStr = "v" + std::to_string(valueToRegister.size()+ TEMPReg.size());
            mir.emplace_back(MOpcode::LOAD_FLOAT, valStr, offset_val, baseReg_val,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if(valType->isIntegerTy(8)) {
            // 如果是8位整数（char）
            valStr = "v" + std::to_string(valueToRegister.size()+ TEMPReg.size());
            mir.emplace_back(MOpcode::LOAD_CHAR, valStr, offset_val, baseReg_val);
        } else {
            // 默认当整型处理
            valStr = "v" + std::to_string(valueToRegister.size()+ TEMPReg.size());
            mir.emplace_back(MOpcode::LOAD_INT, valStr, offset_val, baseReg_val);
        }
        TEMPReg.push(valStr); // 将临时寄存器加入栈
        //valueToRegister[val] = valStr; // 更新寄存器映射

    } else {
        // valStr 就是寄存器名
        offset_val = "0"; // 偏移为0
        baseReg_val = valStr; // 寄存器名作为基址
    }

    std::string addrStr = getOperandStr(ptr, valueToRegister, frame, mir,program); // 可能是寄存器或栈偏移

    std::string offset = "0";
    std::string baseReg = addrStr;

    // 解析栈偏移格式
    if (parseStackOffset(addrStr, offset, baseReg)) {
        // 解析成功，offset 和 baseReg 已拆分
    } else {
        // addrStr 就是寄存器名，offset 保持为0
    }
    const llvm::Type *ty = val->getType();

    if (ty->isIntegerTy(32)) {
        mir.emplace_back(MOpcode::STORE_INT, valStr, offset, baseReg);
    } else if (ty->isFloatTy()) {
        mir.emplace_back(MOpcode::STORE_FLOAT, valStr, offset, baseReg,
            std::vector<ExtraDatum>(), // 没有初始值
            true); // 标记为浮点数
    } else if (ty->isIntegerTy(8)) {
        mir.emplace_back(MOpcode::STORE_CHAR, valStr, offset, baseReg);
    } else {
        mir.emplace_back(MOpcode::STORE_INT, valStr, offset, baseReg);
    }
    // 更新寄存器映射
    //valueToRegister[store] = addrStr; // 假设 store 的地址是
}


void handleGetElementPtrInst(const llvm::GetElementPtrInst *gep,
                             std::vector<MInstruction> &mir,
                             std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                             const StackFrame &frame,MIRProgram &program) {
    using namespace llvm;

    const Value *ptrOperand = gep->getPointerOperand();
    std::string baseReg = getOperandStr(ptrOperand, valueToRegister, frame, mir,program);
    std::cout << "[MIR] 处理 GEP，基址: " << baseReg << std::endl;

    const DataLayout &DL = gep->getModule()->getDataLayout();
    Type *curType = gep->getSourceElementType();

    std::string totalOffsetReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
    TEMPReg.push(totalOffsetReg); // 将临时寄存器加入栈
    mir.emplace_back(MOpcode::LI, totalOffsetReg, "0");

    // 遍历所有索引
    
    auto idxIter = gep->idx_begin();
    std::cout << "[MIR] 处理 GEP，初始类型: " << typeToString(curType) << std::endl;
    std::cout << "[MIR] idxIter 初始位置: " << idxIter << std::endl;
    ++idxIter; 

    // if (curType->isAggregateType()) {
    //     if (curType->isArrayTy()) {
    //         curType = curType->getArrayElementType();
    //     }else if (curType->isPointerTy()) {
    //         curType = curType->getPointerElementType();
    //     }
    // }

    for (; idxIter != gep->idx_end(); ++idxIter) {
        const Value *indexVal = idxIter->get();
        std::string indexReg = getOperandStr(indexVal, valueToRegister, frame, mir,program);

        if (curType->isArrayTy()) {
            auto *arrayTy = dyn_cast<ArrayType>(curType);
            Type *elemTy = arrayTy->getElementType();
            uint64_t elemSize = DL.getTypeAllocSize(elemTy);
            std::cout << "[MIR] 处理 GEP 数组元素，元素类型: " << typeToString(elemTy) << ", 大小: " << elemSize << std::endl;

            // size 寄存器
            std::string sizeReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
            TEMPReg.push(sizeReg); // 将临时寄存器加入栈
            mir.emplace_back(MOpcode::LI, sizeReg, std::to_string(elemSize));

            // offset = index * elemSize
            std::string subOffsetReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
            TEMPReg.push(subOffsetReg); // 将临时寄存器加入栈
            mir.emplace_back(MOpcode::MUL_INT, subOffsetReg, indexReg, sizeReg);

            // totalOffset += subOffset
            std::string newTotalOffsetReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
            TEMPReg.push(newTotalOffsetReg); // 将临时寄存器加入栈
            mir.emplace_back(MOpcode::ADD_INT, newTotalOffsetReg, totalOffsetReg, subOffsetReg);
            totalOffsetReg = newTotalOffsetReg;

            curType = elemTy; // 进入下一层类型
        }else{
            //处理非数组，是int类型或者float或者char
            break;
        } 
    }

    // 最后：result = base + totalOffset
    std::string offsetStr, baseStr;
    std::string resolvedBaseReg = baseReg;
    if (parseStackOffset(baseReg, offsetStr, baseStr)) {
        // 是 4(sp) 形式，先 ADD_INT 计算基址
        std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(tempReg); // 将临时寄存器加入栈
        mir.emplace_back(MOpcode::ADD_INT, tempReg, baseStr, offsetStr);
        resolvedBaseReg = tempReg;
    }

    std::string resultReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
    TEMPReg.push(resultReg); // 将临时寄存器加入栈
    mir.emplace_back(MOpcode::ADD_INT, resultReg, resolvedBaseReg, totalOffsetReg);
    
    valueToRegister[gep] = resultReg;
}



void handleGetElementPtrInst_forLoad(const llvm::GetElementPtrInst *gep,
                                     std::vector<MInstruction> &mir,
                                     std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                                     const StackFrame &frame,
                                     const llvm::DataLayout &DL, MIRProgram &program) {
    using namespace llvm;
    std::cout << "[MIR] 处理 GEP，基址: "  << std::endl;
    const Value *ptrOperand = gep->getPointerOperand();

    std::string baseReg = getOperandStr(ptrOperand, valueToRegister, frame, mir,program);
    std::cout << "[MIR] 处理 GEP，基址: " << baseReg << std::endl;

    Type *curType = gep->getSourceElementType();

    std::string totalOffsetReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
    TEMPReg.push(totalOffsetReg);
    mir.emplace_back(MOpcode::LI, totalOffsetReg, "0");

    unsigned regId = valueToRegister.size() + TEMPReg.size();

    for (auto idxIter = gep->idx_begin(); idxIter != gep->idx_end(); ++idxIter) {
        const Value *indexVal = idxIter->get();
        std::string indexReg = getOperandStr(indexVal, valueToRegister, frame, mir,program);

        if (curType->isArrayTy()) {
            auto *arrayTy = dyn_cast<ArrayType>(curType);
            Type *elemTy = arrayTy->getElementType();
            uint64_t elemSize = DL.getTypeAllocSize(elemTy);

            std::string sizeReg = "v" + std::to_string(regId++);
            TEMPReg.push(sizeReg);
            mir.emplace_back(MOpcode::LI, sizeReg, std::to_string(elemSize));

            std::string subOffsetReg = "v" + std::to_string(regId++);
            TEMPReg.push(subOffsetReg);
            mir.emplace_back(MOpcode::MUL_INT, subOffsetReg, indexReg, sizeReg);

            std::string newTotalOffsetReg = "v" + std::to_string(regId++);
            TEMPReg.push(newTotalOffsetReg);
            mir.emplace_back(MOpcode::ADD_INT, newTotalOffsetReg, totalOffsetReg, subOffsetReg);
            totalOffsetReg = newTotalOffsetReg;

            curType = elemTy;
        } else if (curType->isStructTy()) {
            auto *structTy = dyn_cast<StructType>(curType);
            if (auto *CI = dyn_cast<ConstantInt>(indexVal)) {
                uint64_t fieldIdx = CI->getZExtValue();
                uint64_t fieldOffset = DL.getStructLayout(structTy)->getElementOffset(fieldIdx);

                std::string fieldOffsetReg = "v" + std::to_string(regId++);
                TEMPReg.push(fieldOffsetReg);
                mir.emplace_back(MOpcode::LI, fieldOffsetReg, std::to_string(fieldOffset));

                std::string newTotalOffsetReg = "v" + std::to_string(regId++);
                TEMPReg.push(newTotalOffsetReg);
                mir.emplace_back(MOpcode::ADD_INT, newTotalOffsetReg, totalOffsetReg, fieldOffsetReg);
                totalOffsetReg = newTotalOffsetReg;

                curType = structTy->getElementType(fieldIdx);
            } else {
                std::cerr << "[Error] 非常数索引结构体字段未处理\n";
                break;
            }
        } else {
            // 其他类型不处理，跳出
            break;
        }
    }

    // 解析 baseReg 是否带偏移格式，比如 "4(sp)"
    std::string offsetStr, baseStr;
    std::string resolvedBaseReg = baseReg;
    if (parseStackOffset(baseReg, offsetStr, baseStr)) {
        std::string tempReg = "v" + std::to_string(regId++);
        TEMPReg.push(tempReg);
        mir.emplace_back(MOpcode::ADD_INT, tempReg, baseStr, offsetStr);
        resolvedBaseReg = tempReg;
    }

    // 最终 result = resolvedBaseReg + totalOffsetReg
    std::string resultReg = "v" + std::to_string(regId++);
    TEMPReg.push(resultReg);
    mir.emplace_back(MOpcode::ADD_INT, resultReg, resolvedBaseReg, totalOffsetReg);

    // 记录结果寄存器
    valueToRegister[gep] = resultReg;
}


/*
void handleLoadInst(const llvm::LoadInst *load,
                    std::vector<MInstruction> &mir,
                    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                    const StackFrame &frame) {
    using namespace llvm;

    const Value *ptr = load->getPointerOperand();
    std::string valStr = getOperandStr(ptr, valueToRegister, frame, mir);

    std::string offset_val = "0";
    std::string baseReg_val = "0";
    const llvm::Type *valType = load->getType();

    std::cout << "[MIR] 处理 Load 指令，加载地址: " << valStr << "\n";
    std::cout << "[MIR] 处理 Load 指令，加载类型: " ;
    valType->print(llvm::outs());
    llvm::outs() << "\n";


    // 判断是否是 "4(sp)" 这种形式的栈地址
    if (parseStackOffset(valStr, offset_val, baseReg_val)) {
        // 分配一个新虚拟寄存器用于存储加载结果
        std::string vreg = "v" + std::to_string(valueToRegister.size());

        if (valType->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, vreg, offset_val, baseReg_val);
        } else if (valType->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, vreg, offset_val, baseReg_val);
        } else if (valType->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, vreg, offset_val, baseReg_val);
        } else {
            // 默认当整型处理
            mir.emplace_back(MOpcode::LOAD_INT, vreg, offset_val, baseReg_val);
        }

        // 更新 valueToRegister 映射
        valueToRegister[load] = vreg;
    } else {
        // 如果 valStr 不是内存地址形式，直接 MOV
        std::string vreg = "v" + std::to_string(valueToRegister.size());
        mir.emplace_back(MOpcode::MOV_INT, vreg, valStr);
        valueToRegister[load] = vreg;
    }
}

*/
#include "llvm/Support/raw_ostream.h"
void handleLoadInst(const llvm::LoadInst *load,
                    std::vector<MInstruction> &mir,
                    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                    const StackFrame &frame,MIRProgram &program) {
    using namespace llvm;

    const Value *ptr = load->getPointerOperand();
    std::string addrReg;

    // 假设 load 是 const llvm::LoadInst* 类型
    load->print(llvm::outs());
    llvm::outs() << "\n";

    std::cout << "[DEBUG] ptr = ";
    ptr->print(llvm::outs());
    llvm::outs() << "\n";

    bool isGlobal = false;
    // 如果 ptr 是 GEP，先调用 handleGetElementPtrInst，计算地址寄存器
    if (const llvm::ConstantExpr *ce = llvm::dyn_cast<llvm::ConstantExpr>(ptr)) {
        if (ce->getOpcode() == llvm::Instruction::GetElementPtr) {
            llvm::Type *gepElemType = nullptr;
    
            llvm::Value *ptrOperand = ce->getOperand(0);
            llvm::Type *ptrOperandType = ptrOperand->getType();
    
            if (auto pt = llvm::dyn_cast<llvm::PointerType>(ptrOperandType)) {
                gepElemType = pt->getElementType();  // 这里才是正确的指针元素类型
            } else {
                std::cerr << "[Error] ptrOperand type is not PointerType.\n";
                // 返回或处理错误
            }
    
            llvm::SmallVector<llvm::Value *, 8> idxs;
            for (auto it = ce->op_begin() + 1; it != ce->op_end(); ++it) {
                idxs.push_back(it->get());
            }
    
            llvm::GetElementPtrInst *gepInst = llvm::GetElementPtrInst::Create(
                gepElemType,
                ptrOperand,
                idxs,
                "tmp_gep",
                (llvm::Instruction *)nullptr
            );
    
            if (gepInst) {
                std::cout << "[DEBUG] 处理 GEP 指令，计算地址寄存器\n";
                valueToRegister[gepInst] = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
                TEMPReg.push(valueToRegister[gepInst]);
                std:: cout<<"[DEBUG]gepinst内容"<<gepInst->getName().str()<<std::endl;
                gepInst->print(llvm::outs());
                llvm::outs() << "\n";

                const llvm::DataLayout &DL = load->getModule()->getDataLayout();
                
                handleGetElementPtrInst_forLoad(gepInst, mir, valueToRegister, frame, DL,program);


                std::cout << "[DEBUG] GEP 计算完成，地址寄存器: " << valueToRegister[gepInst] << "\n";
                addrReg = valueToRegister[gepInst];
            } else {
                std::cerr << "[Error] Failed to create temporary GEP instruction.\n";
            }
        }
    }
    else {
        // 否则直接从 valueToRegister 中获取寄存器或操作数字符串
        addrReg = getOperandStr(ptr, valueToRegister, frame, mir,program);
        //如果是全局变量，可能需要加载到寄存器
        if (const GlobalVariable *gv = llvm::dyn_cast<GlobalVariable>(ptr)) {
            isGlobal = true;
            std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
            TEMPReg.push(tempReg); // 将临时寄存器加入栈
            if (gv->getType()->getPointerElementType()->isIntegerTy(32)) {
                mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", addrReg);
            } else if (gv->getType()->getPointerElementType()->isFloatTy()) {
                mir.emplace_back(MOpcode::LOAD_FLOAT, tempReg, "0", addrReg,
                    std::vector<ExtraDatum>(), // 没有初始值
                    true); // 标记为浮点数
            } else if (gv->getType()->getPointerElementType()->isIntegerTy(8)) {
                mir.emplace_back(MOpcode::LOAD_CHAR, tempReg, "0", addrReg);
            } else {
                mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", addrReg);
            }
            //valueToRegister[ptr] = tempReg; // 更新寄存器映射
            valueToRegister.erase(ptr);
            addrReg = tempReg; // 更新 addrReg 为临时寄存器
        }
    }

    const Type *valType = load->getType();

    // 分配用于加载值的寄存器
    std::string vreg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
    TEMPReg.push(vreg);

    std::string offset_val, baseReg_val;

    // 判断 addrReg 是否是类似 "4(sp)" 这种偏移形式
    if (parseStackOffset(addrReg, offset_val, baseReg_val)) {
        // 是栈偏移形式，直接用偏移+基址加载
        if (valType->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, vreg, offset_val, baseReg_val);
        } else if (valType->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, vreg, offset_val, baseReg_val,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (valType->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, vreg, offset_val, baseReg_val);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, vreg, offset_val, baseReg_val);
        }
    } else if (!isGlobal) {
        // 不是偏移形式，addrReg 本身是寄存器名，偏移0加载
        if (valType->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, vreg, "0", addrReg);
        } else if (valType->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, vreg, "0", addrReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (valType->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, vreg, "0", addrReg);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, vreg, "0", addrReg);
        }
    } else {
        valueToRegister[load] = addrReg;
        return; // 如果是全局变量，直接返回
    }

    valueToRegister[load] = vreg;

}



void handleCallInst(const llvm::CallInst *call,
                    std::vector<MInstruction> &mir,
                    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                    const StackFrame &frame,MIRProgram &program) {

    using namespace llvm;

    // 1. 获取调用的函数指针
    const Function *calledFunc = call->getCalledFunction();
    if (!calledFunc) {
        std::cerr << "[Error] handleCallInst: 不是直接调用函数指针的CallInst，暂不支持！\n";
        return;
    }

    std::string funcName = calledFunc->getName().str();

    // 2. 处理参数，转换成寄存器字符串
    std::vector<ExtraDatum> argRegs;
    for (unsigned i = 0; i < call->getNumArgOperands(); ++i) {
        bool isFloat = false;
        const Value *argVal = call->getArgOperand(i);
        std::string argReg = getOperandStr(argVal, valueToRegister, frame, mir,program);
        // 如果是全局变量，可能需要加载到寄存器
        if (const GlobalVariable *gv = llvm::dyn_cast<GlobalVariable>(argVal)) {
            std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
            TEMPReg.push(tempReg); // 将临时寄存器加入栈
            if (gv->getType()->getPointerElementType()->isIntegerTy(32)) {
                mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", argReg);
            } else if (gv->getType()->getPointerElementType()->isFloatTy()) {
                mir.emplace_back(MOpcode::LOAD_FLOAT, tempReg, "0", argReg,
                    std::vector<ExtraDatum>(), // 没有初始值
                    true); // 标记为浮点数
                isFloat = true; // 标记为浮点数
            } else if (gv->getType()->getPointerElementType()->isIntegerTy(8)) {
                mir.emplace_back(MOpcode::LOAD_CHAR, tempReg, "0", argReg);
            } else {
                mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", argReg);
            }
            argReg = tempReg; // 更新为临时寄存器
            valueToRegister.erase(argVal);
            //alueToRegister[argVal] = tempReg; // 更新寄存器映射
        }
        //如果是栈偏移地址格式，处理成寄存器
        std::string offset, baseReg;
        if (parseStackOffset(argReg, offset, baseReg)) {
            int vregCounter = valueToRegister.size();
            std::string tmpReg = "v" + std::to_string(vregCounter + TEMPReg.size());
            TEMPReg.push(tmpReg); // 将临时寄存器加入栈
            if (argVal->getType()->getPointerElementType()->isIntegerTy(32)) {
                mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
            } else if (argVal->getType()->getPointerElementType()->isFloatTy()) {
                mir.emplace_back(MOpcode::LOAD_FLOAT, tmpReg, offset, baseReg,
                    std::vector<ExtraDatum>(), // 没有初始值
                    true); // 标记为浮点数
                isFloat = true; // 标记为浮点数
            } else if (argVal->getType()->getPointerElementType()->isIntegerTy(8)) {
                mir.emplace_back(MOpcode::LOAD_CHAR, tmpReg, offset, baseReg);      
            } else {
                mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
            }
            argReg = tmpReg; // 更新为临时寄存器
            valueToRegister[argVal] = tmpReg; // 更新寄存器映射
        }
        argRegs.push_back(ExtraDatum(argReg, isFloat)); // 包装成 ExtraDatum
    }

    // 3. 判断是否有返回值
    Type *retType = calledFunc->getReturnType();

    if (retType->isVoidTy()) {
        // 无返回值调用
        mir.emplace_back(MOpcode::CALL, "",funcName,"", argRegs);
    } else {
        // 有返回值调用，分配寄存器保存结果
        int vregCounter = valueToRegister.size() + TEMPReg.size();
        std::string retReg = "v" + std::to_string(vregCounter);
        TEMPReg.push(retReg);

        // argRegs 是所有参数的寄存器名列表
        // 把参数列表转换为单个字符串（例如逗号分隔），或者直接传给extra
        mir.emplace_back(MOpcode::CALL, retReg, funcName, "", argRegs);


        // 保存返回值寄存器映射
        valueToRegister[call] = retReg;
    }
}




void handleICmpInst(const llvm::ICmpInst *icmp,
                    std::vector<MInstruction> &mir,
                    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
                    const StackFrame &frame,MIRProgram &program) {
    using namespace llvm;

    const Value *lhs = icmp->getOperand(0);
    const Value *rhs = icmp->getOperand(1);

    std::string lhsReg = getOperandStr(lhs, valueToRegister, frame, mir,program);

    // 如果是全局变量，可能需要加载到寄存器
    if (const GlobalVariable *gv = llvm::dyn_cast<GlobalVariable>(lhs)) {
        std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(tempReg); // 将临时寄存器加入栈
        if (gv->getType()->getPointerElementType()->isIntegerTy (32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", lhsReg);
        } else if (gv->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tempReg, "0", lhsReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (gv->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, tempReg, "0", lhsReg);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", lhsReg);
        }
        valueToRegister.erase(lhs);
        lhsReg = tempReg; // 更新为临时寄存器
        //valueToRegister[lhs] = tempReg; // 更新寄存器映射
    }
    // 如果是栈偏移地址格式，处理成寄存器
    std::string offset, baseReg;
    if (parseStackOffset(lhsReg, offset, baseReg)) {
        int vregCounter = valueToRegister.size();
        std::string tmpReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(tmpReg); // 将临时寄存器加入栈
        if (lhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
        } else if (lhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tmpReg, offset, baseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (lhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, tmpReg, offset, baseReg);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
        }
        lhsReg = tmpReg; // 更新为临时寄存器
        //valueToRegister[lhs] = tmpReg; // 更新寄存器映射
    }


    std::string rhsReg = getOperandStr(rhs, valueToRegister, frame, mir,program);
    // 如果是全局变量，可能需要加载到寄存器
    if (const GlobalVariable *gv = llvm::dyn_cast<GlobalVariable>(rhs)) {
        std::string tempReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
        TEMPReg.push(tempReg); // 将临时寄存器加入栈
        if (gv->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", rhsReg);
        } else if (gv->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tempReg, "0", rhsReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (gv->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, tempReg, "0", rhsReg);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, tempReg, "0", rhsReg);
        }
        valueToRegister.erase(rhs);
        rhsReg = tempReg; // 更新为临时寄存器
        //valueToRegister[rhs] = tempReg; // 更新寄存器映射   
    }
    // 如果是栈偏移地址格式，处理成寄存器
    if (parseStackOffset(rhsReg, offset, baseReg)) {
        int vregCounter = valueToRegister.size();
        std::string tmpReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(tmpReg); // 将临时寄存器加入栈
        if (rhs->getType()->getPointerElementType()->isIntegerTy(32)) {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
        } else if (rhs->getType()->getPointerElementType()->isFloatTy()) {
            mir.emplace_back(MOpcode::LOAD_FLOAT, tmpReg, offset, baseReg,
                std::vector<ExtraDatum>(), // 没有初始值
                true); // 标记为浮点数
        } else if (rhs->getType()->getPointerElementType()->isIntegerTy(8)) {
            mir.emplace_back(MOpcode::LOAD_CHAR, tmpReg, offset, baseReg);
        } else {
            mir.emplace_back(MOpcode::LOAD_INT, tmpReg, offset, baseReg);
        }
        rhsReg = tmpReg; // 更新为临时寄存器
        //valueToRegister[rhs] = tmpReg; // 更新寄存器映射
    }



    std::string dstReg = "v" + std::to_string(valueToRegister.size() + TEMPReg.size());
    TEMPReg.push(dstReg);

    MOpcode op;

    // 判断是否是浮点数比较
    bool isFloat = lhs->getType()->isFloatingPointTy();

    auto pred = icmp->getPredicate();
    if (isFloat) {
        switch (pred) {
            case CmpInst::FCMP_OEQ: op = MOpcode::ICMP_EQ_FLOAT; break;
            case CmpInst::FCMP_ONE: op = MOpcode::ICMP_NE_FLOAT; break;
            case CmpInst::FCMP_OLT: op = MOpcode::ICMP_LT_FLOAT; break;
            case CmpInst::FCMP_OLE: op = MOpcode::ICMP_LE_FLOAT; break;
            case CmpInst::FCMP_OGT: op = MOpcode::ICMP_GT_FLOAT; break;
            case CmpInst::FCMP_OGE: op = MOpcode::ICMP_GE_FLOAT; break;
            default:
                std::cerr << "[Error] 未支持的浮点比较类型：" << pred << "\n";
                return;
        }
    } else {
        switch (pred) {
            case CmpInst::ICMP_EQ: op = MOpcode::ICMP_EQ_INT; break;
            case CmpInst::ICMP_NE: op = MOpcode::ICMP_NE_INT; break;
            case CmpInst::ICMP_SLT: op = MOpcode::ICMP_LT_INT; break;
            case CmpInst::ICMP_SLE: op = MOpcode::ICMP_LE_INT; break;
            case CmpInst::ICMP_SGT: op = MOpcode::ICMP_GT_INT; break;
            case CmpInst::ICMP_SGE: op = MOpcode::ICMP_GE_INT; break;
            default:
                std::cerr << "[Error] 未支持的整数比较类型：" << pred << "\n";
                return;
        }
    }

    mir.emplace_back(op, dstReg, lhsReg, rhsReg,
                     std::vector<ExtraDatum>(), // 没有初始值
                     isFloat); // 是否是浮点数比较
    valueToRegister[icmp] = dstReg;
}


void handleBranchInst(const llvm::BranchInst *br,
                      const llvm::Function &func,
                      std::vector<MInstruction> &mir,
                      const StackFrame &frame,
                      std::unordered_map<const llvm::Value*, std::string> &valueToRegister,MIRProgram &program) {
    using namespace llvm;

    if (br->isUnconditional()) {
        // 无条件跳转
        const BasicBlock *target = br->getSuccessor(0);
        std::string label = br->getFunction()->getName().str() + "_" + br->getSuccessor(0)->getName().str();
        mir.emplace_back(MOpcode::JMP, label);
    } else if (br->isConditional()) {
        const Value *cond = br->getCondition();
        std::string condReg = getOperandStr(cond, valueToRegister, frame, mir,program);

        const BasicBlock *trueBB = br->getSuccessor(0);
        const BasicBlock *falseBB = br->getSuccessor(1);

        std::string trueLabel = br->getFunction()->getName().str() + "_" + trueBB->getName().str();
        std::string falseLabel = br->getFunction()->getName().str() + "_" + falseBB->getName().str();
        

        // condReg ≠ 0 跳转 trueLabel，否则跳 falseLabel
        // 可以用 BNE condReg, z, trueLabel
        mir.emplace_back(MOpcode::BNE, trueLabel, condReg, "x0");
        mir.emplace_back(MOpcode::JMP, falseLabel);
        
    } else {
        std::cerr << "[Error] handleBranchInst: 未知分支类型！\n";
    }
}



void dumpMIR(const MIRProgram &program, const std::string &filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }

    // 先打印全局指令（全局变量和extern函数声明）
    for (const auto &inst : program.globalInstructions) {
        std::string opcode = opcodeToString(inst.op);
        out << opcode;

        std::vector<std::string> operands;
        if (!inst.dst.empty()) operands.push_back(inst.dst);
        if (!inst.src1.empty()) operands.push_back(inst.src1);
        if (!inst.src2.empty()) operands.push_back(inst.src2);

        if (!operands.empty()) {
            out << " " << operands[0];
            for (size_t i = 1; i < operands.size(); ++i) {
                out << ", " << operands[i];
            }
        }
        out << "\n";
    }
    out << "\n"; // 全局部分和函数间空行

    // 再打印所有函数
    for (const auto &func : program.functions) {
        out << "LABEL " << func.name << "\n";
        for (const auto &block : func.blocks) {
            if (!block.label.empty()) {
                out << "LABEL " << block.label << "\n";
            }
            for (const auto &inst : block.instructions) {
                if (inst.op == MOpcode::LABEL) {
                    continue; // 避免重复打印LABEL
                }

                std::string opcode = opcodeToString(inst.op);
                out << opcode;

                std::vector<std::string> operands;
                if (!inst.dst.empty()) operands.push_back(inst.dst);
                if (!inst.src1.empty()) operands.push_back(inst.src1);
                if (!inst.src2.empty()) operands.push_back(inst.src2);

                if (!operands.empty()) {
                    out << " " << operands[0];
                    for (size_t i = 1; i < operands.size(); ++i) {
                        out << ", " << operands[i];
                    }
                }

                out << "\n";
            }
        }
        out << "\n";
    }

    out.close();
}




std::string getOperandStr(
    const llvm::Value *val,
    std::unordered_map<const llvm::Value*, std::string> &valueToRegister,
    const StackFrame &frame,
    std::vector<MInstruction> &mir,
    MIRProgram &program
) {

    // 1. 如果是常量整数
    if (const auto *constInt = llvm::dyn_cast<llvm::ConstantInt>(val)) {
        int vregCounter = valueToRegister.size();
        std::string tmpReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(tmpReg); // 将临时寄存器加入栈
        mir.emplace_back(MOpcode::LI, tmpReg, std::to_string(constInt->getSExtValue())); // 生成寄存器赋值指令
        valueToRegister[val] = tmpReg; // 记录寄存器映射
        return tmpReg;
    }

    // 2. 如果是常量浮点数
    if (const auto *constFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
        float valDouble = constFP->getValueAPF().convertToFloat();
    
        // 1. 从浮点常量表里找对应符号
        std::string symbolName;
        // 先查找是否已经存在
        for (const auto &f : program.staticFloats) {
            if (f.value == valDouble) {
                symbolName = f.symbol;
                break;
            }
        }
        if (symbolName.empty()) {
            // 没找到才添加
            symbolName = "float_const" + std::to_string(program.staticFloats.size());
            program.staticFloats.push_back({symbolName, valDouble});
        }

    
        int vregCounter = valueToRegister.size();
        std::string tmpReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(tmpReg); // 将临时寄存器加入栈

        mir.emplace_back(MOpcode::LA, tmpReg, symbolName); // 生成加载浮点常量的指令
    
        std::string floatReg = "v" + std::to_string(vregCounter + TEMPReg.size() + 1);
        TEMPReg.push(floatReg); // 将临时寄存器加入栈

        mir.emplace_back(MOpcode::LOAD_FLOAT, floatReg, "0", tmpReg,
            std::vector<ExtraDatum>(), // 没有初始值
            true // 标记为浮点数
        ); // 生成加载浮点数的指令
    
        valueToRegister[val] = floatReg;
    
        return floatReg;
    }
    
    if (const auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(val)) {
        // 判断是否已处理过
        int vregCounter = valueToRegister.size();
        std::string addrReg = "v" + std::to_string(vregCounter + TEMPReg.size());
        TEMPReg.push(addrReg); // 将临时寄存器加入栈

        llvm::Type *ty = globalVar->getValueType();
        bool isFloat = ty->isFloatTy() || ty->isDoubleTy(); 

        // 先加载地址
        mir.emplace_back(MOpcode::LA, addrReg, globalVar->getName().str());
        valueToRegister[globalVar] = addrReg;
    
        return valueToRegister.at(globalVar);
    }
    
    // 3. 寄存器映射
    auto it = valueToRegister.find(val);
    if (it != valueToRegister.end()) {
        return it->second;
    }



    
    
    // 4. 栈偏移
    if (val->hasName()) {
        Variable var{val->getName().str()};
        auto offsetIt = frame.localVarOffsets.find(var);
        if (offsetIt != frame.localVarOffsets.end()) {
            // 只返回栈偏移地址字符串，比如 "12(sp)"
            return std::to_string(offsetIt->second) + "(" + SP.name + ")";
        }
    }



    // 5. fallback
    return "tmp" + std::to_string(reinterpret_cast<uintptr_t>(val));
}
