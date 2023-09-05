#pragma once
#include <iostream>
#include <vector>
#include <cstdarg>
#include <map>
#include <string>
#include <stack>
#include <bitset>
#include <queue>
#include <functional>
#include <numeric>
#include  <ast.h>
#include <sstream>

struct CommonElement;
typedef CommonElement* ElementPtr;

typedef std::map<std::string, ElementPtr> SymbolTable;
typedef std::map<std::string, std::bitset<2>> RegisterMap; // [0, 0] bit 0 -> intregs, bit 1 -> floatregs

enum UnaryOp // identifies wehter to decrease or icnrease the upcoming assigner on the fly
{
    INC,
    DEC,
    NONE
};

enum PostOp
{
    POST_INC,
    POST_DEC,
    NONE_POST
};

struct CommonElement
{
    CommonElement(){}

    CommonElement(std::string _id)
    : id(_id)
    {}

    CommonElement(std::string _id, std::string _type)
    : id(_id)
    , type(_type)
    {}

    CommonElement(std::string _id, std::string _type, int _offset)
    : id(_id)
    , type(_type)
    , offset(_offset)
    {}


    void setOffset(int _offset){
        this->offset = _offset;
    }

    virtual std::string getId() { return this->id; }

    ~CommonElement(){}

    std::string id;
    std::string type;
    int offset;
    bool isGlobal = false;
};

struct Variable 
    : public CommonElement // offset here is relative to fp
{
    Variable(){}

    Variable(std::string _id)
    :CommonElement(_id)
    {}

    Variable(std::string _id, std::string _type)
    :CommonElement(_id, _type)
    {}

    Variable(std::string _id, std::string _type, int _offset)
    :CommonElement(_id, _type, _offset)
    {}

    void setOffset(int _offset){
        this->offset = _offset; // relative to fp
    }

    ~Variable(){}
};


struct Array 
    : public CommonElement
{
    Array(){}

    Array(std::string _id)
    :CommonElement(_id)
    {}

    Array(std::string _id, std::string _type)
    :CommonElement(_id, _type)
    {}

    Array(std::string _id, std::string _type, int _offset)
    :CommonElement(_id, _type, _offset)
    {}

    Array(std::string _id, std::string _type, int _offset, std::vector<int> _sizes)
    :CommonElement(_id, _type, _offset)
    {
        this->sizes = _sizes;
        this->assignerOffset = _offset;
    }

    int getSize()
    {
        return std::accumulate(this->sizes.begin(), this->sizes.end(), 1, std::multiplies<int>());
    }

    void decrementAssigner(int size)
    {
        this->assignerOffset -= size;
    }

    void setOffset(int _offset){
        this->offset = _offset; // relative to fp
    }


    int mulValueCalulator(int index)
    {
        int mulValue = 1;
        if (index >= (int)this->sizes.size())
        {
            return mulValue;
        }

        for(int i = index; i < (int)this->sizes.size(); i++)
        {
            mulValue *= this->sizes[i];
        }
        return mulValue;
    }


    ~Array(){}

    int assignerOffset;
    std::vector<int> sizes; // multiple sizes for multidimensional arrays
};

// synonymous with Scope, can only be inside a new scope if im in a function

struct Scope
    : public CommonElement 
{
    Scope(){}

    Scope(std::string _id, std::string _type)
    :CommonElement(_id, _type, -20) // offset here is relative to fp, var storage starts at -20
    {}

    // function/ scope stuff
    void decrementOffset(int size) // added a var, so decrement offset for next var
    {
        this->offset -= size;
    }

    void addSymbol(std::string id, ElementPtr element)
    {
        this->symbolTable[id] = element;
    }
    // end of function/ scope stuff
    
    // asignment queue stuff
    void pushAssignmentQueue(CommonElement* element)
    {
        this->assignmentQueue.push(element);
    }

    CommonElement* getAssignmentFront()
    {
        return this->assignmentQueue.front();
    }

    void popAssignmentQueue()
    {
        this->assignmentQueue.pop();
    }
    // end of assignment queue stuff

    void setQueue(std::queue<CommonElement*> _queue)
    {
        this->assignmentQueue = _queue;
    }

    // symbol table for each scope
    SymbolTable symbolTable;

    // uninitialized variables queue
    std::queue<CommonElement*> assignmentQueue;
    
    ~Scope(){}
};


struct Function
    : public Scope
{

    Function(std::string _id, std::string _type)
    :Scope(_id, _type) // offset here is relative to fp, var storage starts at -20
    {}

    
    std::vector<CommonElement*> params;
};

struct RegisterAllocator
{
    RegisterAllocator(){}
    
