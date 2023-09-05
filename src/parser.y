%code requires{
    #include <cassert>
    #include <stdio.h>
	#include <string>
	#include <cassert>
	#include <iostream>
	#include "ast.h"

	extern CommonNode *root; 
	extern Tracker *trk;
	

    int yylex(void);
	void yyerror(std::string s);
}

%union {
	CommonNodePtr node;
	std::string *string;
}


%token IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
%token L_BRAC R_BRAC L_SQ R_SQ L_CURL R_CURL SEMI COMMA DOT
%token COLON EQ_SIGN AND_UNARY EXCLA TILDE SUB_OP ADD_UNARY PTR_TIMES DIV_OP MOD_OP LT_OP GT_OP XOR_OP OR_UNARY TERNARY_OP
%token END

%start program

%nonassoc NOELSE
%nonassoc ELSE

%type <node> program external_declaration function_definition declaration_specifiers compound_statement declarator
%type <node> declaration init_declarator_list init_declarator direct_declarator
%type <node> parameter_type_list parameter_list parameter_declaration
%type <node> identifier_list type_name specifier_qualifier_list
%type <node> struct_or_union_specifier struct_declaration_list struct_declaration
%type <node> struct_declarator_list struct_declarator
%type <node> enum_specifier enumerator_list enumerator
%type <node> type_qualifier_list initializer_list
%type <node> statement expression_statement labeled_statement
%type <node> selection_statement iteration_statement jump_statement
%type <node> translation_unit declaration_list statement_list
%type <node> postfix_expression argument_expression_list unary_expression
%type <node> cast_expression multiplicative_expression additive_expression
%type <node> shift_expression relational_expression equality_expression
%type <node> and_expression exclusive_or_expression inclusive_or_expression
%type <node> logical_and_expression logical_or_expression conditional_expression
%type <node> assignment_expression expression constant_expression
%type <node> type_specifier assignment_operator unary_operator struct_or_union abstract_declarator direct_abstract_declarator
%type <node> pointer type_qualifier primary_expression initializer storage_class_specifier 

%type <string> CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID TYPE_NAME
%type <string> L_BRAC R_BRAC L_CURL R_CURL SEMI COMMA 
%type <string> MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%type <string> PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
%type <string> IDENTIFIER CONSTANT STRING_LITERAL SIZEOF RETURN
%type <string> BREAK CONTINUE IF ELSE NOELSE WHILE FOR 
%type <string> GOTO SWITCH CASE DEFAULT
%type <string> TYPEDEF EXTERN STATIC AUTO REGISTER
%type <string> L_SQ R_SQ DOT STRUCT ENUM
%type <string> COLON EQ_SIGN AND_UNARY EXCLA TILDE SUB_OP ADD_UNARY PTR_TIMES DIV_OP MOD_OP LT_OP GT_OP XOR_OP OR_UNARY TERNARY_OP

%%

// primary expressions -> id could be arr, func, or var
primary_expression
	: IDENTIFIER {$$ = new IdentifierNode(*$1);}
	| CONSTANT {$$ = new ConstantNode(*$1); }
	| STRING_LITERAL {$$ = new StringLiteralNode(*$1); }
	| L_BRAC expression R_BRAC {$$ = new PrimaryExpressionNode(*$1, $2, *$3); }
	;

