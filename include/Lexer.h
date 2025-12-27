// A simple Lexer meant to demonstrate a few theoretical concepts. It can
// support several parser concepts and is very fast (though speed is not its
// design goal).
//
// J. Arrieta, Nabla Zero Labs
//
// This code is released under the MIT License.
//
// Copyright 2018 Nabla Zero Labs
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish ,distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef __LEXER_CPP__
#define __LEXER_CPP__


#include "Token.hpp"

class Lexer {
 public:
  Lexer(const char* beg) noexcept : m_beg{beg} { linenumber = 1; col = 1;}

  Token next() noexcept;

  int getCurrentLineNumber();
  int getCol();
  // void incrementCurrentLineNumber();
  
  private:
  Token identifier() noexcept;
  Token number() noexcept;
  Token slash_or_comment() noexcept;
  Token atom(Token::Kind) noexcept;
  bool is_space(char c) noexcept;
  bool is_digit(char c) noexcept;
  bool is_identifier_char(char c) noexcept;
  int linenumber;
  int col;
  char peek() const noexcept { return *m_beg; }
  char get() noexcept { return *m_beg++; }
  
  // static int currentLineNumber; //TODO: Fix this mess

  const char* m_beg = nullptr;
};

#endif