%option noyywrap

%{
    #include <stdio.h>
    #include "parser.tab.hpp"
    void count();
    int check_type();
    extern "C" int fileno(FILE *stream);
	extern Tracker *trk;
	Tracker *trk = new Tracker();

%}


D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%%

"/*"[^\*\/]"*/" { }
"//"[^\/\/]"\n" { }

"auto"			{ count(); yylval.string = new std::string(yytext); return(AUTO); }
"break"			{ count(); yylval.string = new std::string(yytext); return(BREAK); }
"case"			{ count(); yylval.string = new std::string(yytext); return(CASE); }
"char"			{ count(); yylval.string = new std::string(yytext); return(CHAR); }
"const"			{ count(); yylval.string = new std::string(yytext); return(CONST); }
"continue"		{ count(); yylval.string = new std::string(yytext); return(CONTINUE); }
"default"		{ count(); yylval.string = new std::string(yytext); return(DEFAULT); }
"do"			{ count(); yylval.string = new std::string(yytext); return(DO); }
"double"		{ count(); yylval.string = new std::string(yytext); return(DOUBLE); }
"else"			{ count(); yylval.string = new std::string(yytext); return(ELSE); }
"enum"			{ count(); yylval.string = new std::string(yytext); return(ENUM); }
"extern"		{ count(); yylval.string = new std::string(yytext); return(EXTERN); }
"float"			{ count(); yylval.string = new std::string(yytext); return(FLOAT); }
"for"			{ count(); yylval.string = new std::string(yytext); return(FOR); }
"goto"			{ count(); yylval.string = new std::string(yytext); return(GOTO); }
"if"			{ count(); yylval.string = new std::string(yytext); return(IF); }
"int"			{ count(); yylval.string = new std::string(yytext); return(INT); }
"long"			{ count(); yylval.string = new std::string(yytext); return(LONG); }
"register"		{ count(); yylval.string = new std::string(yytext); return(REGISTER); }
"return"		{ count(); yylval.string = new std::string(yytext); return(RETURN); }
"short"			{ count(); yylval.string = new std::string(yytext); return(SHORT); }
"signed"		{ count(); yylval.string = new std::string(yytext); return(SIGNED); }
"sizeof"		{ count(); yylval.string = new std::string(yytext); return(SIZEOF); }
"static"		{ count(); yylval.string = new std::string(yytext); return(STATIC); }
"struct"		{ count(); yylval.string = new std::string(yytext); return(STRUCT); }
"switch"		{ count(); yylval.string = new std::string(yytext); return(SWITCH); }
"typedef"		{ count(); yylval.string = new std::string(yytext); return(TYPEDEF); }
"union"			{ count(); yylval.string = new std::string(yytext); return(UNION); }
"unsigned"		{ count(); yylval.string = new std::string(yytext); return(UNSIGNED); }
"void"			{ count(); yylval.string = new std::string(yytext); return(VOID); }
"volatile"		{ count(); yylval.string = new std::string(yytext); return(VOLATILE); }
"while"			{ count(); yylval.string = new std::string(yytext); return(WHILE); }

{L}({L}|{D})*		{ count(); yylval.string = new std::string(yytext); return(check_type()); }

0[xX]{H}+{IS}?		{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }
0{D}+{IS}?		{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }
{D}+{IS}?		{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }
L?'(\\.|[^\\'])+'	{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }

