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
#include "../include/Parser.h"
#include "../include/Token.hpp"
#include "../include/Lexer.h"
#include "../include/ASTNodes.h"
#include <iostream>
#include <string>
#include <vector>

Parser::Parser(const char* beg) : lex(beg), curTok(Token::Kind::Semicolon) {
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();

    

    BinopPrecedence['='] = 2;
    BinopPrecedence['<'] = 10;
    BinopPrecedence['>'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;



    //theJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create()); //TODO: Continue from https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl04.html

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

    // ASTNode::TheFPM = std::make_unique<llvm::FunctionPassManager>(); //Interpreter only
    // ASTNode::TheLAM = std::make_unique<llvm::LoopAnalysisManager>();
    // ASTNode::TheFAM = std::make_unique<llvm::FunctionAnalysisManager>();
    // ASTNode::TheCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
    // ASTNode::TheMAM = std::make_unique<llvm::ModuleAnalysisManager>();
    // ASTNode::ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    // ASTNode::TheSI = std::make_unique<llvm::StandardInstrumentations>(*ASTNode::TheContext, /*DebugLogging*/ true);

    // ASTNode::TheFPM->addPass(llvm::InstCombinePass());

}

Token Parser::getNextToken() {
    return curTok = lex.next();
    //std::cout << curTok.lexeme() << std::endl;
}

std::unique_ptr<ASTNode> Parser::logError(const char* str){
    std::cerr << str << std::endl;
    return nullptr;
}
std::unique_ptr<PrototypeASTNode> Parser::pLogError(const char* str) {
    logError(str);
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
        return logError("expected ')'");
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
    while (getNextToken().kind() != Token::Kind::LeftParen)
    {
        if (auto arg = parseExpression()) {
            args.push_back(std::move(arg));
        } else {
            return nullptr;
        }

        if (curTok.kind() != Token::Kind::Comma) {
            return logError("Expected ')' or ',' in argument list");
        }
        getNextToken();
    }
    //std::cout << "Parsed function call" << std::endl;
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
    
    default:
        return logError("unknown token when expecting a expression");
    }
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    auto LHS = parsePrimary();
    if (!LHS) {
        return nullptr;
    }
    //std::cout << "Parsed expression" << std::endl;
    return parseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ASTNode> Parser::parseBinOpRHS(int exprPrec, std::unique_ptr<ASTNode> LHS) {
    while (true)
    {
        int tokPrec = getTokenPrecedence();

        if (tokPrec < exprPrec)
        {
            return LHS;
        }
        
        Token binOP = curTok;
        getNextToken();

        auto RHS = parsePrimary();
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
    if (curTok.kind() != Token::Kind::Identifier) {
        return pLogError("Expected function name in prototype");
    }
    std::string fnName = std::string(curTok.lexeme());
    

    if (getNextToken().kind() != Token::Kind::LeftParen) {
        return pLogError("Expected '(' in prototype");
    }

    std::vector<std::string> argNames;
    
    while (getNextToken().kind() == Token::Kind::Identifier) //TODO: Check if this works every time
    {
        argNames.push_back(std::string(curTok.lexeme()));
        //getNextToken();
    }
    if (curTok.kind() != Token::Kind::RightParen)
    {
        return pLogError("Expected ')' in prototype");
    }
    
    getNextToken();
    //std::cout << "Parsed function prototype" << std::endl;
    return std::make_unique<PrototypeASTNode>(fnName, std::move(argNames));
    
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
        auto Proto = std::make_unique<PrototypeASTNode>("", std::vector<std::string>());
        return std::make_unique<FunctionASTNode>(std::move(Proto), std::move(E));
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
                if (curTok.lexeme() == "int" || curTok.lexeme() == "void" || curTok.lexeme() == "double" || curTok.lexeme() == "float")
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