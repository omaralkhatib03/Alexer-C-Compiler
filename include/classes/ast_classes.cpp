#pragma once
#include <iostream>
#include <vector>
#include <cstdarg>
#include <map>
#include <string>
#include <stack>
#include "helpers.cpp"
#include "context_classes.cpp"
#include <functional>
#include <numeric>
#include <cstdlib>
#include <regex>
#include <iostream>
#include <cstdint>
#include <sstream>



const std::map<std::string, int> typeSizes = {
    {"char", 1},
    {"short", 2},
    {"int", 4},
    {"long", 4},
    {"long long", 8},
    {"float", 4},
    {"double", 8},
    {"long double", 16},
    {"void", 0}
};


union FloatIntUnion {
    float f;
    uint32_t i;
};

class CommonNode;
class TerminalNode;

typedef CommonNode* CommonNodePtr;

inline CommonNodePtr caseElement = NULL;


class CommonNode
{
public:
    virtual void addChild(CommonNodePtr a)
    {
        children.push_back(a);
    }

    std::vector<CommonNodePtr> getChildren() { return children; }

    virtual void getText(std::ostream &ost)
    {
        for (auto i : children)
        {
            if (i != NULL)
            {
                i->getText(ost);
            }
        }
    }


    virtual void prettyPrint(std::ostream &ost) {ost << " " << typeid(*this).name() << std::endl;}

    virtual void compile(Context *ctx) // default compiler is go through tree
    {
        for (auto i : children)
        {
            if (i != NULL)
            {
                i->compile(ctx);
            }
        }
    }

    virtual ~CommonNode() {}

    
protected:
    std::vector<CommonNodePtr> children;   
};

class TerminalNode
    : public CommonNode
{
public:
    TerminalNode(std::string &str)
    : terminal(str)
    {}

    std::string getTerminal() { return terminal; }

    std::string terminal;

    virtual void getText(std::ostream &ost) 
    override
    { ost << terminal; }

    virtual void prettyPrint(std::ostream &ost)
    override
    { ost << typeid((*this)).name() << " : " << terminal << std::endl; }

    virtual void compile(Context *ctx)
    override
    {}

    virtual ~TerminalNode() {}
};

class UnaryOperator
    : public TerminalNode
{
public:
    UnaryOperator(std::string &x)
    : TerminalNode(x)
    {}

    virtual ~UnaryOperator() {}
};

class StorageClassSpecifierNode // just typeDefs implemented
    : public TerminalNode
{
public:
    StorageClassSpecifierNode(std::string &x)
    : TerminalNode(x)
    {}

    virtual ~StorageClassSpecifierNode() {}
};


class IdentifierNode
    : public CommonNode
{
public:
    IdentifierNode(std::string &x)
    : identifier(x)
    {}

    virtual void getText(std::ostream &ost)
    override
    {
        ost << identifier;
    }

    virtual void prettyPrint(std::ostream &ost)
    override
    {
        ost << typeid((*this)).name() << " : " << identifier << std::endl;
    }

    virtual std::string getId()
    {
        return identifier;
    }

    // could be global or local var, could be function name, array name, etc, check all possibilities
    virtual void compile(Context *ctx)
    override
    {
        if (ctx->sizeOfFlag == 1)
        {
            writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), std::to_string(typeSizes.at(ctx->getSymbol(identifier)->type)));
            return;
        }
        ctx->printAllEnums();
        if (ctx->isEnumId(identifier))
        {
            if (ctx->readWriteToId == 0)
            {
                ctx->out << "li " << ctx->dstRegStack.top() << ", " << ctx->enumValues.at(identifier) << std::endl;
            }
            else
            {

            }
        }
        if (ctx->isFunction(this->identifier))
        {

        }
        else if (ctx->isGlobalSymbol(this->identifier))
        {
            // ctx->isGlobalAssignment = true;
            if (ctx->readWriteToId == true)
            {
                if (ctx->unaryBool != NONE) { throw "Error: note expecting unary operator here";}
                CommonElement *globalSymbol = ctx->getGlobalSymbol(identifier);
                if (globalSymbol == NULL)
                {
                    std::cout << "Error: global symbol not found" << std::endl;
                    exit(1);
                }
                ctx->assignmentQueue.push(globalSymbol);
                std::string addr = ctx->regManager->getReg("a", globalSymbol->type);
                ctx->dstRegStack.push(addr);
                ctx->out << "lui " << ctx->dstRegStack.top() << ", %hi(" << identifier << ")" << std::endl;
                writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), ctx->dstRegStack.top(), "%lo(" + identifier + ")");

            }
            else if(ctx->readWriteToId == false) // reading value of global var
            {
                std::string dst;
                if (ctx->noDstReg)
                {
                    dst = ctx->regManager->getReg("t", ctx->getLocalSymbol(identifier)->type);
                    ctx->dstRegStack.push(dst);
                }
                ctx->out << "lui " << ctx->dstRegStack.top() << ", %hi(" << identifier << ")" << std::endl;
                writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), ctx->dstRegStack.top(), "%lo(" + identifier + ")");
                if (ctx->getGlobalSymbol(identifier)->type == "float")
                {
                    writeSwLw(ctx->out, "flw", ctx->dstRegStack.top(), "0", ctx->dstRegStack.top());
                }
                else
                {
                    writeSwLw(ctx->out, "lw", ctx->dstRegStack.top(), "0", ctx->dstRegStack.top());
                }
                writeDecInc(ctx, identifier, 0);
                writePostDecInc(ctx, identifier, 0);
                if (ctx->noDstReg)
                {
                    ctx->dstRegStack.pop();
                    ctx->regManager->freeReg(dst);
                }

            }

        }
        else if (ctx->isLocalSymbol(this->identifier)) // is a local var, could be array, struct hence check them here
        {   
            if (ctx->readWriteToId == true) //  writing to local var // wont get here if unary is Not None
            {
                if (ctx->unaryBool != NONE) { throw "Error: note expecting unary operator here";}
                CommonElement *localSymbol = ctx->getLocalSymbol(identifier);
                if (localSymbol == NULL)
                {
                    std::cout << "Error: local symbol not found" << std::endl;
                    exit(1);
                }
                ctx->getTopOfScope()->pushAssignmentQueue(localSymbol);
            }
            else if (ctx->readWriteToId == false) // reading value of local var
            {
                std::string dst;
                if (ctx->noDstReg)
                {
                    dst = ctx->regManager->getReg("t", ctx->getLocalSymbol(identifier)->type);
                    ctx->dstRegStack.push(dst);
                }
                if (ctx->getLocalSymbol(identifier)->type == "float")
                {
                    ctx->out << "flw " << ctx->dstRegStack.top() << ", " << ctx->getLocalSymbol(identifier)->offset << "(s0)" << std::endl;
                }
                else
                {
                    ctx->out << "lw " << ctx->dstRegStack.top() << ", " << ctx->getLocalSymbol(identifier)->offset << "(s0)" << std::endl;
                }
                writeDecInc(ctx, identifier, 1);
                writePostDecInc(ctx, identifier, 1);
                if (ctx->noDstReg)
                {
                    ctx->dstRegStack.pop();
                    ctx->regManager->freeReg(dst);
                }
            }
        }
    }

    std::string identifier;
    virtual ~IdentifierNode() {}
};

class ConstantNode
    : public CommonNode
{
public:
    ConstantNode(std::string &x)
    : constant(x)
    {}

    virtual void getText(std::ostream &ost)
    override
    {
        ost << constant;
    }

    virtual void prettyPrint(std::ostream &ost)
    override
    {
        ost << typeid((*this)).name() << " : " << constant << std::endl;
    }

    virtual void compile(Context *ctx)
    override
    {
        std::regex intRegex("[0-9]+");
        std::regex floatRegex("[0-9]+.[0-9]+");
        std::regex charRegex("[a-zA-Z]");
        std::regex fExten("[0-9]+.[0-9]+f");

        if (regex_match(constant, fExten))
        {
            std::string labelL = ctx->makeUnique(".LC");
            std::string rg = ctx->regManager->getReg("a", "int");
            ctx->out << "lui " << rg << ", %hi(" << labelL << ")" << std::endl;
            ctx->out << "flw " << ctx->dstRegStack.top() << ", %lo(" << labelL << ")(" << rg << ")" << std::endl;
            // ctx->ss << ".data" << std::endl;
            ctx->ss << labelL << ":" << std::endl;
            FloatIntUnion fiu;
            std::stringstream ss(constant); 
            ss >> fiu.f;
            ctx->ss << ".word " << fiu.i << std::endl;
            // ctx->out << ".text" << std::endl;
            ctx->regManager->freeReg(rg);
        }
        else
        {
            ctx->out << "li " << ctx->dstRegStack.top() << ", " << constant << std::endl;
        }
    }

    virtual int getValue()
    {
        try
        {
            return std::stod(constant);
        }
        catch(const std::exception& e)
        {
            return (int(constant[0]));
        }
        return 0;
    }

    std::string constant;
    virtual ~ConstantNode() {}
};

class StringLiteralNode
    : public CommonNode
{
public:
    StringLiteralNode(std::string &x)
    : stringLiteral(x)
    {}

    virtual void getText(std::ostream &ost)
    override
    {
        ost << stringLiteral;
    }

    virtual void prettyPrint(std::ostream &ost)
    override
    {
        ost << typeid((*this)).name() << " : " << stringLiteral << std::endl;
    }

    std::string stringLiteral;
    virtual ~StringLiteralNode() {}
};



class PrimaryExpressionNode
    : public CommonNode
{
public:
    PrimaryExpressionNode(std::string &open, CommonNodePtr a, std::string &close)
    {   
        TerminalNode *tmp = new TerminalNode(open);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(close);
        children.push_back(tmp);
    }

    
    virtual ~PrimaryExpressionNode() {}
};


class TypeNode
    : public CommonNode
{
public:
    TypeNode(std::string &strType)
    {
        this->type = strType;
    }

    virtual void getText(std::ostream &ost) 
    override
    { 
        ost << type;
    }

    virtual void prettyPrint(std::ostream &ost)
    override
    { ost << typeid((*this)).name() << " : " << type << std::endl; }


    std::string type;

    virtual ~TypeNode() {}
};

class StorageClassSpec
: public CommonNode
{
public:
    StorageClassSpec(std::string &strType)
    : storageSpec(strType)
    {}

    virtual void getText(std::ostream &ost) 
    override
    { 
        ost << storageSpec;
    }

    std::string storageSpec;

    virtual ~StorageClassSpec() {}
};

class TypeDefNode
    : public CommonNode
{
public:
    TypeDefNode(std::string &newType)
    : userDefType(newType)
    {}
    
    std::string userDefType;

    void getText(std::ostream &ost)
    override
    {
        ost << userDefType;
    }

    void prettyPrint(std::ostream &ost)
    override
    {
        ost << typeid((*this)).name() << " : " << userDefType << std::endl;;
    }

    virtual ~TypeDefNode() {}
};


class DeclarationSpecNode
    : public CommonNode
{
public:
    DeclarationSpecNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    // revise when doing typedef
    virtual std::string getTypeString()
    {
        std::string typeString = "";
        for (int i = 0; i < (int)children.size(); i++)
        {
            if ( children[i] != NULL)
            {
                TypeNode *type = dynamic_cast<TypeNode*>(children[i]);

                if (type != NULL)
                {
                    if (typeString == "")
                    {
                        typeString += type->type; 
                    }
                    else
                    {
                        typeString += " " + type->type;
                    }
                   
                }

            }
            
        }
        return typeString;
    }

    int getTypeSize()
    {
        std::string tString = getTypeString();
        return typeSizes.at(tString);
    }

    virtual ~DeclarationSpecNode() {}
};



