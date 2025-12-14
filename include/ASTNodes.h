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
        static std::map<std::string, llvm::Value *> NamedValues;
        // static std::unique_ptr<llvm::FunctionPassManager> TheFPM; // interpreter only
        // static std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
        // static std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
        // static std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
        // static std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
        // static std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
        // static std::unique_ptr<llvm::StandardInstrumentations> TheSI;
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
};

class BinaryASTNode : public ASTNode {
    private:
        char op;
        std::unique_ptr<ASTNode> LHS, RHS;
    
    public:
        BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS);
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

    public:
        PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments);
        const std::string& getName() const;
        llvm::Function* codegen() override;
};

class FunctionASTNode : public ASTNode {
    private:
        std::unique_ptr<PrototypeASTNode> proto;
        std::unique_ptr<ASTNode> body;

    public:
        FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::unique_ptr<ASTNode> Body);
        llvm::Function* codegen() override;
};

#endif