#include "node.h"
#include "codegen.h"
#include "parser.hpp"
#include <string>


using namespace std;
using namespace llvm;

ExitOnError ExitOnErr;
bool DEBUG = true;
extern int Line;

//Adds functions into the llvm code
//Functions are extern'd in native.cpp
static void createCoreFns() {

    // Should probably add extern statement
    // instead of manually doing this

    //Print double
    std::vector<llvm::Type*> printd_arg_types;
    printd_arg_types.push_back(Builder->getDoubleTy());
    llvm::FunctionType* printd_type = llvm::FunctionType::get(Builder->getInt32Ty(), printd_arg_types, true);
    Function *printdfn = llvm::Function::Create(printd_type, llvm::Function::ExternalLinkage,"printd",*TheModule);
    verifyFunction(*printdfn, &llvm::errs());

    //Print int
    std::vector<llvm::Type*> printi_arg_types;
    printi_arg_types.push_back(Builder->getInt64Ty());
    llvm::FunctionType* printi_type = llvm::FunctionType::get(Builder->getInt32Ty(), printi_arg_types, true);
    Function *printifn = llvm::Function::Create(printi_type, llvm::Function::ExternalLinkage,"printi",*TheModule);
    verifyFunction(*printifn, &llvm::errs());

    //printf
    std::vector<llvm::Type*> print_arg_types;
    print_arg_types.push_back(Builder->getInt8PtrTy());
    llvm::FunctionType* print_type = llvm::FunctionType::get(Builder->getInt32Ty(), print_arg_types, true);
    Function *printfn = llvm::Function::Create(print_type, llvm::Function::ExternalLinkage,"print",*TheModule);
    verifyFunction(*printfn, &llvm::errs());
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
    if(DEBUG) std::cout << "Generating code...\n";

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    if(DEBUG) printf("Creating context and Module\n");
    TheJIT = ExitOnErr(ProtoJIT::Create());
    myContext = &TheJIT->getContext();
    TheModule = std::make_unique<Module>("main_module", *myContext);
    TheModule->setDataLayout(TheJIT->getDataLayout());
    Builder = std::make_unique<IRBuilder<>>(*myContext);

    createCoreFns();

    /* Create the top level interpreter function to call as entry */
    vector<Type*> argTypes;
    FunctionType *ftype = FunctionType::get(Builder->getVoidTy(), makeArrayRef(argTypes), false);
    mainFunction = Function::Create(ftype, Function::ExternalLinkage, "main", TheModule.get());
    BasicBlock *bblock = BasicBlock::Create(*myContext, "entry", mainFunction, 0);

    /* Push a new variable/block context */
    pushBlock(bblock);
    Builder->SetInsertPoint(bblock);
    root.codeGen(*this); // emit bytecode for the toplevel block
    Builder->CreateRetVoid();
    popBlock();


    /* Print the bytecode in a human-readable format 
       to see if out program compiled properly */
    if(DEBUG) {
        TheModule->print(errs(), nullptr);
        std::cout << "Code is generated.\n";
    }
    
}

/* Executes the AST by running the main function */
void CodeGenContext::runCode() {

    ExitOnErr(TheJIT->addModule(std::move(TheModule)));
    if(DEBUG) printf("Added module\n");

    auto main_sym = ExitOnErr(TheJIT->lookup("main"));
    void (*main_fn)() = (void (*)())main_sym.getAddress(); //Get function pointer to main

    main_fn(); //Run the main function

    if(DEBUG) std::cout << "Code was run.\n";
}

/* Returns an LLVM type based on the identifier */
static Type *typeOf(const NIdentifier& type) 
{
    if (type.name.compare("int") == 0) {
        return Builder->getInt64Ty();
    }
    else if (type.name.compare("double") == 0) {
        return Builder->getDoubleTy();
    }
    else if (type.name.compare("boolean") == 0) {
        return Builder->getInt1Ty();
    }
    return Builder->getVoidTy();
}

