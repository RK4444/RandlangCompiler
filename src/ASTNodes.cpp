#include "../include/ASTNodes.h"

NumberASTNode::NumberASTNode(double value) : val(value) {}

VariableASTNode::VariableASTNode(const std::string variableName) : varName(variableName){}

BinaryASTNode::BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS) : op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)){}

CallASTNode::CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments) : callee(Callee), args(std::move(arguments)) {}

PrototypeASTNode:: PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments) : name(Name), args(std::move(arguments)) {}

FunctionASTNode:: FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::unique_ptr<ASTNode> Body) : proto(std::move(prototype)), body(std::move(Body)) {}