{D}+{E}{FS}?		{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }
{D}*"."{D}+({E})?{FS}?	{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }
{D}+"."{D}*({E})?{FS}?	{ count(); yylval.string = new std::string(yytext); return(CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ count(); yylval.string = new std::string(yytext); return(STRING_LITERAL); }

"..."			{ count(); yylval.string = new std::string(yytext); return(ELLIPSIS); }
">>="			{ count(); yylval.string = new std::string(yytext); return(RIGHT_ASSIGN); }
"<<="			{ count(); yylval.string = new std::string(yytext); return(LEFT_ASSIGN); }
"+="			{ count(); yylval.string = new std::string(yytext); return(ADD_ASSIGN); }
"-="			{ count(); yylval.string = new std::string(yytext); return(SUB_ASSIGN); }
"*="			{ count(); yylval.string = new std::string(yytext); return(MUL_ASSIGN); }
"/="			{ count(); yylval.string = new std::string(yytext); return(DIV_ASSIGN); }
"%="			{ count(); yylval.string = new std::string(yytext); return(MOD_ASSIGN); }
"&="			{ count(); yylval.string = new std::string(yytext); return(AND_ASSIGN); }
"^="			{ count(); yylval.string = new std::string(yytext); return(XOR_ASSIGN); }
"|="			{ count(); yylval.string = new std::string(yytext); return(OR_ASSIGN); }
">>"			{ count(); yylval.string = new std::string(yytext); return(RIGHT_OP); }
"<<"			{ count(); yylval.string = new std::string(yytext); return(LEFT_OP); }
"++"			{ count(); yylval.string = new std::string(yytext); return(INC_OP); }
"--"			{ count(); yylval.string = new std::string(yytext); return(DEC_OP); }
"->"			{ count(); yylval.string = new std::string(yytext); return(PTR_OP); }
"&&"			{ count(); yylval.string = new std::string(yytext); return(AND_OP); }
"||"			{ count(); yylval.string = new std::string(yytext); return(OR_OP); }
"<="			{ count(); yylval.string = new std::string(yytext); return(LE_OP); }
">="			{ count(); yylval.string = new std::string(yytext); return(GE_OP); }
"=="			{ count(); yylval.string = new std::string(yytext); return(EQ_OP); }
"!="			{ count(); yylval.string = new std::string(yytext); return(NE_OP); }
";"				{ count(); yylval.string = new std::string(yytext); return(SEMI); }
("{"|"<%")		{ count(); yylval.string = new std::string(yytext); return(L_CURL); }
("}"|"%>")		{ count(); yylval.string = new std::string(yytext); return(R_CURL); }
","				{ count(); yylval.string = new std::string(yytext); return(COMMA); }
":"				{ count(); yylval.string = new std::string(yytext); return(COLON); }
"="				{ count(); yylval.string = new std::string(yytext); return(EQ_SIGN); }
"("				{ count(); yylval.string = new std::string(yytext); return(L_BRAC); }
")"				{ count(); yylval.string = new std::string(yytext); return(R_BRAC); }
("["|"<:")		{ count(); yylval.string = new std::string(yytext); return(L_SQ); }
("]"|":>")		{ count(); yylval.string = new std::string(yytext); return(R_SQ); }
"."				{ count(); yylval.string = new std::string(yytext); return(DOT); }
"&"				{ count(); yylval.string = new std::string(yytext); return(AND_UNARY); }
"!"				{ count(); yylval.string = new std::string(yytext); return(EXCLA); }
"~"				{ count(); yylval.string = new std::string(yytext); return(TILDE); }
"-"				{ count(); yylval.string = new std::string(yytext); return(SUB_OP); }
"+"				{ count(); yylval.string = new std::string(yytext); return(ADD_UNARY); }
"*"				{ count(); yylval.string = new std::string(yytext); return(PTR_TIMES); }
"/"				{ count(); yylval.string = new std::string(yytext); return(DIV_OP); }
"%"				{ count(); yylval.string = new std::string(yytext); return(MOD_OP); }
"<"				{ count(); yylval.string = new std::string(yytext); return(LT_OP); }
">"				{ count(); yylval.string = new std::string(yytext); return(GT_OP); }
"^"				{ count(); yylval.string = new std::string(yytext); return(XOR_OP); }
"|"				{ count(); yylval.string = new std::string(yytext); return(OR_UNARY); }
"?"				{ count(); yylval.string = new std::string(yytext); return(TERNARY_OP); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%


// comment()
// {
// 	char c, c1;

// loop:
// 	while ((c = input()) != PTR_TIMES && c != 0)
// 		putchar(c);

// 	if ((c1 = input()) != DIV_OP && c != 0)
// 	{
// 		unput(c1);
// 		goto loop;
// 	}

// 	if (c != 0)
// 		putchar(c1);
// }


int column = 0;

void count()
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;

	ECHO;
}


int check_type()
{
/*
* pseudo code --- this is what it should check
*
*	if (yytext == type_name)
*		return(TYPE_NAME);
*
*	return(IDENTIFIER);
*/
// std::cout << std::endl;
// std::cout << "PRINTING TYPEDEFS" << std::endl;
// trk->printTypeDefs();
// std::cout << std::endl;
// std::cout << "DONE PRINTING" << std::endl;
std::string *tmp = new std::string(yytext);
if (trk->isTypedef(*tmp))
{
	// std::cout << "FOUND TYPEDEF" << std::endl;
	return(TYPE_NAME);
}
/*
*	it actually will only return IDENTIFIER
*/

	return(IDENTIFIER);
}