/* -- Code Generation -- */

// Done
Value* NInteger::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Creating integer: " << value << std::endl;
    //type = Builder->getInt32Ty();
    return ConstantInt::get(Builder->getInt64Ty(), value);
}

// Done
Value* NDouble::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Creating double: " << value << std::endl;
    //type = Builder->getDoubleTy();
    return ConstantFP::get(*myContext, APFloat(value));
}

Value* NBoolean::codeGen(CodeGenContext& context) 
{
    printf("Doing something stupid");
    return ConstantInt::get(Builder->getInt1Ty(), (int)value);
}

/*
ConstantDataArray* NString::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Creating string: " << value << std::endl;
    
    ConstantDataArray *arr = cast<ConstantDataArray>(ConstantDataArray::getString(*myContext, StringRef(value), false));
    
    return arr;
}
*/

// Done
Value* NIdentifier::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Creating identifier reference: " << name << endl;
    if (context.locals().find(name) == context.locals().end()) {
        fprintf(stderr, "Undeclared variable %s\n", name.c_str());
        return NULL;
    }
    return Builder->CreateLoad(context.locals()[name], name);
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
    Function *function = TheModule->getFunction(id.name.c_str());
    if (function == NULL) {
        fprintf(stderr, "No such function %s\n", id.name.c_str());
    }

    std::vector<Value*> args;
    ExpressionList::const_iterator it;
    for(it = arguments.begin(); it != arguments.end(); it++) {
        args.push_back((**it).codeGen(context));
    }
    CallInst *call = Builder->CreateCall(function, makeArrayRef(args), id.name.c_str());
    if(DEBUG) printf("Creating method call: %s\n", id.name.c_str());
    return call;
}

// Done
Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Creating binary operation " << op << std::endl;
    Value *L = lhs.codeGen(context);
    Value *R = rhs.codeGen(context);

    bool L_doub = L->getType()->isDoubleTy();
    bool R_doub = R->getType()->isDoubleTy();

    //If one of the operands is a double cast the other one if it isnt
    if(L_doub && !R_doub) {
        R = Builder->CreateUIToFP(R, Builder->getDoubleTy());
    }else if(R_doub && !L_doub) {
        L = Builder->CreateUIToFP(L, Builder->getDoubleTy());
    }

    bool doub = L_doub || R_doub;

    switch (op) {
        case PLUS:
            if(doub) return Builder->CreateFAdd(L, R, "addtmp");
            return Builder->CreateAdd(L, R, "addtmp");
        case MINUS:
            if(doub) return Builder->CreateFSub(L, R, "subtmp");
            return Builder->CreateSub(L, R, "subtmp");
        case MUL:
            if(doub) return Builder->CreateFMul(L, R, "multmp");
            return Builder->CreateMul(L, R, "multmp");
        case DIV:
            if(doub) return Builder->CreateFDiv(L, R, "divtmp");
            return Builder->CreateSDiv(L, R, "divtmp");
        case CEQ:
            if(doub) return Builder->CreateFCmpUEQ(L, R, "cmptmp");
            return Builder->CreateICmpEQ(L, R, "cmptmp");
        case CNE:
            if(doub) return Builder->CreateFCmpUNE(L, R, "cmptmp");
            return Builder->CreateICmpNE(L, R, "cmptmp");
        case CLT:
            if(doub) return Builder->CreateFCmpULT(L, R, "cmptmp");
            return Builder->CreateICmpULT(L, R, "cmptmp");
        case CLE:
            if(doub) return Builder->CreateFCmpULE(L, R, "cmptmp");
            return Builder->CreateICmpULE(L, R, "cmptmp");
        case CGT:
            if(doub) return Builder->CreateFCmpUGT(L, R, "cmptmp");
            return Builder->CreateICmpUGT(L, R, "cmptmp");
        case CGE:
            if(doub) return Builder->CreateFCmpUGE(L, R, "cmptmp");
            return Builder->CreateICmpUGE(L, R, "cmptmp");
        default:
            fprintf(stderr, "Invalid binary operator %d\n", op);
            return NULL;
    }
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
    if(DEBUG) printf("Creating assignment for %s\n", lhs.name.c_str());
    if (context.locals().find(lhs.name) == context.locals().end()) {
        fprintf(stderr, "Undeclared variable %s", lhs.name.c_str());
        return NULL;
    }
    Value* expr_val = rhs.codeGen(context);

    if(isa<ConstantFP>(expr_val) && !cast<AllocaInst>(context.locals()[lhs.name])->getAllocatedType()->isDoubleTy()) { // Check if the expr is a double                                                                                         // and if it fits the previously checked type
        fprintf(stderr, "Variable %s is not being assigned with the right type! | Line %d\nAuto converting\n", lhs.name.c_str(), Line);
        Builder->CreateFPToUI(expr_val, Type::getInt64Ty(*myContext));
    }else if(isa<ConstantInt>(expr_val) && !cast<AllocaInst>(context.locals()[lhs.name])->getAllocatedType()->isIntegerTy()) { // Check for int
        fprintf(stderr, "Variable %s is not being assigned with the right type! | Line %d\nAuto converting", lhs.name.c_str(), Line);
        Builder->CreateUIToFP(expr_val, Type::getDoubleTy(*myContext));
    }

    return Builder->CreateStore(expr_val, context.locals()[lhs.name], false);
}

