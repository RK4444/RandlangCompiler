#include "../include/Parser.h"
#include <string>
#include <fstream>
#include <iostream>

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
    if (argc != 2)
    {
      std::cerr << "Number of Arguments is not right";
      return -1;
    }

    std::string code;
    std::string snipped;
    std::ifstream filestr(argv[1]);

    if (filestr.is_open())
    {
      while (filestr.good())
      {
        filestr >> snipped;
        code += snipped;
        code += "\n";
      }
      
      
    } else {
      std::cerr << "could not open file";
      return -1;
    }
    Parser cparse(code.c_str());
    cparse.parse();
  //   Lexer lex(code.c_str());
  //   for (auto token = lex.next();
  //      not token.is_one_of(Token::Kind::End, Token::Kind::Unexpected);
  //      token = lex.next()) {
  //   std::cout << std::setw(12) << token.kind() << " |" << token.lexeme()
  //             << "|\n";
  // }
}