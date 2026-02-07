#ifndef __ASTNODES_CPP__
#define __ASTNODES_CPP__

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
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "Token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

class ASTNode { //is named ExprAST in LLVM tutorial
    public:
        virtual ~ASTNode() = default;
        virtual llvm::Value* codegen() = 0;
        static std::unique_ptr<llvm::LLVMContext> TheContext;
        static std::unique_ptr<llvm::IRBuilder<>> Builder;
        static std::unique_ptr<llvm::Module> TheModule;
        static std::map<std::string, llvm::AllocaInst*> NamedValues;
        static std::unique_ptr<llvm::FunctionPassManager> TheFPM;
        static std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
        static std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
        static std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
        static std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
        static std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
        static std::unique_ptr<llvm::StandardInstrumentations> TheSI;
        llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* TheFunction, llvm::StringRef VarName);
        llvm::Value* vLogError(const char *str);
};

class NumberASTNode : public ASTNode {
    private:
        double val;

    public:
        NumberASTNode(double value);
        llvm::Value* codegen() override;
};

class VariableASTNode : public ASTNode {
    private:
        std::string varName;

    public:
        VariableASTNode(const std::string variableName);
        llvm::Value* codegen() override;
        const std::string& getName() const;
};

class BinaryASTNode : public ASTNode {
    private:
        char op;
        std::unique_ptr<ASTNode> LHS, RHS;
        bool isSinglecharOperator;
        Token::Kind tokenkind;
    
    public:
        BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS, bool isSinglecharOperator, Token::Kind tokenkind);
        llvm::Value* codegen() override;
};

class CallASTNode : public ASTNode {
    private:
        std::string callee;
        std::vector<std::unique_ptr<ASTNode>> args;

    public:
        CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments);
        llvm::Value* codegen() override;
};

class PrototypeASTNode : public ASTNode {
    private:
        std::string name;
        std::vector<std::string> args;
        bool isOperator;
        unsigned Precedence;

    public:
        PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments, bool isOperator=false, unsigned Prec=0);
        const std::string& getName() const;
        llvm::Function* codegen() override;
        bool isUnaryOp() const;
        bool isBinaryOP() const;
        char getOperatorName() const;
        unsigned getBinaryPrecedence() const;
};

class FunctionASTNode : public ASTNode {
    private:
        std::unique_ptr<PrototypeASTNode> proto;
        std::vector<std::unique_ptr<ASTNode>> body;
        //std::map<char, int> BinopPrecedence;
        public:
        static std::map<std::string, std::unique_ptr<PrototypeASTNode>> FunctionProtos;
        FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::vector<std::unique_ptr<ASTNode>> Body);
        llvm::Function* codegen() override;
        static llvm::Function* getFunction(std::string Name);
};

class IfExprAST : public ASTNode
{
private:
    std::unique_ptr<ASTNode> Cond;
    std::vector<std::unique_ptr<ASTNode>> Then, Else;
public:
    IfExprAST(std::unique_ptr<ASTNode> Cond, std::vector<std::unique_ptr<ASTNode>> Then, std::vector<std::unique_ptr<ASTNode>> Else);
    //~IfExprAST();
    llvm::Value* codegen() override;
};

class ForExprAST : public ASTNode
{
private:
    std::string VarName;
    std::unique_ptr<ASTNode> Start, End, Step;
    std::vector<std::unique_ptr<ASTNode>> Body;
public:
    ForExprAST(const std::string &VarName, std::unique_ptr<ASTNode> Start,
        std::unique_ptr<ASTNode> End, std::unique_ptr<ASTNode> Step, std::vector<std::unique_ptr<ASTNode>> Body);

    llvm::Value* codegen() override;
};

class UnaryExprAst : public ASTNode
{
private:
    char Opcode;
    std::unique_ptr<ASTNode> Operand;
public:
    UnaryExprAst(char Opcode, std::unique_ptr<ASTNode> Operand);

    llvm::Value* codegen() override;
};

class VarAstNode : public ASTNode
{
private:
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> VarNames;
    std::unique_ptr<ASTNode> Body;
public:
    VarAstNode(std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> VarNames, std::unique_ptr<ASTNode> Body);
    llvm::Value* codegen() override;
};

#endif