class PostfixExpressionNode
    : public CommonNode
{
public:
    PostfixExpressionNode() {}

    PostfixExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    PostfixExpressionNode(CommonNodePtr a, std::string &x)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    PostfixExpressionNode(CommonNodePtr a, std::string &x, std::string &y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    PostfixExpressionNode(CommonNodePtr a, std::string &x, CommonNodePtr b)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
    }

    PostfixExpressionNode(CommonNodePtr a, std::string &x, CommonNodePtr b, std::string &y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    // for function calls
    virtual std::string getId()
    {
        IdentifierNode *tmp = dynamic_cast<IdentifierNode*>(this->children[0]);
        if (tmp != NULL)
        {
            return tmp->getId();
        }
        else
        {
            return "";
        }
    }


    virtual ~PostfixExpressionNode() {}
};

class ArgumentExpressionListNode
    : public CommonNode
{
public:
    ArgumentExpressionListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~ArgumentExpressionListNode() {}
};


class FunctionCallWithArgsNode
    : public PostfixExpressionNode
{
public:
    FunctionCallWithArgsNode(CommonNodePtr a, std::string &x, CommonNodePtr b, std::string &y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    virtual void compile(Context *ctx)
    override
    {
        // std::cout << " return func " << ctx->returnFunc << " mv expression func " << ctx->mvExpressionFunc << std::endl;
        std::string id = this->getId();
        Function* callee = dynamic_cast<Function*>(ctx->getGlobalSymbol(id)); 
        if (callee == NULL) { throw std::runtime_error("function not found"); }
        ArgumentExpressionListNode* args = dynamic_cast<ArgumentExpressionListNode*>(children[2]);
        if (args == NULL) { throw "expecting argument expression list"; }
        int spOffset = 0;
        for (int i = 0; i < (int)args->getChildren().size(); i++)
        {
            if ((dynamic_cast<Variable*>(callee->params[i]))) // checking what is param
            {
                Variable *var = dynamic_cast<Variable*>(callee->params[i]);
                if (var == NULL) { throw std::runtime_error("variable not found"); }
                if (var->type == "float")
                {

                    std::string dst = (i == 0) ? ctx->regManager->getRetVal("float") : ctx->regManager->getReg("a", "float");

                    if (dst == "-1")
                    {
                        dst = (i == 0) ? ctx->regManager->getRetVal("int") : ctx->regManager->getReg("a", "int");
                        if (dst == "-1")
                        {
                            dst = (ctx->regManager->getReg("t", "float") == "-1") ? ctx->regManager->getReg("s", "float") : ctx->regManager->getReg("t", "float");
                            ctx->dstRegStack.push(dst);
                            args->getChildren()[i]->compile(ctx);
                            ctx->out << "fsw " << dst << spOffset << "(s0)" << std::endl;
                            spOffset += 4;
                            ctx->regManager->freeReg(ctx->dstRegStack.top());
                            ctx->dstRegStack.pop();
                        }

                    }
                    else
                    {
                        args->getChildren()[i]->compile(ctx);
                        ctx->dstRegStack.push(dst);
                    }
                    
                }
                else // int, char, unsigned
                {
                    std::string dst = (i == 0) ? ctx->regManager->getRetVal("int") : ctx->regManager->getReg("a", "int");

                    if (dst == "-1") // put it on stack
                    {
                        dst = (ctx->regManager->getReg("t", "int") == "-1") ? ctx->regManager->getReg("s", "int") : ctx->regManager->getReg("t", "int");
                        ctx->dstRegStack.push(dst);
                        args->getChildren()[i]->compile(ctx);
                        ctx->out << "sw " << dst << spOffset << "(s0)" << std::endl;
                        spOffset += (var->type == "char") ? 1 : 4;
                        ctx->regManager->freeReg(ctx->dstRegStack.top());
                        ctx->dstRegStack.pop();   
                        
                    }
                    else 
                    {
                        ctx->dstRegStack.push(dst);
                        args->getChildren()[i]->compile(ctx);
                    }
                }
            } // check array, pointer etc
        }
        while (ctx->dstRegStack.top() != "a0" || ctx->dstRegStack.top() != "a0") 
        {
            if (ctx->dstRegStack.top() == "-1")
            {
                ctx->dstRegStack.pop();
                continue;
            }
            ctx->regManager->freeReg(ctx->dstRegStack.top());
            ctx->dstRegStack.pop();
        }
        ctx->dstRegStack.pop();
        
        ctx->out << "call " << this->getId() << std::endl;

        if ((ctx->returnFunc || ctx->mvExpressionFunc) && !(ctx->dstRegStack.empty()))
        {
            if (ctx->regManager->getRegType(ctx->dstRegStack.top()) == "float")
            {
                writeTwoIns(ctx->out, "fmv.s", ctx->dstRegStack.top(), "fa0");
            }
            else
            {
                writeTwoIns(ctx->out, "mv", ctx->dstRegStack.top(), "a0");
            }
            ctx->returnFunc = false;
            ctx->mvExpressionFunc = false;
        }
    }

    virtual ~FunctionCallWithArgsNode() {}
};


class FunctionCallWithoutArgsNode
    : public PostfixExpressionNode
{
public:
    FunctionCallWithoutArgsNode(CommonNodePtr a, std::string &x, std::string &y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    virtual void compile(Context *ctx)
    override
    {   
        ctx->out << "call " << this->getId() << std::endl;
        if (ctx->returnFunc || ctx->mvExpressionFunc)
        {
            if (ctx->regManager->getRegType(ctx->dstRegStack.top()) == "float")
            {
                writeTwoIns(ctx->out, "fmv.s", ctx->dstRegStack.top(), "fa0");
            }
            else
            {
                writeTwoIns(ctx->out, "mv", ctx->dstRegStack.top(), "a0");
            }
            ctx->returnFunc = false;
            ctx->mvExpressionFunc = false;
        }
    }

    virtual ~FunctionCallWithoutArgsNode() {}
};



class GeneralExpressions
    : public CommonNode
{
public:
    GeneralExpressions()
    {}

    GeneralExpressions (CommonNodePtr a)
    {
        children.push_back(a);
    }

    GeneralExpressions(CommonNodePtr a, std::string &op, CommonNodePtr b)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(op);
        children.push_back(tmp);
        children.push_back(b);
    }
    
    CommonNodePtr getRight()
    {
        return children[2];
    }

    CommonNodePtr getLeft()
    {
        return children[0];
    }

    std::string getOperator()
    {
        TerminalNode *tmp = dynamic_cast<TerminalNode*>(children[1]);
        return tmp->getTerminal();
    }

    void virtual compile(Context *ctx)
    override
    {
        if (children.size() == 1)
        {
            children[0]->compile(ctx);
        }
        else
        {
            if (!ctx->assignmentQueue.empty())
            {
                if (ctx->assignmentQueue.front()->type == "unsigned" || ctx->assignmentQueue.front()->type == "char")
                {
                    ctx->unsignedFlag = true;
                }
            }
            else if (!ctx->getTopOfScope()->assignmentQueue.empty())
            {
                if (ctx->getTopOfScope()->assignmentQueue.front()->type == "unsigned" || ctx->getTopOfScope()->assignmentQueue.front()->type == "char")
                {
                    ctx->unsignedFlag = true;
                }
            }
            bool recursiveLeft = dynamic_cast<FunctionCallWithArgsNode*>(this->getLeft()) != NULL || dynamic_cast<FunctionCallWithoutArgsNode*>(this->getLeft()) != NULL;
            bool recursiveRight = dynamic_cast<FunctionCallWithArgsNode*>(this->getRight()) != NULL || dynamic_cast<FunctionCallWithoutArgsNode*>(this->getRight()) != NULL;

            std::string srcA = ctx->regManager->getReg((recursiveRight) ? "s" : "t", ((ctx->regManager->getRegType(ctx->dstRegStack.top()) == "f") ? "float" : "int"));
            ctx->dstRegStack.push(srcA);
            ctx->mvExpressionFunc = 1;
            this->getLeft()->compile(ctx);

            std::string logicalLabel1 = ctx->makeUnique("logicalLabel");
            if (this->getOperator()=="&&")
            {
                writeThreeIns(ctx->out, "beq", ctx->dstRegStack.top(), "zero", logicalLabel1);
            }
            else if (this->getOperator()=="||")
            {
                writeThreeIns(ctx->out, "bne", ctx->dstRegStack.top(), "zero", logicalLabel1);
            }

            ctx->dstRegStack.pop();
            std::string srcB = ctx->regManager->getReg((recursiveLeft) ? "s" : "t", ((ctx->regManager->getRegType(ctx->dstRegStack.top()) == "f") ? "float" : "int"));
            ctx->dstRegStack.push(srcB);
            ctx->mvExpressionFunc = 1;
            this->getRight()->compile(ctx);
            std::string logicalLabel2 = ctx->makeUnique("logicalLabel");
            if (this->getOperator()=="&&")
            {   
                writeThreeIns(ctx->out, "beq", ctx->dstRegStack.top(), "zero", logicalLabel1);
                ctx->dstRegStack.pop();
                writeTwoIns(ctx->out, "li" , ctx->dstRegStack.top(), "1");
                writeOneIns(ctx->out, "j", logicalLabel2);
                ctx->out << logicalLabel1 << ":" << std::endl;
                writeTwoIns(ctx->out, "li" , ctx->dstRegStack.top(), "0");
                ctx->out << logicalLabel2 << ":" << std::endl;
            }
            else if (this->getOperator()=="||")
            {
                writeThreeIns(ctx->out, "beq", ctx->dstRegStack.top(), "zero", logicalLabel2);
                ctx->dstRegStack.pop();
                ctx->out << logicalLabel1 << ":" << std::endl;
                writeTwoIns(ctx->out, "li" , ctx->dstRegStack.top(), "1");
                std::string logicalLabel3 = ctx->makeUnique("logicalLabel");
                writeOneIns(ctx->out, "j", logicalLabel3);
                ctx->out << logicalLabel2 << ":" << std::endl;
                writeTwoIns(ctx->out, "li" , ctx->dstRegStack.top(), "0");
                ctx->out << logicalLabel3 << ":" << std::endl;
            }

            if (this->getOperator()=="&&" || this->getOperator()=="||")
            {
                ctx->regManager->freeReg(srcA);
                ctx->regManager->freeReg(srcB);
                return;
            }
            ctx->dstRegStack.pop();
            // ctx->dstRegStack.pop();
            if (ctx->unsignedFlag == 1)
            {
                writeUnsigned(ctx->out, this->getOperator(), ctx->dstRegStack.top(), srcA, srcB);
                ctx->unsignedFlag = 0;
            }
            else
            {
                ((ctx->regManager->getRegType(ctx->dstRegStack.top()) == "f")) ? writeOperation(ctx->out, this->getOperator(), ctx->dstRegStack.top(), srcA, srcB, 1) : writeOperation(ctx->out, this->getOperator(), ctx->dstRegStack.top(), srcA, srcB, 0);
            }
            ctx->regManager->freeReg(srcA);
            ctx->regManager->freeReg(srcB);
            // if (recursiveLeft || recursiveRight)
            // {
            //     ctx->dstRegStack.pop();
            //     if ((ctx->regManager->getRegType(ctx->dstRegStack.top()) == "f"))
            //     {
            //         ctx->out << "fmv.s " << ctx->dstRegStack.top() << ", " << dst << std::endl;
            //     }
            //     else
            //     {
            //         ctx->out << "mv " << ctx->dstRegStack.top() << ", " << dst << std::endl;
            //     }

                // ctx->regManager->freeReg(dst);
            // }

        }
    }

    virtual int getValue()
    {
        GeneralExpressions* tmpL1 = dynamic_cast<GeneralExpressions*>(children[0]);
        ConstantNode* tmpL2 = dynamic_cast<ConstantNode*>(children[0]);
        GeneralExpressions* tmpR1 = dynamic_cast<GeneralExpressions*>(children[2]);
        ConstantNode* tmpR2 = dynamic_cast<ConstantNode*>(children[2]);

        int left = (tmpL1 == NULL ) ? tmpL2->getValue() : tmpL1->getValue();
        int right = (tmpR1 == NULL ) ? tmpR2->getValue() : tmpR1->getValue();
        
        if (this->getOperator() == "+")
        {
            return left + right;
        }
        else if (this->getOperator() == "-")
        {
            return left - right;
        }
        else if (this->getOperator() == "*")
        {
            return left * right;
        }
        else if (this->getOperator() == "/")
        {
            return left / right;
        }
        else if (this->getOperator() == "%")
        {
            return left % right;
        }
        else if (this->getOperator() == "==")
        {
            return left == right;
        }
        else if (this->getOperator() == "!=")
        {
            return left != right;
        }
        else if (this->getOperator() == ">")
        {
            return left > right;
        }
        else if (this->getOperator() == ">=")
        {
            return left >= right;
        }
        else if (this->getOperator() == "<")
        {
            return left < right;
        }
        else if (this->getOperator() == "<=")
        {
            return left <= right;
        }
        else if (this->getOperator() == "&&")
        {
            return left && right;
        }
        else if (this->getOperator() == "||")
        {
            return left || right;
        }
        else if (this->getOperator() == "&")
        {
            return left & right;
        }
        else if (this->getOperator() == "|")
        {
            return left | right;
        }
        else if (this->getOperator() == "^")
        {
            return left ^ right;
        }
        else if (this->getOperator() == "<<")
        {
            return left << right;
        }
        else if (this->getOperator() == ">>")
        {
            return left >> right;
        }
        else
        {
            return 0;
        }
    }

    virtual ~GeneralExpressions() {}
};


class DirDeclaratorNode
    : public CommonNode
{
public:
    DirDeclaratorNode()
    {}
    
    DirDeclaratorNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual std::string getId(){return "";}

    virtual ~DirDeclaratorNode() {}
};

class IdDirDeclarator
    : public DirDeclaratorNode
{
public:
    IdDirDeclarator(std::string &x)
    :id(x)
    {}

    virtual std::string getId()
    override
    {
        return id;
    }

    virtual void prettyPrint(std::ostream &ost)
    {
        ost << typeid((*this)).name() << " : " << id << std::endl;
    }

    std::string id;

    virtual ~IdDirDeclarator() {}
};


class FuncDirDeclNode
    : public DirDeclaratorNode
{
public:
    FuncDirDeclNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual std::string getId()
    override
    {
        IdDirDeclarator *id = dynamic_cast<IdDirDeclarator*>(children[0]);
        if (id == NULL){return "";}
        return id->getId();
    }

    virtual ~FuncDirDeclNode() {}
};


class FuncDirDeclParamsNode
    : public DirDeclaratorNode
{
public:
    FuncDirDeclParamsNode(CommonNodePtr a, std::string x ,CommonNodePtr b, std::string y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    virtual std::string getId()
    {
        IdDirDeclarator *id = dynamic_cast<IdDirDeclarator*>(children[0]);
        if (id == NULL){return "";}
        return id->getId();
    }

    virtual void compile(Context *ctx)
    override
    {
        
        children[2]->compile(ctx);
    }


    virtual ~FuncDirDeclParamsNode() {}
};



class ConstantExpressionNode
    : public CommonNode
{
public:
    ConstantExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual int getValue()
    {
        GeneralExpressions *tmp = dynamic_cast<GeneralExpressions*>(children[0]);
        ConstantNode *tmp2 = dynamic_cast<ConstantNode*>(children[0]);
        return (tmp == NULL ) ? tmp2->getValue() : tmp->getValue();
    }

    virtual ~ConstantExpressionNode() {}
};


class ArrayDirDeclaratorWithSize
    : public DirDeclaratorNode
{
public:
    ArrayDirDeclaratorWithSize(CommonNodePtr a, std::string &x, CommonNodePtr b, std::string y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    virtual std::string getId()
    {
        if (dynamic_cast<IdDirDeclarator*>(children[0]) != NULL)
        {
            return dynamic_cast<IdDirDeclarator*>(children[0])->getId();
        }
        else if (dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0]) != NULL)
        {
            return dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0])->getId();
        }
        else
        {
            return "";
        }
    }

    virtual int getSize()
    {      
        int t = 1;
        
        ArrayDirDeclaratorWithSize* multDim = dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0]);
        if (multDim != NULL)
        {
            t *= multDim->getSize();
        }
        
        ConstantExpressionNode *tmp = dynamic_cast<ConstantExpressionNode*>(children[2]);
        if (tmp == NULL){throw std::runtime_error("no constant expression found ");}

        t *= tmp->getValue();
        return t;
    }

    virtual std::vector<int> getArrSizes()
    {
        std::vector<int> t;
        ArrayDirDeclaratorWithSize* multDim = dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0]);
        if (multDim != NULL)
        {
            t = multDim->getArrSizes();
        }
        ConstantExpressionNode *tmp = dynamic_cast<ConstantExpressionNode*>(children[2]);
        if (tmp == NULL){throw std::runtime_error("no constant expression found ");}
        t.push_back(tmp->getValue());
        return t;
    }

    virtual ~ArrayDirDeclaratorWithSize() {}
};