Value* NBlock::codeGen(CodeGenContext& context)
{
    StatementList::const_iterator it;
    Value *last = NULL;
    for (it = statements.begin(); it != statements.end(); it++) {
        last = (**it).codeGen(context);
    }
    if(DEBUG) std::cout << "Creating block" << std::endl;
    return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
    if(DEBUG) std::cout << "Generating code for " << typeid(expression).name() << std::endl;
    return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context) {
    if(DEBUG) printf("Generating return code for %s\n", typeid(expression).name());
    Value *returnValue = expression.codeGen(context);
    context.setCurrentReturnValue(returnValue);
    return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
    if(DEBUG) printf("Creating variable declaration %s of type %s\n", id.name.c_str(), type.name.c_str());

    Type *l_type = typeOf(type);
    AllocaInst *alloc = Builder->CreateAlloca(l_type, nullptr, id.name.c_str());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != NULL) {
        printf("Not null");
		NAssignment assn(id, *assignmentExpr);
		Value* expr_val = assn.codeGen(context);
	}else {
        printf("Null");
        Builder->CreateStore(Constant::getNullValue(l_type), context.locals()[id.name], false);
    }
	return alloc;
}


Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
    vector<Type*> argTypes;
    VariableList::const_iterator it;

    for(it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    FunctionType *ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
    Function *function = Function::Create(ftype, Function::ExternalLinkage, id.name.c_str(), *TheModule);
    BasicBlock *bblock = BasicBlock::Create(*myContext, "entry", function, 0);
    BasicBlock *parentBlock = context.currentBlock();

    context.pushBlock(bblock);
    Builder->SetInsertPoint(context.currentBlock());

    Function::arg_iterator argsValues = function->arg_begin();
    Value* argumentValue;

    for(it = arguments.begin(); it != arguments.end(); it++) {
        (**it).codeGen(context);

        argumentValue = &*argsValues++;
        argumentValue->setName((*it)->id.name.c_str());
        StoreInst *inst = Builder->CreateStore(argumentValue, context.locals()[(*it)->id.name], false);
    }

    block.codeGen(context);
    Builder->CreateRet(context.getCurrentReturnValue());

    context.popBlock();
    Builder->SetInsertPoint(parentBlock);
    if(DEBUG) printf("Creating function %s\n", id.name.c_str());
    return function;
}