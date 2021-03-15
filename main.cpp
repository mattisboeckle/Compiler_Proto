#include <iostream>
#include "codegen.h"
#include "node.h"

using namespace std;

extern int yyparse();
extern NBlock* programBlock;

extern FILE *yyin;

extern ExitOnError ExitOnErr;

int main(int argc, char **argv)
{

    if(argc == 2) {
        if ((yyin = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
            exit(1);
        }
    }

    CodeGenContext context;

    yyparse();

    ExitOnErr.setBanner(std::string(argv[0]) + ": ");

    context.generateCode(*programBlock);
    context.runCode();

    return 0;
}