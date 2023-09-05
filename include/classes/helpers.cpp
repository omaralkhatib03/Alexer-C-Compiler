#pragma once
#include <iostream>
#include <vector>
#include <cstdarg>
#include <map>
#include <string>
#include <stack>
#include <bitset>
#include <queue>
#include "context_classes.cpp"

inline void writeAssign(std::ostream &out, std::string dest, std::string src, std::string op, bool isFloat = false)
{

    if (op == "=")
    {
        out << ((isFloat) ? "fmv.s "  : "mv ") << dest << ", " << src << std::endl;
    }
    else if (op == "+=")
    {
        out << ((isFloat) ? "fadd.s " : "add ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "-=")
    {
        out << ((isFloat) ? "fsub.s " : "sub ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "*=")
    {
        out << ((isFloat) ? "fmul.s " : "mul ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "/=")
    {
        out << ((isFloat) ? "fdiv.s " : "div ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "%=")
    {
        out << ((isFloat) ? "frem.s " : "rem ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "&=")
    {
        out << ((isFloat) ? "fand.s " : "and ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "|=")
    {
        out << ((isFloat) ? "for.s "  : "or ")<< dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "^=")
    {
        out << ((isFloat) ? "fxor.s " : "xor ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == "<<=")
    {
        out << ((isFloat) ? "fsll.s " : "sll ")  << dest << ", " << dest << ", " << src << std::endl;
    }
    else if (op == ">>=")
    {
        out << ((isFloat) ? "fsra.s " : "sra ")  << dest << ", " << dest << ", " << src << std::endl;
    }

    else
    {
        std::cout << "Error: Invalid operator in writeAssign" << std::endl;
    }
}

inline void writeThreeIns(std::ostream &out, std::string ins, std::string dst, std::string a, std::string b)
{
    out << ins << " " << dst << ", " << a << ", " << b << std::endl;
}

inline void writeTwoIns(std::ostream &out, std::string ins, std::string dst, std::string a)
{
    out << ins << " " << dst << ", " << a << std::endl;
}

inline void writeOneIns(std::ostream &out, std::string ins, std::string a)
{
    out << ins << " " << a << std::endl;
}

inline void writeSwLw(std::ostream &out, std::string ins, std::string dst, std::string a, std::string b)
{
    out << ins << " " << dst << ", " << a << "(" << b << ")" << std::endl;
}

inline void writeOperation(std::ostream &out, std::string op, std::string dst, std::string a, std::string b, bool isFloat = false)
{
    if (op == "+")
    {
        writeThreeIns(out, ((isFloat) ? "fadd.s" : "add"), dst, a, b);
    }
    else if (op == "-")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
    }
    else if (op == "*")
    {
        writeThreeIns(out, (isFloat) ? "fmul.s" : "mul", dst, a, b);
    }
    else if (op == "/")
    {
        writeThreeIns(out, (isFloat) ? "fdiv.s" : "div", dst, a, b);
    }
    else if (op == "%")
    {
        writeThreeIns(out, (isFloat) ? "frem.s" : "rem", dst, a, b);
    }
    else if (op == "&")
    {
        writeThreeIns(out, (isFloat) ? "fand.s" : "and", dst, a, b);
    }
    else if (op == "|")
    {
        writeThreeIns(out, (isFloat) ? "for.s" : "or", dst, a, b);
    }
    else if (op == "^")
    {
        writeThreeIns(out, (isFloat) ? "fxor.s" : "xor", dst, a, b);
    }
    else if (op == "<<")
    {
        writeThreeIns(out, (isFloat) ? "fsll.s" : "sll", dst, a, b);
    }
    else if (op == ">>")
    {
        writeThreeIns(out, (isFloat) ? "fsra.s" : "sra", dst, a, b);
    }
    else if (op == "<")
    {
        writeThreeIns(out, (isFloat) ? "fslt.s" : "slt", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fandi.s" : "andi", dst, dst, "0xff");
    }
    else if (op == ">")
    {

        writeThreeIns(out, (isFloat) ? "fsgt.s" : "sgt", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fandi.s" : "andi", dst, dst, "0xff");
    }
    else if (op == "<=")
    {
        writeThreeIns(out, (isFloat) ? "fsgt.s" : "sgt", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fxori.s" : "xori", dst, dst, "1");
    }
    else if (op == ">=")
    {
        writeThreeIns(out, (isFloat) ? "fslt.s" : "slt", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fxori.s" : "xori", dst, dst, "1");
    }
    else if (op == "==")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
        writeTwoIns(out, (isFloat) ? "fseqz.s" : "seqz", dst, dst);
    }
    else if (op == "!=")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
        writeTwoIns(out, (isFloat) ? "fsnez.s" : "snez", dst, dst);
    }
}

inline void writeUnsigned(std::ostream &out, std::string op, std::string dst, std::string a, std::string b, bool isFloat = false)
{
    if (op == "+")
    {
        writeThreeIns(out, ((isFloat) ? "fadd.s" : "add"), dst, a, b);
    }
    else if (op == "-")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
    }
    else if (op == "*")
    {
        writeThreeIns(out, (isFloat) ? "fmul.s" : "mul", dst, a, b);
    }
    else if (op == "/")
    {
        writeThreeIns(out, (isFloat) ? "fdivu.s" : "divu", dst, a, b);
    }
    else if (op == "%")
    {
        writeThreeIns(out, (isFloat) ? "fremu.s" : "remu", dst, a, b);
    }
    else if (op == "&")
    {
        writeThreeIns(out, (isFloat) ? "fand.s" : "and", dst, a, b);
    }
    else if (op == "|")
    {
        writeThreeIns(out, (isFloat) ? "for.s" : "or", dst, a, b);
    }
    else if (op == "^")
    {
        writeThreeIns(out, (isFloat) ? "fxor.s" : "xor", dst, a, b);
    }
    else if (op == "<<")
    {
        writeThreeIns(out, (isFloat) ? "fsll.s" : "sll", dst, a, b);
    }
    else if (op == ">>")
    {
        writeThreeIns(out, (isFloat) ? "fsrl.s" : "srl", dst, a, b);
    }
    else if (op == "<")
    {
        writeThreeIns(out, (isFloat) ? "fsltu.s" : "sltu", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fandi.s" : "andi", dst, dst, "0xff");
    }
    else if (op == ">")
    {

        writeThreeIns(out, (isFloat) ? "fsgtu.s" : "sgtu", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fandi.s" : "andi", dst, dst, "0xff");
    }
    else if (op == "<=")
    {
        writeThreeIns(out, (isFloat) ? "fsgtu.s" : "sgtu", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fxori.s" : "xori", dst, dst, "1");
    }
    else if (op == ">=")
    {
        writeThreeIns(out, (isFloat) ? "fsltu.s" : "sltu", dst, a, b);
        writeThreeIns(out, (isFloat) ? "fxori.s" : "xori", dst, dst, "1");
    }
    else if (op == "==")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
        writeTwoIns(out, (isFloat) ? "fseqz.s" : "seqz", dst, dst);
    }
    else if (op == "!=")
    {
        writeThreeIns(out, (isFloat) ? "fsub.s" : "sub", dst, a, b);
        writeTwoIns(out, (isFloat) ? "fsnez.s" : "snez", dst, dst);
    }
}




inline void writeDecInc(Context *ctx, std::string identifier, bool isLocal)
{
    if (ctx->unaryBool != NONE)
    {
        if (ctx->unaryBool == DEC)
        {

            writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), ctx->dstRegStack.top(), "-1");
        }
        else if (ctx->unaryBool == INC)
        {
            writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), ctx->dstRegStack.top(), "1");
        }
        ctx->unaryBool = NONE;

        if (isLocal)
        {
            if (ctx->getLocalSymbol(identifier)->type == "float")
            {
                writeSwLw(ctx->out, "fsw", ctx->dstRegStack.top(), std::to_string(ctx->getLocalSymbol(identifier)->offset), "s0");
            }
            else
            {
                writeSwLw(ctx->out, "sw", ctx->dstRegStack.top(), std::to_string(ctx->getLocalSymbol(identifier)->offset), "s0");
            }
        }
        else
        {
            std::string tmpGl = ctx->regManager->getReg("t", ctx->getGlobalSymbol(identifier)->type);
            ctx->out << "lui " << tmpGl << ", %hi(" << identifier << ")" << std::endl;
            writeThreeIns(ctx->out, "addi", tmpGl, tmpGl, "%lo(" + identifier + ")");

            if (ctx->getGlobalSymbol(identifier)->type == "float")
            {

                writeSwLw(ctx->out, "fsw", ctx->dstRegStack.top(), "0", tmpGl);
            }
            else
            {
                writeSwLw(ctx->out, "sw", ctx->dstRegStack.top(), "0", tmpGl);
            }
        }

        // writeSwLw(ctx->out, "lw", ctx->dstRegStack.top(), std::to_string(ctx->getLocalSymbol(identifier)->offset), "s0"); gcc does it, no clue why, see if neccesary
    }
}

inline void writePostDecInc(Context *ctx, std::string identifier, bool isLocal)
{
    if (ctx->postBool != NONE_POST)
    {
        std::string currentReg = ctx->dstRegStack.top();

        std::string tempReg = ctx->regManager->getReg("t", ctx->getSymbol(identifier)->type);
        ctx->dstRegStack.push(tempReg);
        if (ctx->postBool == POST_DEC)
        {
            writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), currentReg, "-1");
        }
        else if (ctx->postBool == POST_INC)
        {
            writeThreeIns(ctx->out, "addi", ctx->dstRegStack.top(), currentReg, "1");
        }
        ctx->postBool = NONE_POST;

        if (isLocal)
        {
            writeSwLw(ctx->out, "sw", ctx->dstRegStack.top(), std::to_string(ctx->getLocalSymbol(identifier)->offset), "s0");
            ctx->regManager->freeReg(ctx->dstRegStack.top());
            ctx->dstRegStack.pop();
        }
        else
        {
            std::string tmpGl = ctx->regManager->getReg("t", ctx->getSymbol(identifier)->type);
            ctx->out << "lui " << tmpGl << ",\%hi(" << identifier << ")" << std::endl;
            writeThreeIns(ctx->out, "addi", tmpGl, tmpGl, "\%lo(" + identifier + ")");

            if (ctx->getGlobalSymbol(identifier)->type == "float")
            {

                writeSwLw(ctx->out, "fsw", ctx->dstRegStack.top(), "0", tmpGl);
            }
            else
            {
                writeSwLw(ctx->out, "sw", ctx->dstRegStack.top(), "0", tmpGl);
            }
            ctx->regManager->freeReg(ctx->dstRegStack.top());
            ctx->dstRegStack.pop();
        }

        // writeSwLw(ctx->out, "lw", ctx->dstRegStack.top(), std::to_string(ctx->getLocalSymbol(identifier)->offset), "s0"); gcc does it, no clue why, see if neccesary
    }
}