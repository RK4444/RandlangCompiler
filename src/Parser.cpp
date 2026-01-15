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
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "../include/Parser.h"
#include "../include/Token.hpp"
#include "../include/Lexer.h"
#include "../include/ASTNodes.h"
#include <iostream>
#include <string>
#include <vector>

Parser::Parser(const char* beg) : curTok(Token::Kind::Semicolon), lex(beg) {
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();

    

    BinopPrecedence['='] = 2;
    BinopPrecedence['<'] = 10;
    BinopPrecedence['>'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;


    InitializeModulesAndManagers();
}

Parser::~Parser()
{
}

void Parser::InitializeModulesAndManagers() {
    ASTNode::TheContext = std::make_unique<llvm::LLVMContext>();
    ASTNode::TheModule = std::make_unique<llvm::Module>("my cool jit", *ASTNode::TheContext);
    //ASTNode::TheModule->setDataLayout(theJIT->getDataLayout());

    ASTNode::Builder = std::make_unique<llvm::IRBuilder<>>(*ASTNode::TheContext);

    ASTNode::TheFPM = std::make_unique<llvm::FunctionPassManager>(); //Interpreter only
    ASTNode::TheLAM = std::make_unique<llvm::LoopAnalysisManager>();
    ASTNode::TheFAM = std::make_unique<llvm::FunctionAnalysisManager>();
    ASTNode::TheCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
    ASTNode::TheMAM = std::make_unique<llvm::ModuleAnalysisManager>();
    ASTNode::ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    ASTNode::TheSI = std::make_unique<llvm::StandardInstrumentations>(*ASTNode::TheContext, /*DebugLogging*/ true);

    ASTNode::TheSI->registerCallbacks(*ASTNode::ThePIC, ASTNode::TheMAM.get());

    ASTNode::TheFPM->addPass(llvm::PromotePass());
    ASTNode::TheFPM->addPass(llvm::InstCombinePass());
    ASTNode::TheFPM->addPass(llvm::ReassociatePass());
    ASTNode::TheFPM->addPass(llvm::GVNPass());
    ASTNode::TheFPM->addPass(llvm::SimplifyCFGPass());

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(*ASTNode::TheMAM);
    PB.registerFunctionAnalyses(*ASTNode::TheFAM);
    PB.crossRegisterProxies(*ASTNode::TheLAM, *ASTNode::TheFAM, *ASTNode::TheCGAM, *ASTNode::TheMAM);

}

Token Parser::getNextToken() {
    return curTok = lex.next();
    //std::cout << curTok.lexeme() << std::endl;
}

std::unique_ptr<ASTNode> Parser::logError(const char* str, int linenumber=0){
    if (linenumber == 0)
    {
        std::cerr << str << std::endl;
    } else {
        std::cerr << str << " in Line: " << linenumber << std::endl;
    }
    
    return nullptr;
}
std::unique_ptr<PrototypeASTNode> Parser::pLogError(const char* str, int linenumber=0) {
    logError(str, linenumber);
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseNumberExpr() {
    double val = std::stod(std::string(curTok.lexeme()));
    auto Result = std::make_unique<NumberASTNode>(val);
    getNextToken();
    //std::cout << "Parsed number" << std::endl;
    return std::move(Result);
}

std::unique_ptr<ASTNode> Parser::parseParenExpr() {
    getNextToken();
    auto V = parseExpression();
    if (!V) {
        return nullptr;
    }

    if (curTok.kind() != Token::Kind::RightParen) {
        return logError("expected ')'", lex.getCurrentLineNumber());
    }
    getNextToken();
    //std::cout << "Parsed parentese expression" << std::endl;
    return V;
}

std::unique_ptr<ASTNode> Parser::parseIdentifierExpr() {
    std::string idName = std::string(curTok.lexeme());
    

    if (getNextToken().kind() != Token::Kind::LeftParen) {
        return std::make_unique<VariableASTNode>(idName);
    }

    
    std::vector<std::unique_ptr<ASTNode>> args;
    if (getNextToken().kind() != Token::Kind::RightParen)
    {
        while (true)
        {
            if (auto Arg = parseExpression())
            {
                args.push_back(std::move(Arg));
            } else {
                return nullptr;
            }
            
            if (curTok.kind() == Token::Kind::RightParen)
            {
                break;
            }
            
            if (curTok.kind() != Token::Kind::Comma)
            {
                return logError("Expected ')' or ',' in argument list", lex.getCurrentLineNumber());
            }
            getNextToken();
        }
        
    }
    
    // while (getNextToken().kind() != Token::Kind::LeftParen)
    // {
    //     if (auto arg = parseExpression()) {
    //         args.push_back(std::move(arg));
    //     } else {
    //         return nullptr;
    //     }

    //     if (curTok.kind() != Token::Kind::Comma) {
    //         return logError("Expected ')' or ',' in argument list");
    //     }
    //     getNextToken();
    // }
    //std::cout << "Parsed function call" << std::endl;
    getNextToken();
    return std::make_unique<CallASTNode>(idName, std::move(args));
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    switch (curTok.kind())
    {
    case Token::Kind::Identifier:
        return parseIdentifierExpr();
    case Token::Kind::Number:
        return parseNumberExpr();
    case Token::Kind::LeftParen:
        return parseParenExpr();
    case Token::Kind::Keyword:
        if (curTok.lexeme() == "if")
        {
            return ParseIfExpr();
        } else if (curTok.lexeme() == "for")
        {
            return ParseForExpr();
        }
        return logError("CAUTION: Everything other than the if and for statements are not implemented. Expecting an if or for statement therefore", lex.getCurrentLineNumber());
    default:
        return logError("unknown token when expecting a expression", lex.getCurrentLineNumber());
    }
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    auto LHS = parseUnary();
    if (!LHS) {
        return nullptr;
    }
    //std::cout << "Parsed expression" << std::endl;
    return parseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ASTNode> Parser::parseBinOpRHS(int exprPrec, std::unique_ptr<ASTNode> LHS) {
    // for (auto tprec : BinopPrecedence) {
    //     std::cout << tprec.first << " is: " << tprec.second << "\n";
    // }
    while (true)
    {
        int tokPrec = getTokenPrecedence();

        if (tokPrec < exprPrec)
        {
            return LHS;
        }
        
        Token binOP = curTok;
        getNextToken();

        auto RHS = parseUnary();
        if (!RHS) {
            return nullptr;
        }

        int nextPrec = getTokenPrecedence();
        if (tokPrec < nextPrec) {
            RHS = parseBinOpRHS(tokPrec+1, std::move(RHS));
            if (!RHS) {
                return nullptr;
            }
        }
        //std::cout << "Parsed RHS" << std::endl;
        LHS = std::make_unique<BinaryASTNode>((char)*binOP.lexeme().begin(), std::move(LHS), std::move(RHS)); //attention here. this doesn't work probable
    }
    
}

std::unique_ptr<PrototypeASTNode> Parser::parsePrototype() {
    std::string FnName;
    unsigned Kind = 0;
    unsigned BinaryPrecedence = 30;

    if (curTok.is(Token::Kind::Identifier))
    {
        FnName = std::string(curTok.lexeme());
        Kind = 0;
        getNextToken();
    } else if (curTok.is(Token::Kind::Keyword)) {
        switch (curTok.type())
        {
        case Token::KeywordType::Binary:
            getNextToken();
            if (!isascii((char)*curTok.lexeme().begin()))
            {
                return pLogError("Expected binary operator", lex.getCurrentLineNumber());
            }
            FnName = "binary";
            FnName += (char)*curTok.lexeme().begin();
            Kind = 2;
                        
            if (getNextToken().is(Token::Kind::Number))
            {
                int numVal = std::stoi(std::string(curTok.lexeme()));
                if (numVal < 1 || numVal > 100)
                {
                    return pLogError("Invalid precedence: must be 1...100", lex.getCurrentLineNumber());
                }
                BinaryPrecedence = numVal;
                getNextToken();
            }
            BinopPrecedence[FnName.back()] = BinaryPrecedence;
            break;
        case Token::KeywordType::Unary:
            getNextToken();
            if (!isascii((char)*curTok.lexeme().begin()))
            {
                return pLogError("Expected unary operator", lex.getCurrentLineNumber());
            }
            FnName = "unary";
            FnName += (char)*curTok.lexeme().begin();
            Kind = 1;
            getNextToken();
            break;
        default:
            return pLogError("Expected function name in prototype", lex.getCurrentLineNumber());
            break;
        }
    } else {
        return pLogError("Expected function name in prototype", lex.getCurrentLineNumber());
    }
    
    

    if (curTok.is_not(Token::Kind::LeftParen)) {
        return pLogError("Expected '(' in prototype", lex.getCurrentLineNumber());
    }

    std::vector<std::string> argNames;
    
    while (getNextToken().is(Token::Kind::Identifier))
    {
        argNames.push_back(std::string(curTok.lexeme()));
        //getNextToken();
    }
    if (curTok.is_not(Token::Kind::RightParen))
    {
        return pLogError("Expected ')' in prototype", lex.getCurrentLineNumber());
    }
    
    getNextToken();
    //std::cout << "Parsed function prototype" << std::endl;
    return std::make_unique<PrototypeASTNode>(FnName, std::move(argNames), Kind != 0, BinaryPrecedence);
    
}

std::unique_ptr<FunctionASTNode> Parser::parseDefinition() {
    getNextToken();
    auto Proto = parsePrototype();
    if (!Proto) {
        return nullptr;
    }

    if (auto E = parseExpression()) {
        //std::cout << "Parsed function" << std::endl;
        return std::make_unique<FunctionASTNode>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<PrototypeASTNode> Parser::parseExtern() {
    getNextToken();
    //std::cout << "Parsed extern" << std::endl;
    return parsePrototype();
}

std::unique_ptr<FunctionASTNode> Parser::parseTopLevelExpr() {
    if (auto E = parseExpression())
    {
        if (curTok.lexeme() == "main")
        {
            auto Proto = std::make_unique<PrototypeASTNode>("main", std::vector<std::string>());
            return std::make_unique<FunctionASTNode>(std::move(Proto), std::move(E));
        } else {
            auto Proto = std::make_unique<PrototypeASTNode>("", std::vector<std::string>());
            return std::make_unique<FunctionASTNode>(std::move(Proto), std::move(E));
        } 
    }
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::ParseIfExpr() {
    getNextToken();

    auto Cond = parseExpression();
    if (!Cond)
    {
        return nullptr;
    }

    if (curTok.lexeme() != "then")
    {
        return logError("Expected then", lex.getCurrentLineNumber());
    }
    getNextToken();

    auto Then = parseExpression();
    if (!Then)
    {
        return nullptr;
    }
    
    if (curTok.lexeme() != "else")
    {
        return logError("expected else", lex.getCurrentLineNumber());
    }
    
    getNextToken();

    auto Else = parseExpression();
    if (!Else)
    {
        return nullptr;
    }
    
    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

std::unique_ptr<ASTNode> Parser::ParseForExpr() {
    if (getNextToken().kind() != Token::Kind::Identifier)
    {
        return logError("Expected identifier after for", lex.getCurrentLineNumber());
    }

    std::string idName = std::string(curTok.lexeme());
    
    if (getNextToken().kind() != Token::Kind::Equal)
    {
        logError("Expected '=' after for", lex.getCurrentLineNumber());
    }

    getNextToken();
    auto Start = parseExpression();
    if (!Start)
    {
        return nullptr;
    }

    if (curTok.kind() != Token::Kind::Comma)
    {
        logError("expected ',' after for start value", lex.getCurrentLineNumber());
    }

    getNextToken();
    auto End = parseExpression();
    
    if (!End)
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> Step;
    if (curTok.kind() == Token::Kind::Comma)
    {
        getNextToken();
        Step = parseExpression();
        if (!Step)
        {
            return nullptr;
        }
    }
    
    if (curTok.lexeme() != "in")
    {
        return logError("Expected 'in' after for", lex.getCurrentLineNumber());
    }

    getNextToken();
    auto Body = parseExpression();
    if (!Body)
    {
        return nullptr;
    }
    
    return std::make_unique<ForExprAST>(idName, std::move(Start), std::move(End), std::move(Step), std::move(Body));
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    if (curTok.is_one_of(Token::Kind::Comma, Token::Kind::LeftParen, Token::Kind::Identifier, Token::Kind::Number, Token::Kind::Keyword))
    {
        return parsePrimary();
    }
    
    int Opc = (char)*curTok.lexeme().begin();
    getNextToken();
    if (auto Operand = parseUnary())
    {
        return std::make_unique<UnaryExprAst>(Opc, std::move(Operand));
    }
    return nullptr;
}

int Parser::getTokenPrecedence() { 
    if (!isascii((char)*curTok.lexeme().begin())) {
        return -1;
    }

    int tokPrec = BinopPrecedence[(char)*curTok.lexeme().begin()];
    
    if (tokPrec <= 0) {
        return -1;
    }
    return tokPrec;
}

void Parser::parse() {
    while (true)
    {
        switch (curTok.kind())
        {
            case Token::Kind::End:
                return;
            case Token::Kind::Semicolon:
                getNextToken();
                break;
            case Token::Kind::Keyword:
                if (curTok.lexeme() == "fn")
                {
                    HandleDefinition();
                    break;
                } else if (curTok.lexeme() == "extern")
                {
                    HandleExtern();
                    break;
                }
                getNextToken();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
    
}

void Parser::HandleDefinition() {
  if (auto FnAST = parseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
        std::cout << "Parsed a function definition." << std::endl;
        FnIR->print(llvm::errs());
        std::cout << "\n";
    }
        
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void Parser::HandleExtern() {
  if (auto FnAST = parseExtern()) {
    if (auto *FnIR = FnAST->codegen()) {
        std::cout << "Parsed an extern." << std::endl;
        FnIR->print(llvm::errs());
        std::cout << "\n";
        FunctionASTNode::FunctionProtos[FnAST->getName()] = std::move(FnAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void Parser::HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = parseTopLevelExpr()) {
    if (auto* FnIR = FnAST->codegen()) {

        std::cout << "Parsed a top-level expr" << std::endl;
        FnIR->print(llvm::errs());
        std::cout << "\n";
    }
    
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}