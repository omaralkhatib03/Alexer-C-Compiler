#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "cli.h"
#include "ast.h"


void printBT(const std::string& prefix, const CommonNodePtr node, bool isLeft, std::ostream &out)
{
    if( node != nullptr )
    {
        out << prefix;

        out << (isLeft ? "├──" : "└──" );

        // print the value of the node
        node->prettyPrint(out);

        // enter the next tree level - left and right branch
        for (int i = 0 ; i < (int) node->getChildren().size(); i++)
        {
            if (i == (int) node->getChildren().size() - 1)
            {
                printBT( prefix + (isLeft ? "│   " : "    "), node->getChildren()[i], false, out);
            }
            else
            {
                printBT( prefix + (isLeft ? "│   " : "    "), node->getChildren()[i], true, out);
            }
        }

    }
}


void compile(std::ostream &out)
{  
    Context *ctx = new Context(out); 
    CommonNodePtr ast = parseAST();
    Tracker *trk = getTracker();
    // trk->printAll(std::cout);
    // out << std::endl;
    // trk->printCallerCallee(out);
    ctx->setFunctionSizes(trk->createCtxTable());
    ctx->setFunctionCallTypes(trk->scopeCallerCallee);
    std::cout << "####################    AST    ####################" << std::endl;
    printBT("", ast, true, std::cout);
    ast->compile(ctx);
    ctx->regManager->printAllRegs();
    ctx->out << ctx->ss.str();

    delete ctx;
    delete trk;

}

// TODO: uncomment the below if you're using Flex/Bison.
extern FILE *yyin;

int main(int argc, char **argv)
{
    // Parse CLI arguments, to fetch the values of the source and output files.
    std::string sourcePath = "";
    std::string outputPath = "";
    if (parse_command_line_args(argc, argv, sourcePath, outputPath))
    {
        return 1;
    }

    // TODO: uncomment the below lines if you're using Flex/Bison.
    // This configures Flex to look at sourcePath instead of
    // reading from stdin.
    yyin = fopen(sourcePath.c_str(), "r");
    if (yyin == NULL)
    {
        perror("Could not open source file");
        return 1;
    }

    // Open the output file in truncation mode (to overwrite the contents)
    std::ofstream output;
    output.open(outputPath, std::ios::trunc);

    // Compile the input
    std::cout << "Compiling: " << sourcePath << std::endl;
    compile(output);
    std::cout << "Compiled to: " << outputPath << std::endl;

    output.close();
    return 0;
}
