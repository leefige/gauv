//
//  main.cpp
//  Ref: https://github.com/antlr/antlr4/tree/master/runtime/Cpp/demo
//

#include "frontend/generated/MPCLexer.h"
#include "frontend/generated/MPCParser.h"

#include <antlr4-runtime.h>

#include <iostream>

using namespace mpc;
using namespace antlr4;

int main(int argc, const char **argv) {
    ANTLRInputStream input(std::string_view(";"));
    MPCLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }

    MPCParser parser(&tokens);
    tree::ParseTree* tree = parser.translationUnit();

    std::cout << tree->toStringTree(&parser) << std::endl << std::endl;

    return 0;
}