class ArrayDirDeclaratorWithoutSize
    : public CommonNode
{
public:
    ArrayDirDeclaratorWithoutSize(CommonNodePtr a, std::string &x, std::string &y)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }
    virtual ~ArrayDirDeclaratorWithoutSize() {}
};


class DeclaratorNode
    : public CommonNode
{
public:

    DeclaratorNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }
    
    
    DeclaratorNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    std::string getDirDeclaratorId()
    {
        if (children.size() == 1)
        {
            DirDeclaratorNode* tmp = dynamic_cast<DirDeclaratorNode*>(children[0]);
            if (tmp != NULL){return tmp->getId();}

        }
        else
        {
            DirDeclaratorNode* tmp = dynamic_cast<DirDeclaratorNode*>(children[1]);
            if (tmp != NULL){return tmp->getId();}
        }
        return "";
    }

    virtual int getTotalDeclarations()
    {
        if ((int)children.size() == 2) // pointer declaration
        {
            return 1;
        }  
        else if ((int)children.size() == 1) // direct declaration
        {
            if (dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0]) != NULL)
            {
                return dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0])->getSize();
            }
            else if (dynamic_cast<IdDirDeclarator*>(children[0]) != NULL)
            {
                return 1;
            }
            else // should never get here
            {
                return -1;
            }
        }
        // not a pointer, array, or var
        return -1;
    }

    bool isArr()
    {
        return (int)children.size() == 1 && dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0]) != NULL;
    }

    bool isVar()
    {
        return (int)children.size() == 1 && dynamic_cast<IdDirDeclarator*>(children[0]) != NULL;
    }


    bool isPtr() // is pointer
    {
        return (int)children.size() == 2;
    }

    bool isFunction()
    {
        return ((int)children.size() == 1 && dynamic_cast<FuncDirDeclParamsNode*>(children[0]) != NULL) || ((int)children.size() == 1 && dynamic_cast<FuncDirDeclNode*>(children[0]) != NULL);
    }

    std::vector<int> getArrSizes()
    {
      return dynamic_cast<ArrayDirDeclaratorWithSize*>(children[0])->getArrSizes(); 
    }

    virtual ~DeclaratorNode() {}

};

class InitDeclaratorNode
    : public CommonNode
{
public:
    InitDeclaratorNode(CommonNodePtr a)
    {
        children.push_back(a);
    }
    InitDeclaratorNode(CommonNodePtr a, std::string &x, CommonNodePtr b)
    : equal(x)
    {
        children.push_back(a);
        children.push_back(b);
    }

    virtual void getText(std::ostream &ost)
    override
    {
        if (children.size() > 1)
        {
            children[0]->getText(ost);
            ost << equal;
            children[1]->getText(ost);
        }
        else 
        {
            children[0]->getText(ost);
        }
    }
    std::string equal;
    virtual ~InitDeclaratorNode() {}
};


class InitializerListNode
    : public CommonNode
{
public:
    InitializerListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void compile(Context *ctx)
    override
    {
        if (!ctx->isGlobalScope())
        {
            CommonElement *el = ctx->getTopOfScope()->getAssignmentFront();
            if (el == NULL) {return;}
            if (dynamic_cast<Variable*>(el) != NULL) {children[0]->compile(ctx);}
            if (dynamic_cast<Array*>(el) != NULL) 
            {
                for (int i = 0; i < (int)children.size(); i++)
                {
                    
                    children[i]->compile(ctx);
                }
            } 
        }
        else
        {
            CommonElement *el = ctx->getAssignmentFront();
            if (el == NULL) {return;}
            if (dynamic_cast<Variable*>(el) != NULL) {children[0]->compile(ctx);}
            if (dynamic_cast<Array*>(el) != NULL) 
            {
                for (int i = 0; i < (int)children.size(); i++)
                {
                    children[i]->compile(ctx);
                }
            } 
        }
    }

    virtual ~InitializerListNode() {}
};


// x = y, (y is the assigner, its assigning a value to the asignee x)
class AssignerExpressionNode
    : public CommonNode
{
public:
    AssignerExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void compile(Context *ctx)
    override
    {
        if (!ctx->isGlobalScope())
        {
            ctx->readWriteToId = 0;
            
            if (ctx->dstRegStack.empty())
            {
                ctx->noDstReg = 1;
                children[0]->compile(ctx);
                ctx->noDstReg = 0;
                return;
            }

            children[0]->compile(ctx);
        }
        else
        {

        }
    }

    virtual int getValue()
    {
        GeneralExpressions* tmp = dynamic_cast<GeneralExpressions*>(children[0]);
        ConstantNode *tmp2 = dynamic_cast<ConstantNode*>(children[0]);
        return (tmp == NULL) ? tmp2->getValue() : tmp->getValue();
    }

    virtual ~AssignerExpressionNode() {}
};

class ExpressionNode
    : public CommonNode
{
public:
    ExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }
    ExpressionNode(CommonNodePtr a, std::string &x, CommonNodePtr b)
    : comma(x)
    {
        children.push_back(a);
        children.push_back(b);
    }
    
    virtual int getValue()
    {
        AssignerExpressionNode* tmp = dynamic_cast<AssignerExpressionNode*>(children[0]);
        if (tmp == NULL){throw std::runtime_error("Cannot get value of an assignment");}
        return tmp->getValue();
    }

    std::string comma;
    virtual ~ExpressionNode() {}

};

class InitializerNode
    : public CommonNode
{
public:
    InitializerNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    InitializerNode(std::string &x, CommonNodePtr a, std::string &y)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    InitializerNode(std::string &x, CommonNodePtr a, std::string &y, std::string &z)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
    }

    virtual void compile (Context *ctx)
    override
    {
        // std::cout << " hit initializer node" << std::endl;
        if (children.size() == 1)
        {
            if (!ctx->isGlobalScope())
            {
                std::string dst = ctx->regManager->getReg("t", ctx->getTopOfScope()->getAssignmentFront()->type);
                ctx->dstRegStack.push(dst);
                ctx->returnFunc = 1;
                children[0]->compile(ctx);
                ctx->returnFunc = 0;
                if (dynamic_cast<Variable*>(ctx->getTopOfScope()->getAssignmentFront()) != NULL)
                {
                    if (dynamic_cast<Variable*>(ctx->getTopOfScope()->getAssignmentFront())->type == "float")
                    {
                        ctx->out << "fsw " << ctx->dstRegStack.top() << "," << ctx->getTopOfScope()->getAssignmentFront()->offset << "(s0)" << std::endl;

                    }
                    else
                    {
                        ctx->out << "sw " << ctx->dstRegStack.top() << "," << ctx->getTopOfScope()->getAssignmentFront()->offset << "(s0)" << std::endl;
                    }
                }
                else if (dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront()) != NULL)
                {
                    if (dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront())->type == "float")
                    {
                        ctx->out << "fsw " << ctx->dstRegStack.top() << "," << dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront())->assignerOffset << "(s0)" << std::endl;
                        dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront())->decrementAssigner(typeSizes.at(ctx->getTopOfScope()->getAssignmentFront()->type));
                    }
                    else
                    {
                        ctx->out << "sw " << ctx->dstRegStack.top() << "," << dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront())->assignerOffset << "(s0)" << std::endl;
                        dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront())->decrementAssigner(typeSizes.at(ctx->getTopOfScope()->getAssignmentFront()->type));
                    }
                }
                ctx->regManager->freeReg(ctx->dstRegStack.top());
                ctx->dstRegStack.pop();
            }
            else 
            {
                if (dynamic_cast<Variable*>(ctx->getAssignmentFront()) != NULL)
                {
                    AssignerExpressionNode *tmp = dynamic_cast<AssignerExpressionNode*>(children[0]);
                    if (tmp == NULL) {throw std::runtime_error("expecting AssignerExpressionNode, got NULL");}
                    int constVal =  tmp->getValue();
                    std::string type = ctx->getAssignmentFront()->type;
                    if (type == "char")
                    {
                        ctx->out << ".byte " << constVal << std::endl;
                    }
                    else if (type == "double")
                    {
                        throw std::runtime_error("double not supported yet");
                    }
                    else
                    {
                        ctx->out << ".word " << constVal << std::endl;

                    }
                }
                else if (dynamic_cast<Array*>(ctx->getAssignmentFront()) != NULL)
                {
                    AssignerExpressionNode *tmp = dynamic_cast<AssignerExpressionNode*>(children[0]);
                    if (tmp == NULL) {throw std::runtime_error("expecting AssignerExpressionNode, got NULL");}
                    int constVal =  tmp->getValue();
                    std::string type = ctx->getAssignmentFront()->type;
                    if (type == "char")
                    {
                        ctx->out << ".byte " << constVal << std::endl;
                    }
                    else if (type == "double")
                    {
                        throw std::runtime_error("double not supported yet");
                    }
                    else
                    {
                        ctx->out << ".word " << constVal << std::endl;
                    }
                    ctx->arrayGlobalCounter++;
                }
                else
                {
                    throw std::runtime_error("expecting Variable or Array, got NULL");
                }

            }
        }
        else
        {
            children[1]->compile(ctx);
        }
    }

    virtual ~InitializerNode() {} 
};

