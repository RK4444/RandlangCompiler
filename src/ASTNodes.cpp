#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "../include/ASTNodes.h"
#include <memory>

NumberASTNode::NumberASTNode(double value) : val(value) {}

VariableASTNode::VariableASTNode(const std::string variableName) : varName(variableName){}

BinaryASTNode::BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS) : op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)){}

CallASTNode::CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments) : callee(Callee), args(std::move(arguments)) {}

PrototypeASTNode:: PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments) : name(Name), args(std::move(arguments)) {}

FunctionASTNode:: FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::unique_ptr<ASTNode> Body) : proto(std::move(prototype)), body(std::move(Body)) {}

llvm::Value* NumberASTNode::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

llvm::Value* VariableASTNode::codegen() {
    
}

llvm::Value* BinaryASTNode::codegen() {
    
}

llvm::Value* CallASTNode::codegen() {
    
}

llvm::Value* PrototypeASTNode::codegen() {
    
}

llvm::Value* FunctionASTNode::codegen() {
    
}