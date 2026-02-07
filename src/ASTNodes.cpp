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
std::map<std::string, llvm::AllocaInst *> ASTNode::NamedValues;
std::map<std::string, std::unique_ptr<PrototypeASTNode>> FunctionASTNode::FunctionProtos;

std::unique_ptr<llvm::FunctionPassManager> ASTNode::TheFPM = nullptr;
std::unique_ptr<llvm::LoopAnalysisManager> ASTNode::TheLAM = nullptr;
std::unique_ptr<llvm::FunctionAnalysisManager> ASTNode::TheFAM = nullptr;
std::unique_ptr<llvm::CGSCCAnalysisManager> ASTNode::TheCGAM = nullptr;
std::unique_ptr<llvm::ModuleAnalysisManager> ASTNode::TheMAM = nullptr;
std::unique_ptr<llvm::PassInstrumentationCallbacks> ASTNode::ThePIC = nullptr;
std::unique_ptr<llvm::StandardInstrumentations> ASTNode::TheSI = nullptr;

llvm::AllocaInst* ASTNode::CreateEntryBlockAlloca(llvm::Function* TheFunction, llvm::StringRef VarName) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getDoubleTy(*TheContext), nullptr, VarName);
}

NumberASTNode::NumberASTNode(double value) : val(value) {}

VariableASTNode::VariableASTNode(const std::string variableName) : varName(variableName){}
const std::string& VariableASTNode::getName() const {
    return varName;
}

BinaryASTNode::BinaryASTNode(char Op, std::unique_ptr<ASTNode> LHS, std::unique_ptr<ASTNode> RHS, bool isSinglecharOperator, Token::Kind tokenkind) : op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)), isSinglecharOperator(isSinglecharOperator), tokenkind(tokenkind){}

CallASTNode::CallASTNode(const std::string& Callee, std::vector<std::unique_ptr<ASTNode>> arguments) : callee(Callee), args(std::move(arguments)) {}

PrototypeASTNode:: PrototypeASTNode(const std::string& Name, std::vector<std::string> arguments, bool isOperator, unsigned Prec) : name(Name), args(std::move(arguments)), isOperator(isOperator), Precedence(Prec) {}

const std::string& PrototypeASTNode::getName() const {
    return name;
};

bool PrototypeASTNode::isUnaryOp() const {
    return isOperator && args.size() == 1;
}

bool PrototypeASTNode::isBinaryOP() const {
    return isOperator && args.size() == 2;
}

char PrototypeASTNode::getOperatorName() const {
    assert(isUnaryOp() || isBinaryOP());
    return name[name.size() - 1];
}

unsigned PrototypeASTNode::getBinaryPrecedence() const {
    return Precedence;
}

FunctionASTNode:: FunctionASTNode(std::unique_ptr<PrototypeASTNode> prototype, std::vector<std::unique_ptr<ASTNode>> Body) : proto(std::move(prototype)), body(std::move(Body)) {}


IfExprAST::IfExprAST(std::unique_ptr<ASTNode> Cond, std::vector<std::unique_ptr<ASTNode>> Then, std::vector<std::unique_ptr<ASTNode>> Else) : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

llvm::Value* NumberASTNode::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

ForExprAST::ForExprAST(const std::string &VarName, std::unique_ptr<ASTNode> Start,
    std::unique_ptr<ASTNode> End, std::unique_ptr<ASTNode> Step, std::vector<std::unique_ptr<ASTNode>> Body) : VarName(VarName), Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)), Body(std::move(Body))
{
}

UnaryExprAst::UnaryExprAst(char Opcode, std::unique_ptr<ASTNode> Operand) : Opcode(Opcode), Operand(std::move(Operand))
{
}

VarAstNode::VarAstNode(std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> VarNames, std::unique_ptr<ASTNode> Body) : VarNames(std::move(VarNames)), Body(std::move(Body))
{
}

llvm::Value* VariableASTNode::codegen() {
    std::string variableName = varName;
    llvm::AllocaInst* V = NamedValues[variableName];
    if (!V)
    {
        vLogError("Unknown variable name");
    }
    return Builder->CreateLoad(V->getAllocatedType(), V, varName.c_str());
}