    RegisterMap TmpRegs =
    {
        {"t0", 0},
        {"t1", 0},
        {"t2", 0},
        {"t3", 0},
        {"t4", 0},
        {"t5", 0},
        {"t6", 0},
        // {"t7", 0},
        // {"t8", 0},
        // {"t9", 0},
        // {"t10", 0},
        // {"t11", 0}
    };

    RegisterMap Aregs =
    {
        {"a1", 0},
        {"a2", 0},
        {"a3", 0},
        {"a4", 0},
        {"a5", 0},
        {"a6", 0},
        {"a7", 0},
    };

    RegisterMap Sregs =
    {
        {"s1", 0},
        {"s2", 0},
        {"s3", 0},
        {"s4", 0},
        {"s5", 0},
        {"s6", 0},
        {"s7", 0},
        {"s8", 0},
        {"s9", 0},
        {"s10", 0},
        {"s11", 0},
    };
    

    std::string getSp() { return "sp"; } // stack pointer
    std::string getFp() { return "s0"; } // frame pointer

    std::string getZero() { return "zero"; } // zero register
    std::string getRa() { return "ra"; } // return address

    // might have to chagne for arrays, since floats in arrays go to int registers
    std::string regTypeSetter(std::string varType)
    {
        if (varType == "float" || varType == "double")
        {
            return "f";
        }
        return "i";
    }



    RegisterMap *setMap(std::string regtype)
    {
        if (regtype == "t")
        {
            return &TmpRegs;
        }else if (regtype == "a")
        {
            return &Aregs;
        }else if (regtype == "s")
        {
            return &Sregs;
        }
        return {};
    }
    
    void setUsed(std::string x)
    {
        int mapType = 0;
        if (x[0] == 'f')
        {
            x = x.substr(1);
            mapType = 1;
        }

        RegisterMap *rmap = setMap(x.substr(0,1));
        for (auto &i : *rmap)
        {
            if (i.first == x)
            {
                i.second[mapType] = 1;
                return;
            }
        }
    }

    int setType(std::string x)
    {
        if (x == "i")
        {
            return 0;
        }else if (x == "f")
        {
            return 1;
        }
        return -1;
    }

    std::string getReg( std::string regtype, std::string x )
    {
        RegisterMap *rmap = setMap(regtype);
        int maptype = setType(regTypeSetter(x));
        
        for (auto &i : *rmap)
        {   
            if ((rmap == &Sregs && i.first == "s10") || (rmap == &Sregs && i.first == "s11"))
            {
                continue;
            }
            if (i.second[maptype] == 0)
            {
                i.second[maptype] = 1;

                return (maptype) ? "f" + i.first : i.first; // fixed
            }
        }

        if (regtype == "s")
        {
            if ((*rmap)["s10"][maptype] == 0)
            {
              return (maptype) ? "fs10" : "s10";                   
            }
            
            if ((*rmap)["s11"][maptype] == 0)
            {
              return (maptype) ? "fs11" : "s11";                   
            }

        }
        return "-1";
    }

    std::string getRetVal(std::string type) 
    { 
        if (type == "float" || type == "double")
        {   
            this->returnReg[1] = 1;
            return "fa0";
        }
        this->returnReg[0] = 1;   
        return "a0"; 
    } 

    void freeReg(std::string x)
    {   
        if (x == "a0")
        {
            this->returnReg[0] = 0;
            return;
        }
        else if (x == "fa0")
        {
            this->returnReg[1] = 0;
            return;
        }

        if (x == "zero" || x == "sp" || x == "s0" || x == "ra")
        {
            std::cout << "cannot free register: " << x << " because it is a reserved register" << std::endl;
            return;
        }

        int mapType = 0;
        if (x[0] == 'f')
        {
            x = x.substr(1);
            mapType = 1;
        }

        RegisterMap *rmap = setMap(x.substr(0,1));
        for (auto &i : *rmap)
        {
            if (i.first == x)
            {
                i.second[mapType] = 0;
                return;
            }
        }
    }


    bool registerStatus(std::string x)
    {
        // if (x == "zero" || x == "sp" || x == "s0" || x == "ra" || x == "a0")
        // {
        //     return true;
        // }
        if (x == "a0")
        {
            return this->returnReg[0];
        }
        else if (x == "fa0")
        {
            return this->returnReg[1];
        }

        int mapType = 0;
        if (x[0] == 'f')
        {
            x = x.substr(1);
            mapType = 1;
        }

        RegisterMap *rmap = setMap(x.substr(0,1));
        return rmap->at(x)[mapType];
    }

    std::string getRegType(std::string x)
    {
        if (x[0] == 'f')
        {
            return "f";
        }
        return "i";
    }