class InitDeclarationListNode
    : public CommonNode
{
public:
    
    InitDeclarationListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    int getTotalDeclarations()
    {
        int total = 0;
        for (auto &child : children)
        {
            InitDeclaratorNode *tmp = dynamic_cast<InitDeclaratorNode*>(child);
            if (tmp == NULL) {std::cout << "no initDeclaDetected" << std::endl; continue;}
            DeclaratorNode *dcl = dynamic_cast<DeclaratorNode*>(tmp->getChildren()[0]);
            if (dcl == NULL) {std::cout << "no declarator detected" << std::endl; continue;}
            int t = dcl->getTotalDeclarations();
            total += (t == -1) ? 0 : t;
        }
        return total;
    }

    InitDeclarationListNode(){}

    virtual ~InitDeclarationListNode() {}
};


class FunctionNode 
    : public CommonNode
{
public:

    FunctionNode(CommonNodePtr a, CommonNodePtr b, CommonNodePtr c)
    {
        children.push_back(a);
        children.push_back(b);
        children.push_back(c);
    }

    virtual void compile(Context *ctx)
    override
    {
        DeclarationSpecNode *declSpec = dynamic_cast<DeclarationSpecNode*>(children[0]);
        if (declSpec == NULL){return;}
        std::string type = declSpec->getTypeString();
        std::string id;
        DeclaratorNode *decl = dynamic_cast<DeclaratorNode*>(children[1]);
        if (decl == NULL){return;}
        id = decl->getDirDeclaratorId();
        Scope *func = new Function(id, type);
        ctx->addFunction(func);
        ctx->pushCurrentScope(func);
        ctx->inFunction = true;
        ctx->out << ".text" << std::endl;
        ctx->out << ".globl " << id << std::endl;
        ctx->out << id << ":" << std::endl;
        ctx->out << "addi sp,sp," << -ctx->getFunctionSize(id) << std::endl;
        if (ctx->isCaller(id))
        {
            ctx->out << "sw ra," << ctx->getFunctionSize(id)-4 << "(sp)" << std::endl;
            ctx->out << "sw s0," << ctx->getFunctionSize(id)-8 << "(sp)" << std::endl;
            ctx->out << "sw s1," << ctx->getFunctionSize(id)-12 << "(sp)" << std::endl;

        }
        else
        {
            ctx->out << "sw s0," << ctx->getFunctionSize(id)-4 << "(sp)" << std::endl;
        }
        ctx->out << "addi s0,sp," << ctx->getFunctionSize(id) << std::endl;
        ctx->returnLabel = ctx->makeUnique("return");
        children[1]->compile(ctx);
        children[2]->compile(ctx);
        ctx->out << ctx->returnLabel << ":" << std::endl;
        ctx->returnLabel = "";
        if (ctx->isCaller(id))
        {
            ctx->out << "lw ra," << ctx->getFunctionSize(id)-4 << "(sp)" << std::endl;
            ctx->out << "lw s0," << ctx->getFunctionSize(id)-8 << "(sp)" << std::endl;
            ctx->out << "lw s1," << ctx->getFunctionSize(id)-12 << "(sp)" << std::endl;
        }
        else
        {
            ctx->out << "lw s0," << ctx->getFunctionSize(id)-4 << "(sp)" << std::endl;
        }
        ctx->out << "addi sp,sp," << ctx->getFunctionSize(id) << std::endl;
        ctx->out << "jr ra" << std::endl;
    }

    virtual ~FunctionNode() {}
};


class DeclarationNode
    : public CommonNode
{
public:
    DeclarationNode(){}

    DeclarationNode(CommonNodePtr a/*, std::string &x*/)
    {
        children.push_back(a);
        // TerminalNode *semi = new TerminalNode(x);
        // children.push_back(semi);
    }

    DeclarationNode(CommonNodePtr a, CommonNodePtr b/*, std::string x*/)
    {
        children.push_back(a);
        children.push_back(b);
        // TerminalNode *semi = new TerminalNode(x);
        // children.push_back(semi);
    }

    virtual void compile(Context *ctx)
    override
    {
        // get type 
        DeclarationSpecNode *declSpec = dynamic_cast<DeclarationSpecNode*>(children[0]);
        if (declSpec == NULL){return;}
        std::string type = declSpec->getTypeString();

        // if i have stuff to declare
        // doesnt deal with global vars yet
        if (!ctx->isGlobalScope())
        {
        if (children.size() == 2)
            {
                InitDeclarationListNode *initDeclList = dynamic_cast<InitDeclarationListNode*>(children[1]);
                if (initDeclList == NULL){return;}

                for (int i = 0; i < (int) initDeclList->getChildren().size(); i++)
                {
                    InitDeclaratorNode *initDecl = dynamic_cast<InitDeclaratorNode*>(initDeclList->getChildren()[i]);
                    if (initDecl == NULL){return;}

                    DeclaratorNode *declNode = dynamic_cast<DeclaratorNode*>(initDecl->getChildren()[0]);
                    if (declNode == NULL){return;}
                    std::string id = declNode->getDirDeclaratorId();

                    // TODO: check if array or var, then compile 
                    // std::cout << " im a var: " << declNode->isVar() << " im an arr: " << declNode->isArr() << std::endl;
                    if (declNode->isVar())
                    {
                        Variable *var = new Variable(id, type, ctx->getCurrentFunction()->offset);
                        ctx->getTopOfScope()->addSymbol(id, var);
                        ctx->getCurrentFunction()->decrementOffset(typeSizes.at(type));
                        if (i == (int) initDeclList->getChildren().size() - 1)
                        {
                            ctx->getTopOfScope()->pushAssignmentQueue(var);
                        }
                    }
                    else if (declNode->isArr())
                    {
                        Array *arr = new Array(id, type, ctx->getCurrentFunction()->offset, declNode->getArrSizes());
                        ctx->getTopOfScope()->addSymbol(id, arr);
                        ctx->getCurrentFunction()->decrementOffset(arr->getSize()*typeSizes.at(type));
                        
                        if (i == (int) initDeclList->getChildren().size() - 1)
                        {
                            ctx->getTopOfScope()->pushAssignmentQueue(arr);
                        }

                    }
                    if (i == (int) initDeclList->getChildren().size() - 1)
                    {
                        InitializerNode *initzr = dynamic_cast<InitializerNode*> (initDecl->getChildren()[1]);
                        
                        if (initzr != NULL)
                        {
                            initzr->compile(ctx);
                            
                            if (declNode->isArr())
                            {
                                Array* tmp = dynamic_cast<Array*>(ctx->getTopOfScope()->getAssignmentFront());
                                if (tmp == NULL){throw std::runtime_error("not an array");}
                                // std::cout << (std::abs((std::abs(tmp->offset) + tmp->assignerOffset))) << "  " << tmp->getSize()*typeSizes.at(tmp->type) << std::endl;
                                while ( (std::abs((std::abs(tmp->offset) + tmp->assignerOffset))) != tmp->getSize()*typeSizes.at(tmp->type))
                                {
                                    if (tmp->type == "float")
                                    {
                                        writeSwLw(ctx->out, "fsw", "zero", std::to_string(tmp->assignerOffset), "s0");
                                    }
                                    else
                                    {
                                        writeSwLw(ctx->out, "sw", "zero", std::to_string(tmp->assignerOffset), "s0");
                                    }
                                    tmp->decrementAssigner(typeSizes.at(type));
                                }
                            }
                            ctx->getTopOfScope()->popAssignmentQueue();
                        }
                        else 
                        {

                            ctx->getTopOfScope()->popAssignmentQueue();
                            break;
                        }
                    }
                }
            }
        }
        else // global declarations // TODO
        {
            if (children.size() == 2)
            {
                ctx->out << ".data" << std::endl;
                InitDeclarationListNode *initDeclList = dynamic_cast<InitDeclarationListNode*>(children[1]);
                if (initDeclList == NULL){return;}

                for (int i = 0; i < (int) initDeclList->getChildren().size(); i++)
                {
                    InitDeclaratorNode *initDecl = dynamic_cast<InitDeclaratorNode*>(initDeclList->getChildren()[i]);
                    if (initDecl == NULL){return;}

                    DeclaratorNode *declNode = dynamic_cast<DeclaratorNode*>(initDecl->getChildren()[0]);
                    if (declNode == NULL){return;}
                    std::string id = declNode->getDirDeclaratorId();
                    if (declNode->isVar())
                    {
                        Variable *var = new Variable(id, type);
                        ctx->addGlobalSymbol(id, var);
                        ctx->out << id << ":" << std::endl;
                        ctx->pushAssignmentQueue(var);
                    }
                    else if (declNode->isArr())
                    {
                        Array *arr = new Array(id, type, 0, declNode->getArrSizes());
                        ctx->addGlobalSymbol(id, arr);
                        ctx->out << id << ":" << std::endl;   
                        ctx->pushAssignmentQueue(arr);
                    }
                    else if (declNode->isFunction())
                    {
                        Function *func = new Function(id, type);
                        ctx->addGlobalSymbol(id, func);
                        ctx->pushCurrentScope(func);
                        ctx->externalFunction = 1;
                        declNode->compile(ctx);
                        ctx->scopeStack.pop();
                        ctx->externalFunction = 0;
                        return;
                    }
                    if (i == (int) initDeclList->getChildren().size() - 1)
                    {
                        
                        InitializerNode *initzr = dynamic_cast<InitializerNode*> (initDecl->getChildren()[1]);
                        // std::cout << " compiling " << std::endl; 
                        if (initzr != NULL)
                        {
                            initzr->compile(ctx);
                            
                            if (declNode->isArr())
                            {
                                Array* tmp = dynamic_cast<Array*>(ctx->getAssignmentFront());
                                if (tmp == NULL){throw std::runtime_error("not an array");}
                                int t = tmp->getSize() - ctx->arrayGlobalCounter;
                                if (t > 0)
                                {
                                    ctx->out << ".zero " << t*typeSizes.at(tmp->type) << std::endl;
                                }
                                ctx->arrayGlobalCounter = 0;
                            }
                            ctx->popAssignmentQueue();
                        }
                        else 
                        {
                            Array* tmp = dynamic_cast<Array*>(ctx->getAssignmentFront());
                            if (tmp != NULL)
                            {
                                ctx->out << ".zero " << tmp->getSize()*typeSizes.at(tmp->type) << std::endl;
                            }
                            ctx->popAssignmentQueue();
                            break;
                        }
                    }
                }                
            }
            else if ((int)children.size() == 1)
            {
                children[0]->compile(ctx);
            }
        }   
    }


    virtual ~DeclarationNode() {}
};