// postfix reffers to operators that appear after their expression
postfix_expression
	: primary_expression {$$ = $1;}
	| postfix_expression L_SQ expression R_SQ {$$ = new ArrayIndexPostfixExpressionNode($1, *$2, $3, *$4);}
	| postfix_expression L_BRAC R_BRAC 
	{
		$$ = new FunctionCallWithoutArgsNode($1, *$2, *$3);
		if (trk->currentScopeId != "global")
		{
			trk->setScopeAsCaller(trk->currentScopeId);			
		}
		if (trk->currentScopeId != "global")
		{
			trk->setScopeAsCaller(trk->currentScopeId);
			PostfixExpressionNode *postNode = dynamic_cast<PostfixExpressionNode*>($1);
			if (postNode != NULL)
			{
				trk->setScopeAsCallee(postNode->getId());
			}
		}

	}
	| postfix_expression L_BRAC argument_expression_list R_BRAC  
	{ 
		$$ = new FunctionCallWithArgsNode($1, *$2, $3, *$4); 
		if (trk->currentScopeId != "global")
		{
			trk->setScopeAsCaller(trk->currentScopeId);
			PostfixExpressionNode *postNode = dynamic_cast<PostfixExpressionNode*>($1);
			if (postNode != NULL)
			{
				trk->setScopeAsCallee(postNode->getId());
			}
		}	
		std::string id;
		if (dynamic_cast<IdentifierNode*>($1) != NULL)
		{
			id = dynamic_cast<IdentifierNode*>($1)->getId();
		}
		else
		{
			throw std::runtime_error("Error: Invalid function call");
		}

		
		// ArgumentExpressionListNode *tmp = dynamic_cast<ArgumentExpressionListNode*>($3);
		for (int i = 0; i < (int)trk->getFunctionParams(id).size(); i++)
		{	
			if ((trk->getFunctionParams(id).at(i)) == "double" || (trk->getFunctionParams(id).at(i)) == "struct" || i > 8)
			{
				trk->allocateMemory(trk->getCurrentScopeId(), typeSizes.at((trk->getFunctionParams(id).at(i))));
			}
		}
	}
	| postfix_expression DOT IDENTIFIER {$$ = new PostfixExpressionNode($1, *$2, *$3); }
	| postfix_expression PTR_OP IDENTIFIER {$$ = new PostfixExpressionNode($1, *$2, *$3); }
	| postfix_expression INC_OP {$$ = new IncDecPostfixExpressionNode($1, *$2); }
	| postfix_expression DEC_OP {$$ = new IncDecPostfixExpressionNode($1, *$2); }
	;


argument_expression_list
	: assignment_expression {$$ = new ArgumentExpressionListNode($1);}
	| argument_expression_list COMMA assignment_expression
	{
		$$ = $1;
		$$->addChild($3);
	} 
	;

//unary operators are operators that appear before their expression
unary_expression  
	: postfix_expression {$$ = $1;}
	| INC_OP unary_expression {$$ = new IncDecUnaryExpressionNode(*$1, $2); }
	| DEC_OP unary_expression  {$$ = new IncDecUnaryExpressionNode(*$1, $2); }
	| unary_operator cast_expression {$$ = new UnaryExpressionNode($1, $2); }
	| SIZEOF unary_expression {$$ = new SizeOfUnaryExpressionNode(*$1, $2);}
	| SIZEOF L_BRAC type_name R_BRAC {$$ = new SizeOfTypeUnarExpressionNode(*$1, *$2, $3, *$4);}
	;

//Unary bitwise operators do not have a tokens, are picked up in grammer
unary_operator
	: AND_UNARY {$$ = new UnaryOperator(*$1);}
	| PTR_TIMES {$$ = new UnaryOperator(*$1);}
	| ADD_UNARY {$$ = new UnaryOperator(*$1);}
	| SUB_OP {$$ = new UnaryOperator(*$1);}
	| TILDE {$$ = new UnaryOperator(*$1);}
	| EXCLA {$$ = new UnaryOperator(*$1);}
	;

// typecasting
cast_expression
	: unary_expression {$$ = $1;}
	| L_BRAC type_name R_BRAC cast_expression {$$ = new CastExpressionNode(*$1, $2, *$3, $4);}
	;

// expressions
multiplicative_expression
	: cast_expression {$$ = $1;}
	| multiplicative_expression PTR_TIMES cast_expression { $$ = new MultiplicativeExpressionNode($1, *$2, $3);}
	| multiplicative_expression DIV_OP cast_expression { $$ = new MultiplicativeExpressionNode($1, *$2, $3);}
	| multiplicative_expression MOD_OP cast_expression { $$ = new MultiplicativeExpressionNode($1, *$2, $3);}
	;

additive_expression
	: multiplicative_expression {$$ = $1;}
	| additive_expression ADD_UNARY multiplicative_expression { $$ = new AdditiveExpressionNode($1, *$2, $3);}
	| additive_expression SUB_OP multiplicative_expression { $$ = new AdditiveExpressionNode($1, *$2, $3);}
	;

shift_expression
	: additive_expression {$$ = $1;}
	| shift_expression LEFT_OP additive_expression { $$ = new ShiftExpressionNode($1, *$2, $3);}
	| shift_expression RIGHT_OP additive_expression { $$ = new ShiftExpressionNode($1, *$2, $3);}
	;

