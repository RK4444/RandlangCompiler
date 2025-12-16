#include "../include/Parser.h"
#include <string>
#include <fstream>
#include <iostream>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

int main(int argc, char** argv) {
  // auto code =
  //     "x = 2\n"
  //     "// This is a comment.\n"
  //     "var x\n"
  //     "var y\n"
  //     "var f = function(x, y) { sin(x) * sin(y) + x * y; }\n"
  //     "der(f, x)\n"
  //     "var g = function(x, y) { 2 * (x + der(f, y)); } // der(f, y) is a "
  //     "matrix\n"
  //     "var r{3}; // Vector of three elements\n"
  //     "var J{12, 12}; // Matrix of 12x12 elements\n"
  //     "var dot = function(u{:}, v{:}) -> scalar {\n"
  //     "          return u[i] * v[i]; // Einstein notation\n"
  //     "}\n"
  //     "var norm = function(u{:}) -> scalar { return sqrt(dot(u, u)); }\n"
  //     "<end>";
    if (argc != 3) // max is argv[2]
    {
      std::cerr << "USAGE: randlang <inputFile> <outputFile>";
      return -1;
    }

    std::string code;
    std::string snipped;
    std::ifstream filestr(argv[1]);

    if (filestr.is_open())
    {
      while (filestr.good())
      {
        std::getline(filestr, snipped);
        code += snipped;
        code += "\n";
        //std::cout << snipped << std::endl;
      }
      
      
    } else {
      std::cerr << "could not open file";
      return -1;
    }

    //std::cout << code.c_str() << std::endl;

    Parser cparse(code.c_str());
    cparse.parse();
  //   Lexer lex(code.c_str());
  //   for (auto token = lex.next();
  //      not token.is_one_of(Token::Kind::End, Token::Kind::Unexpected);
  //      token = lex.next()) {
  //   std::cout << std::setw(12) << token.kind() << " |" << token.lexeme()
  //             << "|\n";
  // }

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    ASTNode::TheModule->setTargetTriple(llvm::Triple(TargetTriple));

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(ASTNode::TheModule->getTargetTriple(), Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
      llvm::errs() << Error;
      return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto TheTargetMachine = Target->createTargetMachine(
        llvm::Triple(TargetTriple), CPU, Features, opt, llvm::Reloc::PIC_);

    ASTNode::TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = argv[2];
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);
    // llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::CD_OpenAlways, llvm::sys::fs::FA_Write, llvm::sys::fs::OF_None); //A different option
    

    if (EC) {
      llvm::errs() << "Could not open file: " << EC.message();
      return 1;
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::ObjectFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
      llvm::errs() << "TheTargetMachine can't emit a file of this type";
      return 1;
    }

    pass.run(*ASTNode::TheModule);
    dest.flush();

    llvm::outs() << "Wrote " << Filename << "\n";

  return 0;
}