class ParameterDeclarationNode
    : public DeclarationNode
{
public:
    ParameterDeclarationNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    ParameterDeclarationNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual int getTypeSize()
    {
        DeclarationSpecNode *declSpec = dynamic_cast<DeclarationSpecNode*>(children[0]);
        if (declSpec == NULL){return 0;}
        std::string type = declSpec->getTypeString();
        return typeSizes.at(type);
    }

    virtual std::string getTypeString()
    {
        DeclarationSpecNode *declSpec = dynamic_cast<DeclarationSpecNode*>(children[0]);
        if (declSpec == NULL){return "";}
        std::string type = declSpec->getTypeString();
        return type;
    }

    virtual ~ParameterDeclarationNode() {}
};


class ParamListNode
    : public CommonNode
{
public:
    ParamListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void compile(Context *ctx)
    override
    {

        std::string dst;
        int offsetCounter = 0;
        for (int i = 0; i < (int)children.size(); i++)
        {
            ParameterDeclarationNode *paramDecl = dynamic_cast<ParameterDeclarationNode*>(children[i]);
            if (paramDecl == NULL){return;}
            std::string type = paramDecl->getTypeString();
            std::string id;
            DeclaratorNode *decl = dynamic_cast<DeclaratorNode*>(paramDecl->getChildren()[1]);
            if (decl == NULL){return;}
            id = decl->getDirDeclaratorId();
            Variable *var = NULL;
            if (ctx->externalFunction)
            {
                var = new Variable(id, type);
                dynamic_cast<Function*>(ctx->getCurrentFunction())->params.push_back(var);
                continue;
            }
            else
            {
                var = new Variable(id, type, ctx->getCurrentFunction()->offset);
                ctx->getCurrentFunction()->addSymbol(id, var);
                dynamic_cast<Function*>(ctx->getCurrentFunction())->params.push_back(var);
            }

            // if (i == 0)
            // {
                
            //     ctx->dstRegStack.push(((var->type == "float") ? ctx->regManager->getRetVal("float") : ctx->regManager->getRetVal("int")));
            // }
            // else
            // {
            //     dst = ctx->regManager->getReg("a", var->type);
            //     if (dst != "-1")
            //     {
            //         ctx->dstRegStack.push(dst);
            //     }
            // }

            if (var->type == "float" && (ctx->regManager->registerStatus("fa0") == 0))
            {
                dst = ctx->regManager->getRetVal("float");
            }
            else if (var->type == "int" && (ctx->regManager->registerStatus("a0") == 0))
            {
                dst = ctx->regManager->getRetVal("int");
            }
            else
            {
                dst = ctx->regManager->getReg("a", var->type);

            }
            
            if (dst != "-1")
            {
                ctx->dstRegStack.push(dst);
            }

            if (dst != "-1")
            {
                if (var->type == "float")
                {
                    writeSwLw(ctx->out, "fsw", ctx->dstRegStack.top(), std::to_string(var->offset), "s0");
                }
                else
                {
                    writeSwLw(ctx->out, "sw", ctx->dstRegStack.top(), std::to_string(var->offset), "s0");
                }
                
                ctx->getCurrentFunction()->decrementOffset(typeSizes.at(type));
                
            }
            else
            {
                var->setOffset(offsetCounter);
                offsetCounter += typeSizes.at(type);
            }
        }
        while (!ctx->dstRegStack.empty())
        {
            ctx->regManager->freeReg(ctx->dstRegStack.top());
            ctx->dstRegStack.pop();
        }


        
    }

    virtual ~ParamListNode() {}
};



class TypeQualNode
    : public CommonNode
{
public:
    TypeQualNode(std::string &x)
    : qualifier(x)
    {}

    virtual void getText(std::ostream &ost)
    override
    {
        ost << qualifier;
    }

    std::string qualifier;
    virtual ~TypeQualNode() {}
};

class StatementNode
    : public CommonNode
{
public:
    StatementNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~StatementNode() {}
};

class CompoundStatementNode
    : public CommonNode
{
public:

    TerminalNode *tmp = NULL;
    CompoundStatementNode(std::string &open, CommonNodePtr a, std::string &close)
    {
        tmp = new TerminalNode(open);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(close);
        children.push_back(tmp);
    }

    CompoundStatementNode(std::string &open, std::string &close)
    {
        tmp = new TerminalNode(open);
        children.push_back(tmp);
        tmp = new TerminalNode(close);
        children.push_back(tmp);
    }

    CompoundStatementNode(std::string &open, CommonNodePtr a, CommonNodePtr b, std::string &close)
    {
        tmp = new TerminalNode(open);
        children.push_back(tmp);
        children.push_back(a);
        children.push_back(b);
        tmp = new TerminalNode(close);
        children.push_back(tmp);
    }

    // ugly solution for now
    virtual void compile(Context *ctx)
    override
    {  
        if (ctx->inFunction == 1) // if i just came from function, dont make a new scope
        {
            ctx->inFunction = 0;
            for (auto i : children)
            {
                i->compile(ctx);
            }
            return;
        }
        Scope *scope = new Scope();
        ctx->pushCurrentScope(scope);
        for (auto i : children)
        {
            i->compile(ctx);
        }
        ctx->popScope();
    }


    virtual ~CompoundStatementNode() {}
};


class ExpressionStatementNode
    : public CommonNode
{
public:
    ExpressionStatementNode(CommonNodePtr a, std::string &x)
    : semiColon(x)
    {
        children.push_back(a);
    }

    ExpressionStatementNode(std::string &x)
    : semiColon(x)
    {}

    virtual void getText(std::ostream &ost)
    override
    {
        for (auto i : children)
        {
            i->getText(ost);
        }
        ost << semiColon << std::endl;
    }

    std::string semiColon;
    virtual ~ExpressionStatementNode() {}
};






class ArrayIndexPostfixExpressionNode
    : public PostfixExpressionNode
{
public:
    ArrayIndexPostfixExpressionNode(CommonNodePtr a, std::string &x, CommonNodePtr b, std::string &y)
    : PostfixExpressionNode(a, x, b, y)
    {}

    virtual std::string getId()
    override
    {
        
        IdentifierNode *tmp = dynamic_cast<IdentifierNode*>(this->children[0]);
        if (tmp != NULL)
        {
            return tmp->getId();
        }
        else if (dynamic_cast<ArrayIndexPostfixExpressionNode*>(children[0]) != NULL)
        {
            return dynamic_cast<ArrayIndexPostfixExpressionNode*>(children[0])->getId();
        }
        else
        {
            return "";
        }
    }


    virtual void compile(Context *ctx)
    override
    {
        std::string id = this->getId();
        if (ctx->isGlobalSymbol(id))
        {

        }
        if (dynamic_cast<ArrayIndexPostfixExpressionNode*>(children[0]) != NULL)
        {
            children[0]->compile(ctx);
        }
        
        if (ctx->arrayCounter == 1)
        {
            writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), "0");
        }

        std::string c = ctx->regManager->getReg("t", "int");
        
        if (c == "-1")
        {
            c = ctx->regManager->getReg("a", "int");
        }

        ctx->dstRegStack.push(c);
        children[2]->compile(ctx);
        ctx->dstRegStack.pop();
        std::string tmp = ctx->regManager->getReg("t", "int");
        if (tmp == "-1")
        {
            tmp = ctx->regManager->getReg("a", "int");
        }
        Array *arr = dynamic_cast<Array*>(ctx->getSymbol(id));
        if (arr == NULL) {throw std::runtime_error("array not found");}
        writeTwoIns(ctx->out, "li", tmp, std::to_string(arr->mulValueCalulator(ctx->arrayCounter)));
        writeThreeIns(ctx->out, "mul", c, c, tmp);
        writeThreeIns(ctx->out, "add", ctx->dstRegStack.top(), ctx->dstRegStack.top(), c);
        ctx->regManager->freeReg(c);
        ctx->regManager->freeReg(tmp);

        if (ctx->arrayCounter == ((int)arr->sizes.size()))
        {
            writeTwoIns(ctx->out, "li", tmp, std::to_string(typeSizes.at(arr->type)));
            writeThreeIns(ctx->out, "mul", ctx->dstRegStack.top(), ctx->dstRegStack.top(), tmp);

            if (!ctx->isGlobalSymbol(id))
            {
                writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), ctx->dstRegStack.top(), std::to_string(std::abs(arr->offset)));
                writeThreeIns(ctx->out, "sub", ctx->dstRegStack.top(), "s0", ctx->dstRegStack.top());
            }
            else
            { 
                ctx->out << "lui " << tmp << ",\%hi" << "(" << id << ")" << std::endl;
                ctx->out << "addi " << tmp << ", " << tmp << ",\%lo" << "(" << id << ")" << std::endl;
                writeThreeIns(ctx->out, "add", ctx->dstRegStack.top(), ctx->dstRegStack.top(), tmp);
            }


            ctx->arrayCounter = 1;
            
            if (ctx->readWriteToAddress == 0)
            {
                writeSwLw(ctx->out, "lw", ctx->dstRegStack.top(), "0", ctx->dstRegStack.top());
            }
        }
        else
        {
            ctx->arrayCounter++;
        }
    }


    virtual ~ArrayIndexPostfixExpressionNode() {}
};


class IncDecPostfixExpressionNode
    : public PostfixExpressionNode
{
public:
    IncDecPostfixExpressionNode(CommonNodePtr a, std::string &x)
    : PostfixExpressionNode(a, x)
    {}
    
    std::string getOp()
    {
        TerminalNode *tmp = dynamic_cast<TerminalNode*>(children[1]);
        if (tmp == NULL) { throw "expecting ++ or --"; }
        return tmp->getTerminal();
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string Op = getOp();
        if (Op == "++")
        {
            ctx->postBool = POST_INC;
        }
        else if (Op == "--")
        {
            ctx->postBool = POST_DEC;
        }
        children[0]->compile(ctx);
    }



    virtual ~IncDecPostfixExpressionNode() {}
};


class PrimaryExprPostfixNode
    : public PostfixExpressionNode
{
public:
    PrimaryExprPostfixNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~PrimaryExprPostfixNode() {}
};



class UnaryExpressionNode
    : public CommonNode
{
public:
    UnaryExpressionNode(){}

    UnaryExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    UnaryExpressionNode(std::string &x, CommonNodePtr a)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
    }


    UnaryExpressionNode(std::string &x, std::string &y, CommonNodePtr b, std::string &z)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
    }

    UnaryExpressionNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    virtual std::string getId()
    {
        throw std::runtime_error("UnaryExpressionNode::getId() called, not implemented");
    }


    virtual void compile(Context *ctx)
    override
    {
        UnaryOperator *op = dynamic_cast<UnaryOperator*>(children[0]);
        if (op == NULL) {throw std::runtime_error("UnaryExpressionNode::compile(), could not find operator");}
        children[1]->compile(ctx);
        if (op->getTerminal() == "-")
        {
            ctx->out << "neg " << ctx->dstRegStack.top() << ", " << ctx->dstRegStack.top() << std::endl;
        }
        // else if (op->getTerminal() == "sizeof")
        // {
            // ctx->out << "li " << ctx->dstRegStack.top() << ", " << typeSizes.at(children[1]->getType()) << std::endl;
        // }
    }

    virtual ~UnaryExpressionNode() {}
};