relational_expression
	: shift_expression {$$ = $1;}
	| relational_expression LT_OP shift_expression { $$ = new RelationalExpressionNode($1, *$2, $3);}
	| relational_expression GT_OP shift_expression { $$ = new RelationalExpressionNode($1, *$2, $3);}
	| relational_expression LE_OP shift_expression { $$ = new RelationalExpressionNode($1, *$2, $3);}
	| relational_expression GE_OP shift_expression { $$ = new RelationalExpressionNode($1, *$2, $3);}
	;

equality_expression
	: relational_expression {$$ = $1;}
	| equality_expression EQ_OP relational_expression { $$ = new EqualityExpressionNode($1, *$2, $3);}
	| equality_expression NE_OP relational_expression { $$ = new EqualityExpressionNode($1, *$2, $3);}
	;

and_expression
	: equality_expression {$$ = $1;}
	| and_expression AND_UNARY equality_expression { $$ = new AndExpressionNode($1, *$2, $3);}
	;

exclusive_or_expression
	: and_expression {$$ = $1;}
	| exclusive_or_expression XOR_OP and_expression { $$ = new ExclusiveOrExpressionNode($1, *$2, $3);}
	;

inclusive_or_expression
	: exclusive_or_expression {$$ = $1;}
	| inclusive_or_expression OR_UNARY exclusive_or_expression { $$ = new InclusiveOrExpressionNode($1, *$2, $3);}
	;

logical_and_expression
	: inclusive_or_expression {$$ = $1;}
	| logical_and_expression AND_OP inclusive_or_expression { $$ = new LogicalAndExpressionNode($1, *$2, $3);}
	;

