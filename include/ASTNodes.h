#ifndef __ASTNODES_CPP__
#define __ASTNODES_CPP__

#include "Token.hpp"
#include <string>
#include <vector>
#include <memory>

class ASTNode { //is named ExprAST in LLVM tutorial
    public:
        virtual ~ASTNode() = default;
};

class NumberASTNode : public ASTNode {
    private:
        double val;

    public:
        NumberASTNode(double value);
};

class VariableASTNode : public ASTNode {
    private:
        std::string varName;

    public:
        VariableASTNode(const std::string variableName);
};

class BinaryASTNode : public ASTNode {
    private:
        char op;
        std::unique_ptr<ASTNode> LHS, RHS;
    
    public:
        BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS);
};

class CallASTNode : public ASTNode {
    private:
        std::string callee;
        std::vector<std::unique_ptr<ASTNode>> args;

    public:
        CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments);
};

class PrototypeASTNode : public ASTNode {
    private:
        std::string name;
        std::vector<std::string> args;

    public:
        PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments);
        const std::string& getName() const;
};

class FunctionASTNode : public ASTNode {
    private:
        std::unique_ptr<PrototypeASTNode> proto;
        std::unique_ptr<ASTNode> body;

    public:
        FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::unique_ptr<ASTNode> Body);
};

#endif