class IncDecUnaryExpressionNode
    : public UnaryExpressionNode
{
public:
    IncDecUnaryExpressionNode(std::string &x, CommonNodePtr a)
    : UnaryExpressionNode(x, a)
    {}


    virtual std::string getId()
    {
        PostfixExpressionNode *tmp = dynamic_cast<PostfixExpressionNode*>(children[1]);
        if (tmp != NULL)
        {
            return tmp->getId();
        } 
        IdentifierNode *tmp2 = dynamic_cast<IdentifierNode*>(children[1]);
        if (tmp2 != NULL)
        {
            return tmp2->getId();
        }

        ConstantNode *tmp3 = dynamic_cast<ConstantNode*>(children[0]);
        if (tmp3 != NULL){throw std::runtime_error("Cannot assign to a constant");}
        StringLiteralNode *tmp4 = dynamic_cast<StringLiteralNode*>(children[0]);
        if (tmp4 != NULL){throw std::runtime_error("Cannot assign to a string literal");}
        return "";
    }

    virtual void compile(Context *ctx)
    override
    {
        
        TerminalNode *tmp = dynamic_cast<TerminalNode*>(children[0]);
        if (tmp == NULL) { return; }
        std::string op = tmp->getTerminal();
        if (op == "++")
        {
            ctx->unaryBool = INC;
        }
        else if (op == "--")
        {
            ctx->unaryBool = DEC;
        }
        children[1]->compile(ctx);
    }
    
    virtual ~IncDecUnaryExpressionNode() {}
};


class SizeOfUnaryExpressionNode
    : public UnaryExpressionNode
{
public:
    SizeOfUnaryExpressionNode(std::string &x, CommonNodePtr a)
    : UnaryExpressionNode(x, a)
    {}

    virtual void compile(Context *ctx)
    override
    {
        UnaryExpressionNode *tmp = dynamic_cast<UnaryExpressionNode*>(children[1]);
        CommonElement *elem = NULL;
        std::string id;
        if (tmp != NULL) 
        { 
            id = tmp->getId();
            elem = ctx->getSymbol(id);
            if (elem == NULL) { throw std::runtime_error("symbol not found"); }
            writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), std::to_string(typeSizes.at(elem->type)));
        }

        IdentifierNode *tmp2 = dynamic_cast<IdentifierNode*>(children[1]);
        if (tmp2 != NULL) 
        { 
            id = tmp2->getId();
            elem = ctx->getSymbol(id);
            if (elem == NULL) { throw std::runtime_error("symbol not found"); }
            writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), std::to_string(typeSizes.at(elem->type)));
        }

        PostfixExpressionNode *tmp3 = dynamic_cast<PostfixExpressionNode*>(children[1]);
        if (tmp3 != NULL) 
        { 
            id = tmp3->getId();
            elem = ctx->getSymbol(id);
            if (elem == NULL) { throw std::runtime_error("symbol not found"); }
            writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), std::to_string(typeSizes.at(elem->type)));
        }

        PrimaryExpressionNode *tmp4 = dynamic_cast<PrimaryExpressionNode*>(children[1]);
        if (tmp4 != NULL) 
        { 
            ExpressionNode *exp = dynamic_cast<ExpressionNode*>(tmp4->getChildren()[1]);
            if (exp == NULL) {throw std::runtime_error("no expression found");}
            ctx->sizeOfFlag = 1;
            exp->compile(ctx);
            ctx->sizeOfFlag = 0;            
        }
        else
        {
            throw std::runtime_error("SizeOfUnaryExpressionNode::compile() not implemented");
        }
    }


    virtual ~SizeOfUnaryExpressionNode() {}
};


class SizeOfTypeUnarExpressionNode
    : public UnaryExpressionNode
{
public:
    SizeOfTypeUnarExpressionNode(std::string &x, std::string &y, CommonNodePtr b, std::string &z)
    : UnaryExpressionNode(x, y, b, z)
    {}

    virtual void compile(Context *ctx)
    override
    {
        TypeNode *tmp = dynamic_cast<TypeNode*>(children[2]);
        if (tmp == NULL) { throw std::runtime_error("no type found");}
        writeTwoIns(ctx->out, "li", ctx->dstRegStack.top(), std::to_string(typeSizes.at(tmp->type)));
    }

    virtual ~SizeOfTypeUnarExpressionNode() {}

};

class AssignmentOperatorNode
    : public TerminalNode
{
public:
    AssignmentOperatorNode(std::string &op)
    : TerminalNode(op)
    {}

    std::string getOperator()
    {
        return getTerminal();
    }

    virtual ~AssignmentOperatorNode() {}
};



// x = y, x is the asignee (being assigned)
class AssigneeExpressionNode
    : public CommonNode
{
public: 
    AssigneeExpressionNode(CommonNodePtr a, CommonNodePtr b, CommonNodePtr c)
    {
        children.push_back(a);
        children.push_back(b);
        children.push_back(c);
    }

    virtual std::string pullId()
    {
        // is prim expr
        IdentifierNode *tmp = dynamic_cast<IdentifierNode*>(children[0]);
        if (tmp != NULL)
        {
            return tmp->getId();
        }
        ConstantNode *tmp2 = dynamic_cast<ConstantNode*>(children[0]);
        if (tmp2 != NULL){throw std::runtime_error("Cannot assign to a constant");}
        StringLiteralNode *tmp3 = dynamic_cast<StringLiteralNode*>(children[0]);
        if (tmp3 != NULL){throw std::runtime_error("Cannot assign to a string literal");}
        PrimaryExpressionNode *tmp4 = dynamic_cast<PrimaryExpressionNode*>(children[0]);
        if (tmp4 != NULL){throw std::runtime_error("Not sure if i can assign to u");}
        
        // is postfix expr
        PostfixExpressionNode *tmp5 = dynamic_cast<PostfixExpressionNode*>(children[0]);
        if (tmp5 != NULL)
        {
            return tmp5->getId();
        }
        // is unary expr
        UnaryExpressionNode *tmp6 = dynamic_cast<UnaryExpressionNode*>(children[0]);
        if (tmp6 != NULL)
        {
            return tmp6->getId();
        }

        throw std::runtime_error("Could not find id");
    }   

    virtual void compile(Context *ctx)
    override
    {
        std::string id = pullId();

        if (!ctx->isGlobalSymbol(id))
        {
            
            if (ctx->getLocalSymbol(id) == NULL)
            {
                throw std::runtime_error("Could not find id");
            }
            else if (dynamic_cast<Variable*>(ctx->getLocalSymbol(id)) != NULL)
            {
                ctx->readWriteToId = 1;
                children[0]->compile(ctx);
                ctx->readWriteToId = 0;
                // std::cout << " id is " << ctx->getTopOfScope()->getAssignmentFront()->id << std::endl;
                AssignerExpressionNode *assigner = dynamic_cast<AssignerExpressionNode*>(children[2]);
                if (assigner == NULL ) { std::cout << "assigning to " << id << "\t";  throw std::runtime_error("no assigner found");}
                if (assigner != NULL) 
                { 
                    ctx->dstRegStack.push(ctx->regManager->getReg("t", ctx->getTopOfScope()->getAssignmentFront()->type)); 
                }
                ctx->returnFunc = 1;
                children[2]->compile(ctx);
                ctx->returnFunc = 0;
                std::string dstReg = ctx->regManager->getReg("t", ctx->getTopOfScope()->getAssignmentFront()->type);
                AssignmentOperatorNode* opN = dynamic_cast<AssignmentOperatorNode*>(children[1]);
                if (opN == NULL) { return; }
                std::string op = opN->getOperator();
                if (op != "=")
                {
                    ctx->dstRegStack.push(dstReg);
                    children[0]->compile(ctx);
                    ctx->dstRegStack.pop();
                }
                if (ctx->getTopOfScope()->getAssignmentFront()->type == "float")
                {
                    writeAssign(ctx->out, dstReg, ctx->dstRegStack.top(), op, 1);
                }
                else
                {
                    writeAssign(ctx->out, dstReg, ctx->dstRegStack.top(), op);
                }
                if (dynamic_cast<Variable*>(ctx->getLocalSymbol(id))->type == "float")
                {
                  ctx->out << "fsw " << dstReg << ", " << ctx->getTopOfScope()->getAssignmentFront()->offset << "(s0)" << std::endl;  
                }
                else
                {
                  ctx->out << "sw " << dstReg << ", " << ctx->getTopOfScope()->getAssignmentFront()->offset << "(s0)" << std::endl;
                } 
                ctx->getTopOfScope()->assignmentQueue.pop();
                if (ctx->getTopOfScope()->assignmentQueue.empty())
                {
                    ctx->regManager->freeReg(ctx->dstRegStack.top());
                    ctx->dstRegStack.pop();
                }
                ctx->regManager->freeReg(dstReg);
            }
            else if (dynamic_cast<Array*>(ctx->getLocalSymbol(id))!=NULL)
            {
                std::string strOffsetReg = ctx->regManager->getReg("t", "int");
                ctx->dstRegStack.push(strOffsetReg);
                ctx->readWriteToAddress = 1;
                children[0]->compile(ctx);
                ctx->readWriteToAddress = 0;
                ctx->dstRegStack.pop();
                std::string valReg = ctx->regManager->getReg("t", ctx->getLocalSymbol(id)->type);
                ctx->dstRegStack.push(valReg);
                ctx->returnFunc = 1;
                children[2]->compile(ctx);
                ctx->returnFunc = 0;
                ctx->dstRegStack.pop();
                AssignmentOperatorNode* opN = dynamic_cast<AssignmentOperatorNode*>(children[1]);
                if (opN == NULL) { return; }
                std::string dst = ctx->regManager->getReg("t", (ctx->regManager->getRegType(valReg) == "f" ? "float" : "int"));
                std::string op = opN->getOperator();
                if (op != "=")
                {
                    ctx->dstRegStack.push(dst);
                    children[0]->compile(ctx);
                    ctx->dstRegStack.pop();
                }

                if (ctx->getLocalSymbol(id)->type == "float")
                {
                    writeAssign(ctx->out, dst, valReg, opN->getOperator(), 1);

                }
                else
                {
                    writeAssign(ctx->out, dst, valReg, opN->getOperator());
                }
                

                if (ctx->getLocalSymbol(id)->type == "float")
                {
                    writeSwLw(ctx->out, "fsw", valReg, "0", strOffsetReg);
                }
                else
                {
                    writeSwLw(ctx->out, "sw", valReg, "0", strOffsetReg);
                }
                writeTwoIns(ctx->out, "li", strOffsetReg, "0");
                
                ctx->regManager->freeReg(valReg);
                ctx->regManager->freeReg(strOffsetReg);
            }   
        }
        else
        {
            
            if (ctx->getGlobalSymbol(id) == NULL)
            {
                std::cout << " couldnt find globl symbol" << std::endl;
                throw std::runtime_error("Could not find id");
            }
            else if (dynamic_cast<Variable*>(ctx->getGlobalSymbol(id)) != NULL)
            {
                ctx->readWriteToId = 1;
                children[0]->compile(ctx);
                ctx->readWriteToId = 0;
                AssignerExpressionNode *assigner = dynamic_cast<AssignerExpressionNode*>(children[2]);
                if (assigner == NULL ) { std::cout << "assigning to " << id << "\t";  throw std::runtime_error("no assigner found");}
                std::string valReg = ctx->regManager->getReg("t", ctx->getAssignmentFront()->type);
                ctx->dstRegStack.push(valReg); 
                ctx->returnFunc = 1;
                children[2]->compile(ctx);
                ctx->returnFunc = 0;
                std::string dstReg = ctx->regManager->getReg("t", ctx->getAssignmentFront()->type);
                AssignmentOperatorNode* opN = dynamic_cast<AssignmentOperatorNode*>(children[1]);
                if (opN == NULL) { return; }
                std::string op = opN->getOperator();
                if (op != "=")
                {
                    ctx->dstRegStack.push(dstReg);
                    children[0]->compile(ctx);
                    ctx->dstRegStack.pop();
                }
                if (ctx->getAssignmentFront()->type == "float")
                {
                    writeAssign(ctx->out, dstReg, ctx->dstRegStack.top(), op, 1);
                }
                else
                {
                    writeAssign(ctx->out, dstReg, ctx->dstRegStack.top(), op);
                }
                ctx->regManager->freeReg(ctx->dstRegStack.top());
                ctx->dstRegStack.pop();
                // ctx->dstRegStack.pop();
                if (dynamic_cast<Variable*>(ctx->getGlobalSymbol(id))->type == "float")
                {
                    writeSwLw(ctx->out, "fsw", dstReg, "0", ctx->dstRegStack.top());
                }
                else
                {
                    writeSwLw(ctx->out, "sw", dstReg, "0", ctx->dstRegStack.top());
                }
                ctx->regManager->freeReg(ctx->dstRegStack.top());
                ctx->dstRegStack.pop();      
                ctx->assignmentQueue.pop();
                // if (ctx->assignmentQueue.empty())
                // {
                    // std::cout << " nothing on queue" << std::endl;
                // }
                ctx->regManager->freeReg(dstReg); 
                // if (ctx->dstRegStack.empty())
                // {
                    // std::cout << " success " << std::endl;
                // }
            }
            else if (dynamic_cast<Array*>(ctx->getGlobalSymbol(id))!=NULL)
            {
                std::string strOffsetReg = ctx->regManager->getReg("t", "int");
                ctx->dstRegStack.push(strOffsetReg);
                ctx->readWriteToAddress = 1;
                children[0]->compile(ctx);
                ctx->readWriteToAddress = 0;
                ctx->dstRegStack.pop();
                std::string valReg = ctx->regManager->getReg("t", ctx->getGlobalSymbol(id)->type);
                ctx->dstRegStack.push(valReg);
                ctx->returnFunc = 1;
                children[2]->compile(ctx);
                ctx->returnFunc = 0;
                ctx->dstRegStack.pop();
                AssignmentOperatorNode* opN = dynamic_cast<AssignmentOperatorNode*>(children[1]);
                if (opN == NULL) { return; }
                std::string dst = ctx->regManager->getReg("t", (ctx->regManager->getRegType(valReg) == "f" ? "float" : "int"));
                std::string op = opN->getOperator();
                if (op != "=")
                {
                    ctx->dstRegStack.push(dst);
                    children[0]->compile(ctx);
                    ctx->dstRegStack.pop();
                }

                if (ctx->getGlobalSymbol(id)->type == "float")
                {
                    writeAssign(ctx->out, dst, valReg, opN->getOperator(), 1);

                }
                else
                {
                    writeAssign(ctx->out, dst, valReg, opN->getOperator());
                }
                if (ctx->getGlobalSymbol(id)->type == "float")
                {
                    writeSwLw(ctx->out, "fsw", valReg, "0", strOffsetReg);
                }
                else
                {
                    writeSwLw(ctx->out, "sw", valReg, "0", strOffsetReg);
                }
                writeTwoIns(ctx->out, "li", strOffsetReg, "0");
                ctx->regManager->freeReg(valReg);
                ctx->regManager->freeReg(strOffsetReg);
            }
        }
    }
    virtual ~AssigneeExpressionNode() {}
};