logical_or_expression
	: logical_and_expression {$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression { $$ = new LogicalOrExpressionNode($1, *$2, $3);}
	;

conditional_expression
	: logical_or_expression {$$ = $1;}
	| logical_or_expression TERNARY_OP expression COLON conditional_expression { $$ = new TernaryExpressionNode($1, *$2, $3, *$4, $5);}
	;

// end of expressions

// assigning to vars, those defined in symbol table
assignment_expression
	: conditional_expression {$$ = new AssignerExpressionNode($1);}
	| unary_expression assignment_operator assignment_expression { $$ = new AssigneeExpressionNode($1, $2, $3);}
	;

// assignment operatos
assignment_operator
	: EQ_SIGN {$$ = new AssignmentOperatorNode(*$1);}
	| MUL_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| DIV_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| MOD_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| ADD_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| SUB_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| LEFT_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| RIGHT_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| AND_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| XOR_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	| OR_ASSIGN {$$ = new AssignmentOperatorNode(*$1);}
	;

expression
	: assignment_expression { $$ = new ExpressionNode($1);}
	| expression COMMA assignment_expression { $$ = new ExpressionNode($1, *$2, $3); }
	;

constant_expression
	: conditional_expression {$$ = new ConstantExpressionNode($1);}
	;

// declaring vars
declaration
	: declaration_specifiers SEMI { $$ = new DeclarationNode($1);}
	| declaration_specifiers init_declarator_list SEMI 
	{ 
		std::string type;
		StorageClassSpecifierNode* storageClass = dynamic_cast<StorageClassSpecifierNode*>($1->getChildren()[0]);


		if (storageClass != NULL )
		{
			
			DeclarationSpecNode *declSpec = dynamic_cast<DeclarationSpecNode*>($1);
			if (declSpec == NULL) {throw std::runtime_error("declaration: could not find DeclarationSpecNode");}
			type = declSpec->getTypeString();
		

		InitDeclarationListNode *initDeclList = dynamic_cast<InitDeclarationListNode*>($2);
		if (initDeclList == NULL) {throw std::runtime_error("typedef: could not find InitDeclarationList");} 
		
		for (int i = 0; i < (int)initDeclList->getChildren().size(); i++)
		{
			InitDeclaratorNode *initDecl = dynamic_cast<InitDeclaratorNode*>(initDeclList->getChildren()[i]);
			if (initDecl == NULL){throw std::runtime_error("could not find InitDeclaratorNode typdef");}
			DeclaratorNode *declNode = dynamic_cast<DeclaratorNode*>(initDecl->getChildren()[0]);
			if (declNode == NULL){throw std::runtime_error("could not find declarator node typedef");}
		    std::string id = declNode->getDirDeclaratorId();
			trk->addTypedef(id, type);
		}

		}
		else
		{
			// std::cout << " hit " << std::endl;
			int typeSize;
			if (trk->getCurrentScopeId() != "global")
			{
				if (dynamic_cast<EnumSpecifierNode*>((dynamic_cast<DeclarationSpecNode*>($1)->getChildren()[0])) != NULL)
				{
					typeSize = 8; // 8 bytes for enum, overestimates
				}	
				else
				{
					typeSize =dynamic_cast<DeclarationSpecNode*>($1)->getTypeSize();
				}
				int declarationLength = dynamic_cast<InitDeclarationListNode*>($2)->getTotalDeclarations();
				// std::cout << " total declarations : " << declarationLength << std::endl;
				trk->allocateMemory( trk->getCurrentScopeId() , typeSize * (declarationLength)); // overestimates, but that's ok

			}

			$$ = new DeclarationNode($1, $2);
		}

	}
	;

// no type quals, either typedef, type, or enum, for now just types
declaration_specifiers
	: storage_class_specifier { $$ = new DeclarationSpecNode($1);}
	| storage_class_specifier declaration_specifiers 
	{
		$$ = new DeclarationSpecNode($1);
		for (auto child : $2->getChildren())
		{
			$$->addChild(child);
		}
	}
	| type_specifier { $$ = new DeclarationSpecNode($1);}
	| type_specifier declaration_specifiers
	{
		$$ = new DeclarationSpecNode($1);
		for (auto child : $2->getChildren())
		{
			$$->addChild(child);
		}

	}
	| type_qualifier 
	{
		$$ = new DeclarationSpecNode($1);
	}
	| type_qualifier declaration_specifiers 
	{
		$$ = new DeclarationSpecNode($1);
		for (auto child : $2->getChildren())
		{
			$$->addChild(child);
		}
	}
	;

// init declarator is to rec through initzer, init declarator list comma is for declaring multiple vars of same type
init_declarator_list
	: init_declarator {$$ = new InitDeclarationListNode($1);}
	| init_declarator_list COMMA init_declarator {} 
	{
		$$ = $1;
		$$->addChild($3);
	}

// run through initzer and assign (put value on stack)
init_declarator 
	: declarator { $$ = new InitDeclaratorNode($1);}
	| declarator EQ_SIGN initializer { $$ = new InitDeclaratorNode($1, *$2, $3);}
	;

storage_class_specifier
	: TYPEDEF { $$ = new StorageClassSpecifierNode(*$1);}
	| EXTERN {}
	| STATIC {}
	| AUTO {}
	| REGISTER {}
	;

type_specifier
	: VOID { $$ = new TypeNode(*$1);} 
	| CHAR { $$ = new TypeNode(*$1);} 
	| SHORT { $$ = new TypeNode(*$1);} 
	| INT {$$ = new TypeNode(*$1);} 
	| LONG { $$ = new TypeNode(*$1);} 
	| FLOAT { $$ = new TypeNode(*$1);} 
	| DOUBLE { $$ = new TypeNode(*$1);} 
	| SIGNED { $$ = new TypeNode(*$1);} 
	| UNSIGNED { $$ = new TypeNode(*$1);} 
	| struct_or_union_specifier {$$ = $1;}
	| enum_specifier {$$ = $1;}
	| TYPE_NAME 
	{
		std::string tmpType = trk->getTypedefType(*$1);
		$$ = new TypeNode(tmpType);
	}
	;

//struct stuff
struct_or_union_specifier
	: struct_or_union IDENTIFIER L_CURL struct_declaration_list R_CURL { $$ = new StructSpecifierNode($1, *$2, $4);}
	| struct_or_union L_CURL struct_declaration_list R_CURL {$$ = new StructSpecifierNode($1, $3);}
	| struct_or_union IDENTIFIER {$$ = new StructSpecifierNode($1, *$2);}
	;

struct_or_union
	: STRUCT { $$ = new StructTerminal(*$1); }
	| UNION {}
	;

struct_declaration_list
	: struct_declaration { $$ = new StructDeclarationListNode($1);}
	| struct_declaration_list struct_declaration 
	{
		$$ = $1;
		$$->addChild($2);
	}
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list SEMI {$$ = new StructDeclarationNode($1, $2);}
	;

// no qualifiers, just types 
specifier_qualifier_list
	: type_specifier specifier_qualifier_list 
	{}
	| type_specifier {$$ = $1;}
	| type_qualifier specifier_qualifier_list 
	{
		$$ = new StructTypeSpecListNode($2);
		$$->getChildren().insert($$->getChildren().begin(), $1);
	}
	| type_qualifier 
	{
		$$ = new StructTypeSpecListNode($1);
	}
	;

struct_declarator_list
	: struct_declarator {$$ = new StructDeclaratorListNode($1);}
	| struct_declarator_list COMMA struct_declarator 
	{
		$$ = $1;
		$$->addChild($3);
	}
	;

struct_declarator
	: declarator {$$ = new StructDeclaratorNode($1);}
	| COLON constant_expression {$$ = new StructDeclaratorNode($2);}
	| declarator COLON constant_expression { $$ = new StructDeclaratorNode($1, $3); }
	;
// end struct stuff
enum_specifier
	: ENUM L_CURL enumerator_list R_CURL {$$ = new EnumSpecifierNode(*$1, $3);}
	| ENUM IDENTIFIER L_CURL enumerator_list R_CURL 
	{
		$$ = new EnumSpecifierNode(*$1, $4);
		IdentifierNode* id = new IdentifierNode(*$2);
		$$->addChild(id);
	}
	| ENUM IDENTIFIER 
	{
		$$ = new EnumSpecifierNode(*$1);
		IdentifierNode* id = new IdentifierNode(*$2);
		$$->addChild(id);
	}
	;

// enum stuff
enumerator_list
	: enumerator 
	{
		$$ = new EnumListNode($1);
	}
	| enumerator_list COMMA enumerator 
	{
		$$ = $1;
		$$->addChild($3);
	}
	;

enumerator
	: IDENTIFIER {$$ = new EnumIdNode(*$1);}
	| IDENTIFIER EQ_SIGN constant_expression 
	{
		$$ = new EnumaratorNode(*$1, $3);
	}
	;
// end enum stuff

type_qualifier
	: CONST {$$ = new TypeQualNode (*$1);}
	| VOLATILE {$$ = new TypeQualNode (*$1);}
	;

// for declarations
declarator
	: pointer direct_declarator { $$ = new DeclaratorNode($1, $2);}
	| direct_declarator { $$ = new DeclaratorNode($1);}
	;

// declaring vars and stuff
direct_declarator
	: IDENTIFIER  { $$ = new IdDirDeclarator(*$1);}
	| L_BRAC declarator R_BRAC 
	{
		$$ = new DirDeclaratorNode($2);
	} 
	| direct_declarator L_SQ constant_expression R_SQ { $$ = new ArrayDirDeclaratorWithSize($1, *$2, $3, *$4);
		// int size = dynamic_cast<ConstantExpressionNode*>($3)->getValue(); used for debugging, calculates size of array
		// std::cout << "size of array: " << size << std::endl;  ca
	}
	| direct_declarator L_SQ R_SQ { $$ = new ArrayDirDeclaratorWithoutSize($1, *$2, *$3);}
	| direct_declarator L_BRAC parameter_type_list R_BRAC
	{
		$$ = new FuncDirDeclParamsNode($1, *$2, $3, *$4);
		trk->addScope(dynamic_cast<IdDirDeclarator*>($1)->getId());
		ParamListNode *tmp = dynamic_cast<ParamListNode*>($3);
		if (tmp == NULL) { throw "expecting parameters in function"; }
		std::vector<std::string> paramTypes;
		for (int i = 0; i < (int)tmp->getChildren().size(); i++)
		{
			ParameterDeclarationNode* paramDeclTmp = dynamic_cast<ParameterDeclarationNode*>(tmp->getChildren()[i]);
			if (paramDeclTmp == NULL) { std::cout << "\nexpecting parameter declaration" << std::endl; }
			int size = paramDeclTmp->getTypeSize();
			paramTypes.push_back(paramDeclTmp->getTypeString());
			trk->allocateMemory( trk->getCurrentScopeId() , size); // overestimates, but that's ok
		}
		trk->addFunctionParams(dynamic_cast<IdDirDeclarator*>($1)->getId(), paramTypes);
		// trk->printFunctionParams();
	}
	| direct_declarator L_BRAC identifier_list R_BRAC
	{
		$$ = new FuncDirDeclParamsNode($1, *$2, $3, *$4);
	}
	| direct_declarator L_BRAC R_BRAC 
	{
		trk->addScope(dynamic_cast<IdDirDeclarator*>($1)->getId());	
		$$ = new FuncDirDeclNode($1);
		CommonNodePtr tmp1 = new TerminalNode(*$2);
		CommonNodePtr tmp2 = new TerminalNode(*$3);
		$$->addChild(tmp1);
		$$->addChild(tmp2);		
		trk->addFunctionParams(dynamic_cast<IdDirDeclarator*>($1)->getId(), std::vector<std::string>());		
	}// function declared with no params
	;

// pointer
pointer
	: PTR_TIMES {$$ = new TerminalNode(*$1);}
	| PTR_TIMES type_qualifier_list { $$ = new PointerNode($2);}
	| PTR_TIMES pointer { $$ = new PointerNode(*$1, $2); } 
	| PTR_TIMES type_qualifier_list pointer {$$ = new PointerNode(*$1, $2, $3);}
	;

// not ype quals
type_qualifier_list
	: type_qualifier { }
	| type_qualifier_list type_qualifier 
	{}
	;

// paramters of function
parameter_type_list
	: parameter_list {$$ = $1;}
	| parameter_list COMMA ELLIPSIS {}
	;

// paramater list
parameter_list
	: parameter_declaration {$$ = new ParamListNode($1);}
	| parameter_list COMMA parameter_declaration 
	{
		$$ = $1;
		$$->addChild($3);
	}
	;

// param decla, similiar to normal decla but only for params
parameter_declaration
	: declaration_specifiers declarator { $$ = new ParameterDeclarationNode($1, $2);}
	| declaration_specifiers abstract_declarator {$$ = new ParameterDeclarationNode($1, $2);}
	| declaration_specifiers { $$ = new ParameterDeclarationNode($1); }
	;

// idt is used, for kr style
identifier_list
	: IDENTIFIER {$$ = new IdListNode(*$1);}
	| identifier_list COMMA IDENTIFIER 
	{
		$$ = $1;
		dynamic_cast<IdListNode*>($$)->addChildId(*$3);
	}
	;

// kr style aswell
type_name
	: specifier_qualifier_list 
	{

	}
	| specifier_qualifier_list abstract_declarator 
	{
		
	}
	;

// not sure abt abstract stuff
abstract_declarator
	: pointer { $$ = new AbstractDeclaratorNode($1);}
	| direct_abstract_declarator { $$ = new AbstractDeclaratorNode($1);}
	| pointer direct_abstract_declarator { $$ = new AbstractDeclaratorNode($1, $2);}
	;

direct_abstract_declarator
	: L_BRAC abstract_declarator R_BRAC {}
	| L_SQ L_SQ {}
	| L_SQ constant_expression R_SQ {}
	| direct_abstract_declarator L_SQ L_SQ {}
	| direct_abstract_declarator L_SQ constant_expression R_SQ {}
	| L_BRAC R_BRAC {}
	| L_BRAC parameter_type_list R_BRAC {}
	| direct_abstract_declarator L_BRAC R_BRAC {}
	| direct_abstract_declarator L_BRAC parameter_type_list R_BRAC {}
	;


// initzer
initializer
	: assignment_expression { $$ = new InitializerNode($1); }
	| L_CURL initializer_list R_CURL { $$ = new InitializerNode(*$1, $2, *$3);}
	| L_CURL initializer_list COMMA R_CURL { $$ = new InitializerNode(*$1, $2, *$3, *$4);}
	;

// list for initzer
initializer_list
	: initializer { $$ = new InitializerListNode($1);}
	| initializer_list COMMA initializer 
	{
		$$ = new InitializerListNode($1);
		$$->addChild($3);
	}
	;

// statements, should be straight forward
statement
	: labeled_statement { $$ = $1;}
	| compound_statement { $$ = new StatementNode($1);}
	| expression_statement { $$ = new StatementNode($1);}
	| selection_statement { $$ = new StatementNode($1);} 
	| iteration_statement { $$ = new StatementNode($1);}
	| jump_statement { $$ = new StatementNode($1);} // return, break, continue only ones implemented
	;


// case and go to
labeled_statement
	: IDENTIFIER COLON statement {}
	| CASE constant_expression COLON statement {$$ = new LabelStatementNode(*$1, $2, *$3, $4);}
	| DEFAULT COLON statement { $$ = new DefualtStatementNode(*$1, $3); }
	;

// block on stuff, recurse function
compound_statement 
	: L_CURL R_CURL {$$ = new CompoundStatementNode(*$1, *$2);}
	| L_CURL statement_list R_CURL {$$ = new CompoundStatementNode(*$1, $2,*$3);}
	| L_CURL declaration_list R_CURL {$$ = new CompoundStatementNode(*$1, $2,*$3);}
	| L_CURL declaration_list statement_list R_CURL {$$ = new CompoundStatementNode(*$1, $2, $3, *$4);}
	;

declaration_list
	: declaration { $$ = new DeclarationListNode($1);}
	| declaration_list declaration
	{
		$$ = $1;
		$$->addChild($2);
	}
	;

statement_list
	: statement { $$ = new StatementListNode($1);}
	| statement_list statement
	{
		$$ = $1;
		$$->addChild($2);
	}
	;

expression_statement
	: SEMI {$$ = new ExpressionStatementNode(*$1);}
	| expression SEMI {$$ = new ExpressionStatementNode($1, *$2);}
	;

selection_statement
	: IF L_BRAC expression R_BRAC statement %prec NOELSE { $$ = new IfNoElseNode(*$1, *$2, $3, *$4, $5);}
	| IF L_BRAC expression R_BRAC statement ELSE statement { $$ = new IfElseNode(*$1, *$2, $3, *$4, $5, *$6, $7);}
	| SWITCH L_BRAC expression R_BRAC statement 
	{
		// trk->allocateMemory( trk->getCurrentScopeId() , 8); // bug in code not enough time to debug// overestimeate but that is ok
		// trk->setScopeAsCaller( trk->getCurrentScopeId() );
		$$ = new SwitchNode(*$1, *$2, $3, *$4, $5);
	} 
	;

iteration_statement
	: WHILE L_BRAC expression R_BRAC statement {$$ = new WhileNode(*$1, *$2, $3, *$4, $5);}
	| DO statement WHILE L_BRAC expression R_BRAC SEMI {} // not done yet
	| FOR L_BRAC expression_statement expression_statement R_BRAC statement {$$ = new ForTwoStatementNode(*$1, *$2, $3, $4,*$5, $6); } // not done yet
	| FOR L_BRAC expression_statement expression_statement expression R_BRAC statement {$$ = new ForThreeStatementNode(*$1, *$2, $3, $4, $5, *$6, $7); } // not done yet
	;

jump_statement
	: GOTO IDENTIFIER SEMI {}
	| CONTINUE SEMI { $$ = new BreakContinueNode(*$1, *$2);}
	| BREAK SEMI { $$ = new BreakContinueNode(*$1, *$2);}
	| RETURN SEMI { $$ = new ReturnNode(*$1, *$2);}
	| RETURN expression SEMI { $$ = new ReturnNode(*$1, $2, *$3);}
	;

program
	: translation_unit { root = $$ = new ProgramNode($1); }
	;

translation_unit
	: external_declaration {$$ = new TranslationUnitNode($1);}
	| translation_unit external_declaration 
	{
		$$ = $1;
		$$->addChild($2);
	}
	;

external_declaration
	: function_definition {$$ = new ExternalDeclarationNode($1);}
	| declaration {$$ = new ExternalDeclarationNode($1);}
	;

// declist not used
function_definition
	: declaration_specifiers declarator declaration_list compound_statement 
	| declaration_specifiers declarator compound_statement 
	{ 
		$$ = new FunctionNode($1, $2, $3); 
		if (trk->isCaller(trk->currentScopeId) == 0)
		{
			trk->setScopeAsCallee(trk->currentScopeId);
		}
		trk->resetScope();
	}
	| declarator declaration_list compound_statement // not used
	| declarator compound_statement
	;

%%

extern char yytext[];
extern int column;

void yyerror(std::string s)
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s.c_str());
}

CommonNode *root; // Definition of variable (to match declaration earlier)

CommonNode *parseAST()
{
  root=0;
  yyparse();
  std::cout << "Parsing complete" << std::endl;
  return root;
}

Tracker *getTracker()
{
	return trk;
}