llvm::Value* BinaryASTNode::codegen() {

    if (op == '=')
    {
        VariableASTNode* LHSE = static_cast<VariableASTNode*>(LHS.get());
        if (!LHSE)
        {
            return vLogError("destination of '=' must be a variable");
        }
        
        llvm::Value* Val = RHS->codegen();
        if (!Val)
        {
            return nullptr;
        }
        
        llvm::Value* Variable = NamedValues[LHSE->getName()];
        if (!Variable)
        {
            return vLogError("Unknown variable Name");
        }
        
        Builder->CreateStore(Val, Variable);
        return Val;
    }
    

    llvm::Value* L = LHS->codegen();
    llvm::Value* R = RHS->codegen();
    if (!L || !R)
    {
       return nullptr;
    }
    if (isSinglecharOperator)
    {
        
        switch (op)
        {
        case '+':
            return ASTNode::Builder->CreateFAdd(L, R, "addtmp");
    
        case '-':
            return ASTNode::Builder->CreateFSub(L, R, "subtmp");
    
        case '*':
            return ASTNode::Builder->CreateFMul(L, R, "multmp");
    
        case '<':
            L = ASTNode::Builder->CreateFCmpULT(L, R, "cmptmpl");
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmpl"); // Convert bool 0/1 to double 0.0 or 1.0
        case '>':
            L = ASTNode::Builder->CreateFCmpUGT(L, R, "cmptmpr");
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmpr"); // Convert bool 0/1 to double 0.0 or 1.0 
        
        default:
            break;;
        }
    } else {
        switch (tokenkind)
        {
        case Token::Kind::DoubleEqual:
            L = ASTNode::Builder->CreateFCmpUEQ(L, R);
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmpe"); // Convert bool 0/1 to double 0.0 or 1.0
        case Token::Kind::GreaterOrEqual:
            L = ASTNode::Builder->CreateFCmpUGE(L, R, "cmptmpre");
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmpre"); // Convert bool 0/1 to double 0.0 or 1.0
        case Token::Kind::LessOrEqual:
            L = ASTNode::Builder->CreateFCmpULE(L, R, "cmptmple");
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmple"); // Convert bool 0/1 to double 0.0 or 1.0
        case Token::Kind::NotEqual:
            L = ASTNode::Builder->CreateFCmpUNE(L, R);
            return ASTNode::Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmpne"); // Convert bool 0/1 to double 0.0 or 1.0
        default:
            break;
        }
    }
    

    llvm::Function* F = FunctionASTNode::getFunction(std::string("binary") + op);
    assert(F && "binary operator not found!");

    llvm::Value* Ops[2] = {L, R};
    return Builder->CreateCall(F, Ops, "binop");
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
    // llvm::Function* TheFunction = TheModule->getFunction(proto->getName());

    // if(!TheFunction) {
    //     TheFunction = proto->codegen();
    // }

    // if(!TheFunction) {
    //     return nullptr;
    // }

    // if(!TheFunction->empty()) {
    //     return (llvm::Function*) vLogError("Function cannot be redefined");
    // }
    auto &P = *proto;
    FunctionProtos[proto->getName()] = std::move(proto);
    llvm::Function* TheFunction = getFunction(P.getName());
    if (!TheFunction)
    {
        return nullptr;
    }
    
    llvm::BasicBlock* BB = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for(auto& Arg : TheFunction->args()) {
        // NamedValues[std::string(Arg.getName())] = &Arg;// Before Kaleidoscope Chapter 7 only
        llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());
        Builder->CreateStore(&Arg, Alloca);
        NamedValues[std::string(Arg.getName())] = Alloca;
    }

    llvm::Value* lastValue;
    for (auto &expr : body)
    {
        lastValue = expr->codegen();
        if (!lastValue)
        {
            TheFunction->eraseFromParent();
            return nullptr;
        }
        
    }
    
    Builder->CreateRet(lastValue);
    llvm::verifyFunction(*TheFunction);
    TheFPM->run(*TheFunction, *TheFAM);
    return TheFunction;

    // if(llvm::Value *RetVal = body->codegen()) {
    //     Builder->CreateRet(RetVal);
    //     llvm::verifyFunction(*TheFunction);

    //     TheFPM->run(*TheFunction, *TheFAM);

    //     return TheFunction;
    // }

    
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

    llvm::Value* lastValueThen;
    for (auto &expr : Then)
    {
        lastValueThen = expr->codegen();
        if (!lastValueThen)
        {
            return nullptr;
        }
        
    }
    
    Builder->CreateBr(MergeBB);

    ThenBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    llvm::Value* lastValueElse;
    for (auto &expr : Else)
    {
        lastValueElse = expr->codegen();
        if (!lastValueElse)
        {
            return nullptr;
        } 
    }
    
    Builder->CreateBr(MergeBB);
    ElseBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);
    llvm::PHINode* PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");

    PN->addIncoming(lastValueThen, ThenBB);
    PN->addIncoming(lastValueElse, ElseBB);
    return PN;
}