    // DEBUGGING

    // print all register status
    void printAllRegs()
    {
        for (auto &i : TmpRegs)
        {
            std::cout << i.first << " " << i.second[0] << " " << i.second[1] << std::endl;
        }
        for (auto &i : Aregs)
        {
            std::cout << i.first << " " << i.second[0] << " " << i.second[1] << std::endl;
        }
        for (auto &i : Sregs)
        {
            std::cout << i.first << " " << i.second[0] << " " << i.second[1] << std::endl;
        }
    }


    std::bitset<2> returnReg;

    ~RegisterAllocator(){}
};


class Enum 
    : public CommonElement
{
public:
    Enum(std::string _id)
    : CommonElement(_id)
    {}

    
    bool isValue(int x)
    {
        for (auto i: enumerators)
        {
            if (i.second == x)
            {
                return true;
            }
        }
        return false;
    }
    

    void addEnumeratorVal(std::string id, int value)
    {
        enumerators[id] = value;
    }

    void addEnumerator(std::string id)
    {
        
        while(isValue(counter))
        {
            counter++;
        }

        enumerators[id] = counter;
        counter++;
    }

    int getEnumeratorVal(std::string id)
    {
        return this->enumerators.at(id);
    }

    ~Enum(){}

    int counter = 0;
    std::map<std::string, int> enumerators;
};




struct Context{

    Context(std::ostream &_out)
    :out(_out)
    {}

    void setFunctionSizes(std::map<std::string, int> _FunctionSizes)
    {
        this->functionSizes = _FunctionSizes;
    }

    bool isFunction(std::string id)
    {
        try
        {
            ElementPtr t = this->globalSymbolTable.at(id);
            return (dynamic_cast<Scope*>(t) != NULL);
        }
        catch(const std::exception& e)
        {
            return false;
        }
        
    }

    void pushCurrentScope(Scope *t) // adds function to stack (entering new Scope)
    {
        this->scopeStack.push(t);
    }

    void addFunction(Scope *t)
    {
        globalSymbolTable[t->getId()] = t;
    }

    void popScope() // removes function from stack (leaving Scope)
    {
        this->scopeStack.pop();
    }

    int unq = 0; //  for unq labels
    std::string makeUnique(std::string str)
    {
        return str + "_" + std::to_string(unq++);
    }

    void setFunctionCallTypes(std::map<std::string, std::bitset<2>> _callType)
    {
        this->functionCallType = _callType;
    }

    int getFunctionSize(std::string id)
    {
        return this->functionSizes.at(id);
    }

    bool isCaller(std::string id)
    {
        return this->functionCallType.at(id)[0];
    }

