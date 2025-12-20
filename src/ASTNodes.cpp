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

std::unique_ptr<llvm::LLVMContext> ASTNode::TheContext = nullptr;
std::unique_ptr<llvm::IRBuilder<>> ASTNode::Builder = nullptr;
std::unique_ptr<llvm::Module> ASTNode::TheModule = nullptr;
std::map<std::string, llvm::Value *> ASTNode::NamedValues;

NumberASTNode::NumberASTNode(double value) : val(value) {}

VariableASTNode::VariableASTNode(const std::string variableName) : varName(variableName){}

BinaryASTNode::BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS) : op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)){}

CallASTNode::CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments) : callee(Callee), args(std::move(arguments)) {}

PrototypeASTNode:: PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments) : name(Name), args(std::move(arguments)) {}

const std::string& PrototypeASTNode::getName() const {
    return name;
};

FunctionASTNode:: FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::unique_ptr<ASTNode> Body) : proto(std::move(prototype)), body(std::move(Body)) {}


IfExprAST::IfExprAST(std::unique_ptr<ASTNode> Cond, std::unique_ptr<ASTNode> Then, std::unique_ptr<ASTNode> Else) : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

llvm::Value* NumberASTNode::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

llvm::Value* VariableASTNode::codegen() {
    std::string variableName = varName;
    llvm::Value* V = NamedValues[variableName];
    if (!V)
    {
        vLogError("Unknown variable name");
    }
    return V;
}

llvm::Value* BinaryASTNode::codegen() {
    llvm::Value* L = LHS->codegen();
    llvm::Value* R = RHS->codegen();
    if (!L || !R)
    {
       return nullptr;
    }
    
    switch (op)
    {
    case '+':
        return ASTNode::Builder->CreateFAdd(L, R, "addtmp");

    case '-':
        return ASTNode::Builder->CreateFSub(L, R, "subtmp");

    case '*':
        return ASTNode::Builder->CreateFMul(L, R, "multmp");

    case '<':
        L = ASTNode::Builder->CreateFCmpULT(L, R, "cmptmp");
        return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp");
    
    default:
        return vLogError("invalid binary operator");
    }
}

llvm::Value* CallASTNode::codegen() {
    llvm::Function* CalleeF = TheModule->getFunction(callee);
    if (!CalleeF)
    {
        return vLogError("unknown function referenced");
    }

    if (CalleeF->arg_size() != args.size())
    {
        return vLogError("Incorrect number of arguments");
    }
    
    std::vector<llvm::Value*> argsV;
    for (unsigned i = 0, e = args.size(); i != e; i++)
    {
        argsV.push_back(args[i]->codegen());
        if (!argsV.back())
        {
            return nullptr;
        }
        
    }
    
    return Builder->CreateCall(CalleeF, argsV, "calltmp");
}


llvm::Function* PrototypeASTNode::codegen() {
    std::vector<llvm::Type*> Doubles(args.size(), llvm::Type::getDoubleTy(*TheContext));

    llvm::FunctionType* FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, TheModule.get());

    unsigned Idx = 0;
    for (auto& Arg : F->args()) {
        Arg.setName(args[Idx++]);
    }

    return F;
}

llvm::Function* FunctionASTNode::codegen() {
    llvm::Function* TheFunction = TheModule->getFunction(proto->getName());

    if(!TheFunction) {
        TheFunction = proto->codegen();
    }

    if(!TheFunction) {
        return nullptr;
    }

    if(!TheFunction->empty()) {
        return (llvm::Function*) vLogError("Function cannot be redefined");
    }

    llvm::BasicBlock* BB = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for(auto& Arg : TheFunction->args()) {
        NamedValues[std::string(Arg.getName())] = &Arg;
    }

    if(llvm::Value *RetVal = body->codegen()) {
        Builder->CreateRet(RetVal);
        llvm::verifyFunction(*TheFunction);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

llvm::Value* IfExprAST::codegen() {
    llvm::Value* CondV = Cond->codegen();
    if (!CondV)
    {
        return nullptr;
    }

    CondV = Builder->CreateFCmpONE(CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");

    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

    Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    Builder->SetInsertPoint(ThenBB);

    llvm::Value* ThenV = Then->codegen();
    if (!ThenV)
    {
        return nullptr;
    }
    
    Builder->CreateBr(MergeBB);

    ThenBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    llvm::Value* ElseV = Else->codegen();
    if (!ElseV)
    {
        return nullptr;
    }
    
    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);
    llvm::PHINode* PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

llvm::Value* ASTNode::vLogError(const char *str){
    std::cout << "Code generation error: " << str << std::endl;
    return nullptr;
}