class MultiplicativeExpressionNode
    : public GeneralExpressions
{
public:
    MultiplicativeExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    MultiplicativeExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~MultiplicativeExpressionNode() {}
};

class AdditiveExpressionNode
    : public GeneralExpressions
{
public:
    AdditiveExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    AdditiveExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~AdditiveExpressionNode() {}
};

class RelationalExpressionNode
    : public GeneralExpressions
{
public:
    RelationalExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    RelationalExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~RelationalExpressionNode() {}
};

class ShiftExpressionNode
    : public GeneralExpressions
{
public:
    ShiftExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    ShiftExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~ShiftExpressionNode() {}
};

class EqualityExpressionNode
    : public GeneralExpressions
{
public:
    EqualityExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    EqualityExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~EqualityExpressionNode() {}
};

class AndExpressionNode
    : public GeneralExpressions
{
public:
    AndExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    AndExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~AndExpressionNode() {}
};

class ExclusiveOrExpressionNode
    : public GeneralExpressions
{
public:
    ExclusiveOrExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    ExclusiveOrExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~ExclusiveOrExpressionNode() {}
};

class InclusiveOrExpressionNode
    : public GeneralExpressions
{
public:
    InclusiveOrExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    InclusiveOrExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~InclusiveOrExpressionNode() {}
};

class LogicalAndExpressionNode
    : public GeneralExpressions
{
public:
    LogicalAndExpressionNode (CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    LogicalAndExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~LogicalAndExpressionNode() {}
};

class LogicalOrExpressionNode
    : public GeneralExpressions
{
public:
    LogicalOrExpressionNode(CommonNodePtr a)
    : GeneralExpressions(a)
    {}

    LogicalOrExpressionNode(CommonNodePtr a, std::string &op, CommonNodePtr b)
    : GeneralExpressions(a, op, b)
    {}
    virtual ~LogicalOrExpressionNode() {}
};


class TernaryExpressionNode // needs seperate compile function
    : public GeneralExpressions
{
public:
    TernaryExpressionNode (CommonNodePtr a)
    {
        children.push_back(a);
    } 

    TernaryExpressionNode(CommonNodePtr a, std::string &op1, CommonNodePtr b, std::string &op2, CommonNodePtr c)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(op1);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(op2);
        children.push_back(tmp);
        children.push_back(c);
    }

    virtual int getValue()
    override
    {
        GeneralExpressions *tmpC1 = dynamic_cast<GeneralExpressions*>(children[0]);
        ConstantNode *tmpC2 = dynamic_cast<ConstantNode*>(children[0]);
        int c;
        if (tmpC1 != NULL)
        {
            c = tmpC1->getValue();
        }
        else if (tmpC2 != NULL)
        {
            c = tmpC2->getValue();
        }

        if (c == 0)
        {
            GeneralExpressions *tmpT1 = dynamic_cast<GeneralExpressions*>(children[4]);
            ConstantNode *tmpT2 = dynamic_cast<ConstantNode*>(children[4]);
            if (tmpT1 != NULL)
            {
                return tmpT1->getValue();
            }
            else if (tmpT2 != NULL)
            {
                return tmpT2->getValue();
            }
        }
        else
        {
            ExpressionNode *tmpE = dynamic_cast<ExpressionNode*>(children[2]);
            if (tmpE != NULL)
            {
                return tmpE->getValue();
            }
        }

        return -1;
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string dst = ctx->regManager->getReg("a", "int");
        ctx->dstRegStack.push(dst);
        children[0]->compile(ctx);
        ctx->regManager->freeReg(dst);
        ctx->dstRegStack.pop();
        std::string endLabel = ctx->makeUnique("ENDIF");
        std::string elseLabel = ctx->makeUnique("ELSE");
        writeThreeIns(ctx->out, "beq", dst, "zero", elseLabel);
        children[2]->compile(ctx);
        writeOneIns(ctx->out, "j", endLabel);
        ctx->out << elseLabel << ":" << std::endl;
        children[4]->compile(ctx);
        ctx->out << endLabel << ":" << std::endl;
    }   
    
    virtual ~TernaryExpressionNode() {}
};





class ReturnNode
    : public CommonNode
{
public:
    ReturnNode(std::string &x, std::string &semi)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(semi);
        children.push_back(tmp);
    }

    ReturnNode(std::string &x, CommonNodePtr a, std::string &semi)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(semi);
        children.push_back(tmp);
    }

    virtual void compile(Context *ctx)
    override
    {
        // ctx->printScopeSymbolTables(); // debug function
        if (children.size() == 3)
        {

            // std::string dst = ctx->regManager->getReg("a", ctx->getCurrentFunction()->type);
            std::string dst = ctx->regManager->getReg("t", ctx->getCurrentFunction()->type);
            ctx->dstRegStack.push(dst);
            ctx->returnArr = true;
            ctx->returnFunc = true;
            children[1]->compile(ctx);
            ctx->regManager->freeReg(dst);
            if (ctx->getCurrentFunction()->type == "float")
            {
                ctx->out << "fmv.s fa0, " << ctx->dstRegStack.top() << std::endl;
            }
            else
            {
                ctx->out << "mv a0, " << ctx->dstRegStack.top() << std::endl;
            }
            ctx->dstRegStack.pop();
            // ctx->regManager->freeReg(ctx->dstRegStack.top());
            // ctx->dstRegStack.pop();
            ctx->out << "j " << ctx->returnLabel << std::endl;
        }
    }

    virtual ~ReturnNode() {} 
};


class DeclarationListNode
    : public CommonNode
{
public:
    DeclarationListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }
    virtual ~DeclarationListNode() {}
};



class BreakContinueNode
    : public CommonNode
{
public:
    BreakContinueNode(std::string &x, std::string &semi)
    {
        this->jump = x;
        this->semi = semi;
    }

    virtual void getText(std::ostream &ost)
    override
    {
        ost << jump << semi << std::endl;

    }

    virtual void compile(Context *ctx)
    override
    {
        if (jump == "break")
        {
            ctx->out << "j " << ctx->breakStack.top() << std::endl;
        }
        else if (jump == "continue")
        {
            ctx->out << "j " << ctx->continueStack.top() << std::endl;
        }
    }

    std::string semi;
    std::string jump;
};


class CastExpressionNode
    : public CommonNode
{
public:
    CastExpressionNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    CastExpressionNode(std::string &x, CommonNodePtr a, std::string &y, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(b);
    }
    virtual ~CastExpressionNode() {}
};






class IfNoElseNode
    : public CommonNode
{
public:
    IfNoElseNode(std::string &x, std::string &y, CommonNodePtr a, std::string &z, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(b);
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string dst = ctx->regManager->getReg("a", "int");
        ctx->dstRegStack.push(dst);
        children[2]->compile(ctx);
        ctx->regManager->freeReg(dst);
        ctx->dstRegStack.pop();
        std::string skipLabel = ctx->makeUnique("skip");
        writeThreeIns(ctx->out, "beq", dst, "zero", skipLabel);
        children[4]->compile(ctx);
        ctx->out << skipLabel << ":" << std::endl;
    }

    virtual ~IfNoElseNode() {}
};

class IfElseNode
    : public CommonNode
{
public:
    IfElseNode(std::string &x, std::string &y, CommonNodePtr a, std::string &z, CommonNodePtr b, std::string &w, CommonNodePtr c)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(b);
        tmp = new TerminalNode(w);
        children.push_back(tmp);
        children.push_back(c);
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string dst = ctx->regManager->getReg("a", "int");
        ctx->dstRegStack.push(dst);
        children[2]->compile(ctx);
        ctx->regManager->freeReg(dst);
        ctx->dstRegStack.pop();
        std::string endLabel = ctx->makeUnique("ENDIF");
        std::string elseLabel = ctx->makeUnique("ELSE");
        writeThreeIns(ctx->out, "beq", dst, "zero", elseLabel);
        children[4]->compile(ctx);
        writeOneIns(ctx->out, "j", endLabel);
        ctx->out << elseLabel << ":" << std::endl;
        children[6]->compile(ctx);
        ctx->out << endLabel << ":" << std::endl;
    }   
    

    virtual ~IfElseNode() {}

};


class WhileNode
    : public CommonNode
{
public:
    WhileNode(std::string &x, std::string &y, CommonNodePtr a, std::string &z, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(b);
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string dst = ctx->regManager->getReg("a", "int");
        ctx->dstRegStack.push(dst);
        std::string beginLabel = ctx->makeUnique("WHILE");
        std::string endLabel = ctx->makeUnique("ENDWHILE");
        ctx->breakStack.push(endLabel);
        ctx->continueStack.push(beginLabel);
        ctx->out << beginLabel << ":" << std::endl;
        children[2]->compile(ctx);
        ctx->regManager->freeReg(dst);
        ctx->dstRegStack.pop();
        writeThreeIns(ctx->out, "beq", dst, "zero", endLabel);
        children[4]->compile(ctx);
        writeOneIns(ctx->out, "j", beginLabel);
        ctx->out << endLabel << ":" << std::endl;
        ctx->breakStack.pop();
        ctx->continueStack.pop();
    }

    virtual ~WhileNode() {}
};

