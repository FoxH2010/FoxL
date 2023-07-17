@echo off

rem Biên dịch FoxL.g
antlr4 -o src -Dlanguage=Cpp FoxL.g

rem Biên dịch các tệp C++
g++ -o FoxL FoxL.cpp FoxLLexer.cpp FoxLParser.cpp -lfl

rem Chạy FoxL
FoxL