    Scope* getCurrentFunction()
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            if (dynamic_cast<Function*>(i.top()) != nullptr)
            {
                return i.top();
            }
        }
        return nullptr;
    }

    bool isLocalSymbol(std::string id)
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            if (i.top()->symbolTable.find(id) != i.top()->symbolTable.end())
            {
                return 1;
            }
        }
        return 0;
    }

    bool isGlobalSymbol(std::string id)
    {
        bool ret = (this->globalSymbolTable.find(id) != this->globalSymbolTable.end());
        return ret && (dynamic_cast<Scope*>(globalSymbolTable.at(id)) == nullptr);
    }
    
    bool isGlobalScope() { return this->scopeStack.empty(); };

    CommonElement* getLocalSymbol(std::string id)
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            if (i.top()->symbolTable.find(id) != i.top()->symbolTable.end())
            {
                return i.top()->symbolTable.at(id);
            }
        } 
        return nullptr;       
    }

    CommonElement* getSymbol(std::string id) // returns the symbol from the closest scope
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            if (i.top()->symbolTable.find(id) != i.top()->symbolTable.end())
            {
                return i.top()->symbolTable.at(id);
            }
        }
        try 
        {
            return this->globalSymbolTable.at(id);
        }
        catch(const std::exception& e)
        {
            return nullptr;
        }
        return nullptr;
    }

    bool isTopOfScopeFunction()
    {
        return dynamic_cast<Function*>(this->scopeStack.top()) != nullptr;
    }


    Scope* getTopOfScope()
    {
        return this->scopeStack.top();
    }

    void addGlobalSymbol(std::string id, CommonElement *t)
    {
        globalSymbolTable[id] = t;
    }

    void pushAssignmentQueue(CommonElement* element)
    {
        this->assignmentQueue.push(element);
    }

    CommonElement* getAssignmentFront()
    {
        return this->assignmentQueue.front();
    }

    void popAssignmentQueue()
    {
        this->assignmentQueue.pop();
    }

    CommonElement* getGlobalSymbol(std::string id)
    {
        return this->globalSymbolTable.at(id);
    }

    bool isEnumValue(std::string id)
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            if (i.top()->symbolTable.find(id) != i.top()->symbolTable.end())
            {
                return dynamic_cast<Enum*>(i.top()->symbolTable.at(id)) != nullptr;
            }
        }
        return false;
    }

    bool isEnumId(std::string id)
    {
        try
        {
            enumValues.at(id);;
            return true;
        }
        catch(const std::exception& e)
        {
            return false;
        }
        
    }

    void printAllEnums()
    {
        for (auto i = enumValues.begin(); i != enumValues.end(); i++)
        {
            std::cout << i->first << " " << i->second << std::endl;
        }
    }

    void addEnumValue(std::string id, int value)
    {
        this->enumValues[id] = value;
    }

    // where to output
    std::ostream &out;
    
    // where am i, Scope stack
    std::stack<Scope*> scopeStack;
    bool inFunction = 0;

    // what do i always have
    SymbolTable globalSymbolTable;

    // how much of each function do i have
    std::map<std::string, int> functionSizes;

    //is the function a caller, callee, or both
    std::map<std::string, std::bitset<2>> functionCallType;

    // what registers are available
    RegisterAllocator *regManager = new RegisterAllocator();

    // where to store result of expression
    std::stack<std::string> dstRegStack; 


    // i want to write to this var, put on stack
    // or i want to read its value, load it into a register
    // 0 = read, 1 = write, (default = read)
    bool readWriteToId = 0;

    // i want the address of the array, keep that on dst
    // or i want to read the value at the address, load it into a register
    // 0 = read, 1 = write, (default = read)
    bool readWriteToAddress = 0;

    // used when writing to a global variable
    // 0 = local, 1 = global
    // default = local
    bool isGlobalAssignment = 0;

    // throw away the result of an expression
    bool noVar;

    // do i need to jump to return stack
    std::string returnLabel = "";

    // lone expressions, ignore or assign register inf inc/dec 
    bool noDstReg = 0;

    // do i break 
    std::stack<std::string> breakStack;

    // do i continue
    std::stack<std::string> continueStack;


    // if im returning from a function
    bool returnFunc = 0;

    // should i increment, decerement or do nothing to my upcoming postExpr/id
    UnaryOp unaryBool = NONE;

    // should i increment, decerement but using the postExprMethod
    PostOp postBool = NONE_POST;
    
    // used to calculate the strOffset for mutlidiemnsional arrays
    int arrayCounter = 1; // STARTS AT 1, NOT 0

    bool returnArr = 0; // used when returning an array

    bool sizeOfFlag = 0; // used to flag that im trying to return the SIZEOF

    std::queue<CommonElement*> assignmentQueue; // for assigning global vars

    int arrayGlobalCounter = 0; // used to keep track of how many have been initialized

    bool externalFunction = 0; // used to flag that im calling an external function

    bool mvExpressionFunc = 0; // used to flag that i need the function to move the to the dst reg on the stack in geenral expressions

    int recursiveLCounter = 0; // used to keep track of how many left recursive calls i have
    int recursiveRCounter = 0; // used to keep track of how many right recursive calls i have

    std::queue<std::string> switchLabels; // used to keep track of the labels for the switch statement
    
    bool switchFlag = 0; // used for switch, ugly solution compiling twice but it works

    std::string caseReg; // used for the case statement

    Enum *currentEnum = nullptr; // used to keep track of the current enum

    std::map<std::string, int> enumValues; // used to keep track of the enum values

    bool unsignedFlag = 0; // used to flag that im using an unsigned int

    std::stringstream ss; // used to keep track of the string stream



    std::stack<std::string> currentConstantLabel;
    // DEBUGGING


    void printGlobalSymbolTable()
    {
        std::cout << "Global Symbol Table" << std::endl;
        for (auto &i : this->globalSymbolTable)
        {
            std::cout << i.first << " " << i.second->type << std::endl;
        }
    }

    void printCurrentFunctionSymbolTable()
    {
        std::cout << "Current Function Symbol Table" << std::endl;
        for (auto &i : this->getCurrentFunction()->symbolTable)
        {
            std::cout << i.first << " " << i.second->type << std::endl;
        }
    }

    void printScopeSymbolTables()
    {
        std::stack<Scope*> temp = scopeStack;

        for (auto i = temp; !i.empty(); i.pop())
        {
            std::cout << "Scope Symbol Table\n" << i.top()->id << std::endl;
            for (auto &j : i.top()->symbolTable)
            {
                std::cout << j.first << " " << j.second->type << std::endl;
            }
        }
    }

    ~Context(){}
};