class LabelStatementNode
    : public CommonNode
{
public:

    LabelStatementNode(){}

    LabelStatementNode(std::string &x, std::string &y)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }    

    LabelStatementNode(std::string x, CommonNodePtr a, std::string y, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(b);
    }

    LabelStatementNode(std::string def, CommonNodePtr a)
    {
        TerminalNode *tmp = new TerminalNode(def);
        children.push_back(tmp);
        children.push_back(a);
    }


    virtual void compile(Context *ctx)
    override
    {
        if (ctx->switchFlag == 1)
        {
            std::string label = ctx->makeUnique(".LABEL");
            ctx->switchLabels.push(label);
            std::string checkReg = ctx->dstRegStack.top();
            caseElement->compile(ctx);
            int t = dynamic_cast<ConstantExpressionNode*>(children[1])->getValue();
            ctx->out << "li " << ctx->caseReg << ", " << t << std::endl;
            ctx->out << "beq " << checkReg << ", " << ctx->caseReg << ", " << label << std::endl;
        }
        else
        {
            ctx->out << ctx->switchLabels.front() << ":" << std::endl;
            ctx->switchLabels.pop();
            children[3]->compile(ctx);
        }

    }
};

class DefualtStatementNode
    : public LabelStatementNode
{
public:
    DefualtStatementNode(std::string &x, CommonNodePtr a)
    : LabelStatementNode(x, a)
    {}

    virtual void compile(Context *ctx)
    override
    {
        if (ctx->switchFlag == 1)
        {
            std::string def = ctx->makeUnique(".DEFAULT");
            ctx->switchLabels.push(def);

            ctx->out << "j " << def << std::endl;
            
        }
        else
        {
            ctx->out << ctx->switchLabels.front() << ":" << std::endl;
            children[1]->compile(ctx);
            ctx->switchLabels.pop();
        }

    }

    virtual ~DefualtStatementNode() {}
};


class SwitchNode
    : public CommonNode
{
public:
    SwitchNode(std::string &x, std::string &y, CommonNodePtr a, std::string &z, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(b);
    }

    virtual void compile(Context *ctx)
    override
    {
        std::string endSwitch = ctx->makeUnique("ENDSWITCH");
        ctx->breakStack.push(endSwitch);
        ctx->continueStack.push(endSwitch);
        std::string dst = ctx->regManager->getReg("a", "int");
        ctx->dstRegStack.push(dst);
        caseElement = children[2];
        std::string caseReg = ctx->regManager->getReg("t", "int");
        ctx->caseReg = caseReg;
        ctx->switchFlag = 1;
        children[4]->compile(ctx);
        ctx->out << "j " << endSwitch << std::endl;
        ctx->switchFlag = 0;
        children[4]->compile(ctx);
        ctx->out << endSwitch << ":" << std::endl;
        ctx->caseReg = "";
        ctx->regManager->freeReg(dst);
        ctx->dstRegStack.pop();
        ctx->regManager->freeReg(caseReg);
        caseElement = nullptr;
    }

};


class StatementListNode
    : public CommonNode
{
public:
    StatementListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void compile(Context *ctx)
    override
    {
        if (ctx->switchFlag == 0)
        {
            for (auto i : children)
            {
                i->compile(ctx);
            }
        }
        else
        {
            for (auto i : children)
            {
                if (dynamic_cast<LabelStatementNode*>(i) != NULL)
                {
                    i->compile(ctx);
                }
            }
        }

    }

    virtual ~StatementListNode() {}
};


class ForTwoStatementNode
    : public CommonNode
{
public:

    ForTwoStatementNode(std::string &x, std::string &y, CommonNodePtr a, CommonNodePtr b, std::string &z, CommonNodePtr c)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        children.push_back(b);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(c);
    }

    
    virtual void compile(Context *ctx)
    override
    {
        std::string endFor = ctx->makeUnique("ENDFOR");
        writeOneIns(ctx->out, "j", endFor);
        std::string beginFor = ctx->makeUnique("FOR");
        ctx->out << beginFor << " :" << std::endl;
        children[children.size()-1]->compile(ctx);
        children[4]->compile(ctx);
        ctx->out << endFor << " :" << std::endl;
        std::string dst = ctx->regManager->getReg("a", "int"); // need check if float or int, seems like the dst reg is int
        ctx->dstRegStack.push(dst);
        children[3]->compile(ctx);
        ctx->regManager->freeReg(dst);
        writeThreeIns(ctx->out, "bne", dst, "zero", beginFor);
    }



    virtual ~ForTwoStatementNode() {}
};


class ForThreeStatementNode
    : public CommonNode
{
public:

    ForThreeStatementNode(std::string &x, std::string &y, CommonNodePtr a, CommonNodePtr b, CommonNodePtr c, std::string &z, CommonNodePtr d)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
        children.push_back(b);
        children.push_back(c);
        tmp = new TerminalNode(z);
        children.push_back(tmp);
        children.push_back(d);
    }
    
    virtual void compile(Context *ctx)
    override
    {
        children[2]->compile(ctx);
        std::string endFor = ctx->makeUnique("ENDFOR");
        writeOneIns(ctx->out, "j", endFor);
        std::string beginFor = ctx->makeUnique("FOR");
        ctx->out << beginFor << " :" << std::endl;
        children[children.size()-1]->compile(ctx);
        children[4]->compile(ctx);
        ctx->out << endFor << " :" << std::endl;
        std::string dst = ctx->regManager->getReg("a", "int"); // need check if float or int, seems like the dst reg is int
        ctx->dstRegStack.push(dst);
        children[3]->compile(ctx);
        ctx->regManager->freeReg(dst);
        writeThreeIns(ctx->out, "bne", dst, "zero", beginFor);
    }



    virtual ~ForThreeStatementNode() {}

};
class StructTypeNode
    : public CommonNode
{
public:
    StructTypeNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    ~StructTypeNode() {}
};

class EnumTypeNode
    : public CommonNode
{
public:
    EnumTypeNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    ~EnumTypeNode() {}
};


class TranslationUnitNode
    : public CommonNode
{
public:

    TranslationUnitNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    TranslationUnitNode(CommonNodePtr a)
    {
        children.push_back(a);
    }


    virtual ~TranslationUnitNode() {}
};


class ProgramNode
    : public CommonNode
{
public:
    ProgramNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void getText(std::ostream &ost)
    override
    {
        getChildren()[0]->getText(ost);
    }

    virtual ~ProgramNode() {}
};

class ExternalDeclarationNode
    : public CommonNode
{
public:
    ExternalDeclarationNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual void getText(std::ostream &ost)
    override
    {
        getChildren()[0]->getText(ost);
    }

    virtual ~ExternalDeclarationNode() {}
};



class StructSpecifierNode
    : public CommonNode
{
public:
    StructSpecifierNode(CommonNodePtr a, std::string &b, CommonNodePtr c)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(b);
        children.push_back(tmp);
        children.push_back(c);
    }


    StructSpecifierNode(CommonNodePtr a, std::string &b)
    {
        children.push_back(a);
        TerminalNode *tmp = new TerminalNode(b);
        children.push_back(tmp);
    }

    StructSpecifierNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    virtual void getText(std::ostream &ost)
    override
    {
        getChildren()[0]->getText(ost);
    }

    virtual ~StructSpecifierNode() {}
};


class StructTerminal
    : public CommonNode
{
public:
    StructTerminal(std::string &x)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    virtual void getText(std::ostream &ost)
    override
    {
        ost << "struct";
    }

    virtual ~StructTerminal() {}
};

class StructDeclarationListNode
    : public CommonNode
{
public:
    StructDeclarationListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }


    virtual ~StructDeclarationListNode() {}
};


class StructDeclarationNode
    : public CommonNode
{
public:
    StructDeclarationNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    virtual ~StructDeclarationNode() {}
};


class StructTypeSpecListNode
    : public CommonNode
{
public:
    StructTypeSpecListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~StructTypeSpecListNode() {}
};
class StructDeclaratorNode
    : public CommonNode
{
public:
    StructDeclaratorNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    StructDeclaratorNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    virtual ~StructDeclaratorNode() {}
};

class StructDeclaratorListNode
    : public CommonNode
{
public:
    StructDeclaratorListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~StructDeclaratorListNode() {}
};


class EnumSpecifierNode
    : public CommonNode
{
public:

    EnumSpecifierNode(std::string &x, CommonNodePtr a)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
    }

    EnumSpecifierNode(std::string &x, std::string &y)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    EnumSpecifierNode(std::string &x, std::string &y, CommonNodePtr a)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(a);
    }

    EnumSpecifierNode(std::string &x)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    void virtual compile(Context *ctx)
    override
    {
        std::string id;
        for (auto &child : children)
        {
            if (dynamic_cast<IdentifierNode*>(child) != NULL)
            {
                id = dynamic_cast<IdentifierNode*>(child)->getId();
            }
        }
        Enum *en = new Enum(id);
        ctx->addGlobalSymbol(id, en);
        ctx->currentEnum = en;
        children[1]->compile(ctx);
        ctx->currentEnum = NULL;
    }

    virtual ~EnumSpecifierNode() {}
};


class EnumListNode
    : public CommonNode
{
public:
    
    EnumListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    virtual ~EnumListNode() {}
};



class EnumaratorNode
    : public CommonNode
{
public:
    EnumaratorNode(std::string &x, CommonNodePtr a)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
    }

    virtual void compile(Context *ctx)
    override
    {
        if (ctx->currentEnum != NULL)
        {
            ConstantExpressionNode *tmp = dynamic_cast<ConstantExpressionNode*>(children[1]);
            if (tmp == NULL ){throw std::runtime_error("Error: expecting value");}
            ctx->currentEnum->addEnumeratorVal(dynamic_cast<TerminalNode*>(children[0])->getTerminal(), tmp->getValue());
            ctx->enumValues[dynamic_cast<TerminalNode*>(children[0])->getTerminal()] = tmp->getValue();
            // std::cout << " added " <<  ctx->enumValues[dynamic_cast<TerminalNode*>(children[0])->getTerminal()] << " with value" << tmp->getValue() << std::endl;
        }
        else
        {
            std::cout << "Error: enumarator outside of enum" << std::endl;
        }
    }
};

class EnumIdNode
    : public CommonNode
{
public:
    EnumIdNode(std::string &x)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    virtual void compile(Context *ctx)
    override
    {
        if (ctx->currentEnum != NULL)
        {
            ctx->currentEnum->addEnumerator(dynamic_cast<TerminalNode*>(children[0])->getTerminal());
            ctx->enumValues[dynamic_cast<TerminalNode*>(children[0])->getTerminal()] = ctx->currentEnum->getEnumeratorVal(dynamic_cast<TerminalNode*>(children[0])->getTerminal());
        }
        else
        {
            std::cout << "Error: enumarator outside of enum" << std::endl;
        }
    }

    virtual ~EnumIdNode() {}
};


class PointerNode
    : public CommonNode
{
public:
    PointerNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    PointerNode(std::string &x, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
    }
    
    PointerNode(std::string &x, CommonNodePtr b, CommonNodePtr c)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(b);
        children.push_back(c);
    }



    virtual ~PointerNode() {}
};


class IdListNode
    : public CommonNode
{
public:
    IdListNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    IdListNode(std::string &x)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    virtual void addChildId(std::string &x)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
    }

    virtual ~IdListNode() {}
};


class AbstractDeclaratorNode
    : public CommonNode
{
public:
    AbstractDeclaratorNode(CommonNodePtr a)
    {
        children.push_back(a);
    }

    AbstractDeclaratorNode(CommonNodePtr a, CommonNodePtr b)
    {
        children.push_back(a);
        children.push_back(b);
    }

    AbstractDeclaratorNode(CommonNodePtr a, CommonNodePtr b, CommonNodePtr c)
    {
        children.push_back(a);
        children.push_back(b);
        children.push_back(c);
    }

    virtual ~AbstractDeclaratorNode() {}
};

class AbstractDirDeclaratorNode
    : public DirDeclaratorNode
{
public:
    AbstractDirDeclaratorNode(std::string &x, std::string &y)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    AbstractDirDeclaratorNode(std::string &x, CommonNodePtr a, std::string &y)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
    }

    AbstractDirDeclaratorNode(std::string &x, CommonNodePtr a, std::string &y, CommonNodePtr b)
    {
        TerminalNode *tmp = new TerminalNode(x);
        children.push_back(tmp);
        children.push_back(a);
        tmp = new TerminalNode(y);
        children.push_back(tmp);
        children.push_back(b);
    }


    virtual ~AbstractDirDeclaratorNode() {}
};