llvm::Value* ForExprAST::codegen() {
    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();
    llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

    llvm::Value* StartVal = Start->codegen();
    if (!StartVal)
    {
        return nullptr;
    }

    Builder->CreateStore(StartVal, Alloca);
    
    // llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent(); // Before Kaleidoscope Chapter 7 only
    // llvm::BasicBlock* PreheaderBB = Builder->GetInsertBlock();
    llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);

    Builder->CreateBr(LoopBB);

    Builder->SetInsertPoint(LoopBB);

    // llvm::PHINode* Variable = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, VarName); //Before Kaleidoscope Chapter 7 only
    // Variable->addIncoming(StartVal, PreheaderBB);

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    llvm::AllocaInst* oldVal = NamedValues[VarName];
    NamedValues[VarName] = Alloca;

    llvm::Value* lastValue;
    for (auto &expr : Body)
    {
        lastValue = expr->codegen();
        if (!lastValue)
        {
            return nullptr;
        }
        
    }

    // if (!Body->codegen())
    // {
    //     return nullptr;
    // }

    llvm::Value* StepVal = nullptr;
    if (Step)
    {
        StepVal = Step->codegen();
        if (!StepVal)
        {
            return nullptr;
        }
        
    } else
    {
        StepVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(1.0));
    }
    
    //llvm::Value* NextVar = Builder->CreateFAdd(Variable, StepVal, "nextvar");// Before Kaleidoscope Chapter 7 only

    llvm::Value* EndCond = End->codegen();
    if (!EndCond)
    {
        return nullptr;
    }

    llvm::Value* CurVar = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
    llvm::Value* NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    Builder->CreateStore(NextVar, Alloca);
    
    EndCond = Builder->CreateFCmpONE(EndCond, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "loopcond");

    // llvm::BasicBlock* LoopEndBB = Builder->GetInsertBlock(); // Before Kaleidoscope Chapter 7 only
    llvm::BasicBlock* AfterBB = llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);

    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    Builder->SetInsertPoint(AfterBB);
    // Variable->addIncoming(NextVar, LoopEndBB); // Before Kaleidoscope Chapter 7 only

    //Restore the unshadowed variable
    if (oldVal)
    {
        NamedValues[VarName] = oldVal;
    } else
    {
        NamedValues.erase(VarName);
    }

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext));
}

llvm::Value* UnaryExprAst::codegen() {
    llvm::Value* OperandV = Operand->codegen();
    if (!OperandV)
    {
        return nullptr;
    }
    
    llvm::Function* F = FunctionASTNode::getFunction(std::string("unary") + Opcode);
    if (!F)
    {
        return vLogError("Unknown unary operator");
    }
    
    return Builder->CreateCall(F, OperandV, "unop");
}


llvm::Value* ASTNode::vLogError(const char *str){
    std::cout << "Code generation error: " << str << std::endl;
    return nullptr;
}

llvm::Function* FunctionASTNode::getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}

llvm::Value* VarAstNode::codegen() {
    std::vector<llvm::AllocaInst*> OldBindings;

    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();
    for (unsigned i = 0, e = VarNames.size(); i != e; i++)
    {
        const std::string& VarName = VarNames[i].first;
        ASTNode* Init = VarNames[i].second.get();

        llvm::Value* InitVal;
        if (Init)
        {
            InitVal = Init->codegen();
            if (!InitVal)
            {
                return nullptr;
            }
            
        } else {
            InitVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0));
        }
        
        llvm::AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
        Builder->CreateStore(InitVal, Alloca);

        OldBindings.push_back(NamedValues[VarName]);

        NamedValues[VarName] = Alloca;
    }
    
    llvm::Value* BodyVal = Body->codegen();
    if (!BodyVal)
    {
        return nullptr;
    }
    
    for (unsigned i = 0, e = VarNames.size(); i != e; i++)
    {
        NamedValues[VarNames[i].first] = OldBindings[i];
    }
    return BodyVal;
}