#ifndef __PARSER_CPP__
#define __PARSER_CPP__

#include "Token.hpp"
#include "Lexer.h"
#include <iostream>
#include "ASTNodes.h"
#include <map>

class Parser
{
private:
    Token curTok;
    Token getNextToken();
    Lexer lex;
    std::map<char, int> BinopPrecedence;

    std::unique_ptr<ASTNode> logError(const char* str, int linenumber);
    std::unique_ptr<PrototypeASTNode> pLogError(const char* str, int linenumber);
    std::unique_ptr<ASTNode> parseNumberExpr();
    std::unique_ptr<ASTNode> parseParenExpr();
    std::unique_ptr<ASTNode> parseIdentifierExpr();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseBinOpRHS(int exprPrec, std::unique_ptr<ASTNode> LHS);
    std::unique_ptr<PrototypeASTNode> parsePrototype();
    std::unique_ptr<FunctionASTNode> parseDefinition();
    std::unique_ptr<PrototypeASTNode> parseExtern();
    std::unique_ptr<FunctionASTNode> parseTopLevelExpr();
    std::unique_ptr<ASTNode> ParseIfExpr();
    llvm::ExitOnError ExitOnErr;
    void InitializeModulesAndManagers();
    void HandleDefinition();
    void HandleExtern();
    void HandleTopLevelExpression();

    int getTokenPrecedence();
public:
    Parser(const char* beg);
    void parse();
    ~Parser();
};


#endif