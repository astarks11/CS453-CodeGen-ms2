/* File: compile.c
Author: Alex Starks
Path:export LD_LIBRARY_PATH=/home/cs453/spring18/stlib-a/lib*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libstree.h>
// create struct for 3 address code
enum threeAddrType {
	Arg,
	Param,
	Call,
	Retrieve,
	Enter,
	Leave,
	Assignment,
	Add,
	Sub,
	UnarySub,
	Mul,
	Division,
	ArrayLen,
	ArrayAddr,
	ArrayAssg,
	Dest,
	Cond,
	JumpDest,
	LessThan,
	LessThanEq,
	GreaterThan,
	GreaterThanEq,
	NotEq,
	Eq,
};

typedef struct addrCode {
	enum threeAddrType type;
	enum threeAddrType condType;
	int flag; // 1 for const
	symtabnodelist ** local_symtbl;
	symtabnode * src1;
	symtabnode * src2;
	symtabnode * dest;
	char * trueDest;
	char * falseDest;
	char * endDest;
	enum ValueType symType;
	int argSpace;
	int num;
	char * string;
	char ch;
	struct addrCode * next;
	char * functionName;
} addrCode;

/* Function Definition */
void getMethodBody(parser_info * curr_parse);
void traverseFunctionBody(tnode * t, enum threeAddrType type);
void addrCode_methodEnter(tnode * t);
void addrCode_arguments(tnode * t);
void appendToInstructionList(addrCode * newInstr);
void print_threeInstruction(addrCode * head);
void print_assembly(addrCode * code);
void getArgList(symtabnode * t);
void print_global_variables(symtabnodelist ** global_symtbl);
void evaluateCondition(tnode * t);
void conditionDistribute(tnode * t);
void print_local_variables(char * currFun, symtabnodelist ** local_symtbl);
void evaluateWhile(tnode * t);
void codeGen_Bool(tnode * boolExpr, enum SyntaxNodeType type,addrCode * trueDest, addrCode * falseDest);
addrCode * newInstr_Cond_Dest(enum threeAddrType type, char * dest);
addrCode * newInstr_Cond(enum threeAddrType type,enum threeAddrType condType, symtabnode * op1, symtabnode * op2, char * trueDest, char * falseDest);
addrCode * newInstr_Param_Array(enum threeAddrType type, symtabnode * t,symtabnode * subscript);
addrCode * newInstr_Array_Addr(enum threeAddrType type, symtabnode * dest, symtabnode * src1,symtabnode * array);
addrCode * newInstr_Array_Assg(enum threeAddrType type, symtabnode * dest, symtabnode * src1);
addrCode * newInstr_Array_Len(enum threeAddrType type, symtabnode * dest, symtabnode * src1,symtabnode * array);
addrCode * newInstr_Exp(enum threeAddrType type, symtabnode * dest, symtabnode * src1, symtabnode * src2);
addrCode * newInstr(enum threeAddrType type, symtabnode * src1, symtabnode * src2, symtabnode * dest);
addrCode * newInstr_Param_Num(enum threeAddrType type, int val);
addrCode * newInstr_Param_Char(enum threeAddrType type, char val);
addrCode * newInstr_Param_String(enum threeAddrType type, char * val);
addrCode * newInstr_Param_Var(enum threeAddrType type, symtabnode * t);
addrCode * newInstrEnter(enum threeAddrType type, symtabnode * t,symtabnodelist ** local_symtbl);
addrCode * newInstrLeave(enum threeAddrType type, symtabnode * t);
addrCode * newInstrCall(enum threeAddrType type, symtabnode * t);
addrCode * newInstr_Assg_Num(enum threeAddrType type, symtabnode * t, int val);
addrCode * newInstr_Assg_Char(enum threeAddrType type, symtabnode * src1, char val);
addrCode * newInstr_Assg_String(enum threeAddrType type, symtabnode * src1, char * val);
addrCode * newInstr_Assg_Simple(enum threeAddrType type, symtabnode * src1, symtabnode * dest);
addrCode * newInstr_Arg(enum threeAddrType type, symtabnode * t, int size);
void tac_assignments(tnode * t);
symtabnode * tac_assignments_rhs(tnode * t);
void tac_assignments_lhs(tnode * t, symtabnode * smbtbl);
char * temp_create_str();
char * create_str_label(int n);
char * temp_create_label();
/* Variables */
struct addrCode * head = NULL;
symtabnodelist ** global_local_symbtbl;
symtabnodelist ** global_global_symbtbl;
int tmp_counter = 0;
int tmp_counter_lbl = 0;

void funCallArguments(tnode * t);
/* Function: Main
Purpose: initializes the parser. Traverses methods in the given program */
int main() {
	parser_info * curr_parse;
	printf("#");
	int err_occurred = init_parser();
	printf("error:%d\n",err_occurred);

	for (curr_parse = next_fun(); curr_parse != NULL; curr_parse = next_fun()) {
		// create 3 address code for function call
		// get method body
		if (stree_Get_TreeNodeType(curr_parse->syntax_tree) == FunDef) {
			global_local_symbtbl = curr_parse->local_symtbl;
			global_global_symbtbl = curr_parse->global_symtbl;
			getMethodBody(curr_parse);
		}
	}	
	print_threeInstruction(head);
	print_assembly(head);
	return err_occurred;
} // main



/* Function: getMethodBody
Purpose: Gets the function definition body node of the given syntax tree*/
void getMethodBody(parser_info * curr_parse) {

	tnode * t = curr_parse->syntax_tree;
	if (stree_Get_TreeNodeType(t) == FunDef) {
		printf("Function:%s()\n",sym_Get_Name(stree_Get_FunDef_SymTabEntry(t)));

		tnode * method_body = stree_Get_FunDef_Body(t);
		// method_body is pointer to top node in method syntax tree
		// add proper 3addr code to enter method (callee)
		//addrCode_methodEnter(t);
		appendToInstructionList(newInstrEnter(Enter,stree_Get_FunDef_SymTabEntry(t),curr_parse->local_symtbl));
		//get formals
		getArgList(stree_Get_FunDef_SymTabEntry(t));

		traverseFunctionBody(method_body,Enter);
		printf("Leave Function:%s()\n",sym_Get_Name(stree_Get_FunDef_SymTabEntry(t)));
		appendToInstructionList(newInstrLeave(Leave,stree_Get_FunDef_SymTabEntry(t)));
	}
} // getMethodBody


void getArgList(symtabnode * t) {
	int space = 8;
	symtabnodelist * list = sym_Get_FormalsList(t);
	while (list != NULL) {
		if (sym_Get_IsFormal(list->stnode)) {
			switch (sym_Get_Type(list->stnode)) {
				case t_Int:
					printf("Formal Arg(int):%s @ %d\n",sym_Get_Name(list->stnode),space);
					appendToInstructionList(newInstr_Arg(Arg,list->stnode,space));
					space = space+4;
					break;
				case t_Char:
					printf("Formal Arg(int):%s @ %d\n",sym_Get_Name(list->stnode),space);
					appendToInstructionList(newInstr_Arg(Arg,list->stnode,space));
					space = space+4;
					break;
				default:
					break;
			}
		}
		list = list->next;
	}
} // newInstr

void traverseFunctionBody(tnode * t,enum threeAddrType type) {
	if (t == NULL)
		return;

	switch (stree_Get_TreeNodeType(t)) {
		case Error:
			return;
		/* Constants*/
		case Intcon:
			return;
		case Charcon:
			return;
		case Stringcon:
			return;
		/* Variables and array references */
		case Var:
			return;
		case ArraySubscript:
			return;
		/* Arithmetic operators */
		case Plus:
			return;
		case BinaryMinus:
			return;
		case Mult:
			return;
		case Div:
			return;
		case UnaryMinus:
			return;
		/* Comparison operators */
		case Equals:
			return;
		case Neq: 
			return;
		case Leq:
			return;
		case Lt:
			return;
		case Geq:
			return;
		case Gt:
			return;
		/* Logical operators */
		case LogicalAnd:
			return;
		case LogicalOr:
			return;
		case LogicalNot:
			return;
		/* Functions */
		case FunCall:
			// find/set arguments through recursion
			funCallArguments(stree_Get_FunCall_Args(t));
			// set 3addr code for function call
			printf("Fun:%s\n",sym_Get_Name(stree_Get_FunCall_Fun(t)));
			appendToInstructionList(newInstrCall(Call,stree_Get_FunCall_Fun(t)));
			return;
		/* Statements */
		case Assg:
			// there are assignments coming	
			tac_assignments(t);
			return;			
		case Return:
			return;
		case For:
			return;
		case While:
			evaluateWhile(t);
			return;
		case If:
			evaluateCondition(t);
			return;
		case STnodeList:
			while (t != NULL) {
				traverseFunctionBody(stree_Get_List_Head(t),type);
				t = stree_Get_List_Rest(t);
			}
			return;
		default: 
			printf("UNKOWN\n");
			return;
	}
} // 


















void evaluateWhile(tnode * t) {
	tnode * test = stree_Get_While_Test(t);
	tnode * body = stree_Get_While_Body(t);

	addrCode * endDest = newInstr_Cond_Dest(Dest,temp_create_label());
	addrCode * loopDest = newInstr_Cond_Dest(Dest,temp_create_label());
	appendToInstructionList(newInstr_Cond_Dest(Dest,loopDest->endDest));
	codeGen_Bool(test,While,endDest,loopDest);
	conditionDistribute(body);
	appendToInstructionList(newInstr_Cond_Dest(JumpDest,loopDest->endDest));
	appendToInstructionList(endDest);

} // evaluateWhile





















































void evaluateCondition(tnode * t) {
	// deal with condition // codeGen_bool
	printf("--condition--\n");
	tnode * test  = stree_Get_If_Test(t);
	tnode * ifThen = stree_Get_If_Then(t);
	tnode * ifElse = stree_Get_If_Else(t);
	addrCode * trueDest = newInstr_Cond_Dest(Dest,temp_create_label());
	addrCode * falseDest = newInstr_Cond_Dest(Dest,temp_create_label());
	addrCode * endDest = newInstr_Cond_Dest(Dest,temp_create_label());
	// append test to instruction list
	codeGen_Bool(test,If,trueDest,falseDest);
	// deal with if-then
	printf("--ifThen--\n");
	conditionDistribute(ifThen);
	appendToInstructionList(newInstr_Cond_Dest(JumpDest,endDest->endDest));
	appendToInstructionList(falseDest);
	conditionDistribute(ifElse);
	appendToInstructionList(endDest);


} // evaluateCondition



void conditionDistribute(tnode * t) {
	switch (stree_Get_TreeNodeType(t)) {
		case STnodeList:
			while (t != NULL) {
				if (stree_Get_TreeNodeType(stree_Get_List_Head(t)) == Assg) {
					tac_assignments(stree_Get_List_Head(t));
				} 
				if (stree_Get_TreeNodeType(stree_Get_List_Head(t)) == FunCall) {
					// find/set arguments through recursion
					funCallArguments(stree_Get_FunCall_Args(stree_Get_List_Head(t)));
					// set 3addr code for function call
					printf("Fun:%s\n",sym_Get_Name(stree_Get_FunCall_Fun(stree_Get_List_Head(t))));
					appendToInstructionList(newInstrCall(Call,stree_Get_FunCall_Fun(stree_Get_List_Head(t))));
				}
				t = stree_Get_List_Rest(t);

			}
			break;

		default:
			printf("UNEXPECTED\n");
			return;
	}
}




void codeGen_Bool(tnode * boolExpr,enum SyntaxNodeType type, addrCode * trueDest, addrCode * falseDest) {
	addrCode * newCode;
	symtabnode * binop1;
	symtabnode * binop2;
	//tnode * binop11;
	//tnode * binop22;
	//addrCode * newTrueDest = newInstr_Cond_Dest(Dest,temp_create_label());
	switch (stree_Get_TreeNodeType(boolExpr)) {
		case Lt:
			printf("codeGen_Bool:Lt\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			if (type == If) {
				newCode = newInstr_Cond(Cond,LessThan,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
			} else if (type == While) {
				newCode = newInstr_Cond(Cond,GreaterThanEq,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
			}
			break;
		case Leq:
			printf("codeGen_Bool:Leq\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			if (type == If) {
				newCode = newInstr_Cond(Cond,LessThanEq,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
			} else if (type == While) {
				newCode = newInstr_Cond(Cond,GreaterThan,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
			}			
			break;
		case Gt:
			printf("codeGen_Bool:Gt\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			if (type == If) {
				newCode = newInstr_Cond(Cond,GreaterThan,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
			} else if (type == While) {
				newCode = newInstr_Cond(Cond,LessThanEq,binop1,binop2,trueDest->endDest,falseDest->endDest);
				appendToInstructionList(newCode);
			}
			break;
		case Geq:
			printf("codeGen_Bool:Geq\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			newCode = newInstr_Cond(Cond,GreaterThanEq,binop1,binop2,trueDest->endDest,falseDest->endDest);		
			appendToInstructionList(newCode);	
			if (type == If) {
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
			}
		break;
		case Equals:
			printf("codeGen_Bool:Equals\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			if (type == If) {
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
				newCode = newInstr_Cond(Cond,Eq,binop1,binop2,trueDest->endDest,falseDest->endDest);		
				appendToInstructionList(newCode);
			} else if (type == While) {
				newCode = newInstr_Cond(Cond,NotEq,binop1,binop2,trueDest->endDest,falseDest->endDest);		
				appendToInstructionList(newCode);
			}
		break;		
		case Neq:
			printf("codeGen_Bool:Equals\n");
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(boolExpr));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(boolExpr));
			if (type == If) {
				newCode = newInstr_Cond(Cond,NotEq,binop1,binop2,trueDest->endDest,falseDest->endDest);		
				appendToInstructionList(newCode);
				appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
				appendToInstructionList(trueDest);
			} else if (type == While) {
				newCode = newInstr_Cond(Cond,Eq,binop1,binop2,trueDest->endDest,falseDest->endDest);		
				appendToInstructionList(newCode);
			}
		break;
		case LogicalAnd:
		/*	binop11 = stree_Get_Binop_Op1(boolExpr);
			binop22 = stree_Get_Binop_Op2(boolExpr);
			codeGen_Bool(binop11,trueDest,falseDest);
			appendToInstructionList(trueDest);
			codeGen_Bool(binop22,newTrueDest,falseDest);
			appendToInstructionList(newInstr_Cond_Dest(JumpDest,falseDest->endDest));
			appendToInstructionList(falseDest);*/
			break;
		default:
			printf("NOT EXCPECTED\n");
			break;
	}
} // codeGen_Bool





addrCode * newInstr_Cond_Dest(enum threeAddrType type, char * dest) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->endDest = dest;
	newCode->next = NULL;
	return newCode;
}


addrCode * newInstr_Cond(enum threeAddrType type,enum threeAddrType condType, symtabnode * op1, symtabnode * op2, char * trueDest, char * falseDest) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->condType = condType;
	newCode->src1 = op1;
	newCode->src2 = op2;
	newCode->trueDest = trueDest;
	newCode->falseDest = falseDest;
	newCode->next = NULL;
	return newCode;
} // newInstr




















































































void tac_assignments(tnode * t) {
	// deal with rhs
	symtabnode * rhsPtr = tac_assignments_rhs(stree_Get_Assg_Rhs(t));
	// deal with lhs
	tac_assignments_lhs(stree_Get_Assg_Lhs(t),rhsPtr);
	// marry the two
} // tac_assignments

int isLastAnd = 0;
symtabnode * tac_assignments_rhs(tnode * t) {
	symtabnode * symtabptr = NULL;
	symtabnode * binop1;
	symtabnode * binop2;
	addrCode * newCode;
	printf("...RHS...\n");
	switch (stree_Get_TreeNodeType(t)) {
		case Charcon:
			printf("Assg RHS Charcon: %c\n",stree_Get_Charcon(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1); // 1 for local scope
			newCode = newInstr_Assg_Char(Assignment,symtabptr,stree_Get_Charcon(t));
			appendToInstructionList(newCode);
			break;
		case Intcon:
			printf("Assg RHS Intcon: %d\n",stree_Get_Intcon(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1); // 1 for local scope
			newCode = newInstr_Assg_Num(Assignment,symtabptr,stree_Get_Intcon(t));
			appendToInstructionList(newCode);
			break;
		case Stringcon:
			printf("Assg RHS Stringcon: %s\n",stree_Get_Stringcon(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1); // 1 for local scope
			newCode = newInstr_Assg_String(Assignment,symtabptr,stree_Get_Stringcon(t));
			appendToInstructionList(newCode);
			break;
		case Var:
			symtabptr = stree_Get_Var(t);
			break;
		case Plus:
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(t));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Exp(Add,symtabptr,binop1,binop2);
			appendToInstructionList(newCode);
			break;
		case BinaryMinus:
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(t));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Exp(Sub,symtabptr,binop1,binop2);
			appendToInstructionList(newCode);
			break;
		case Mult:
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(t));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Exp(Mul,symtabptr,binop1,binop2);
			appendToInstructionList(newCode);
			break;
		case Div:
			binop1 = tac_assignments_rhs(stree_Get_Binop_Op1(t));
			binop2 = tac_assignments_rhs(stree_Get_Binop_Op2(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Exp(Division,symtabptr,binop1,binop2);
			appendToInstructionList(newCode);
			break;
		case UnaryMinus:
			binop1 = tac_assignments_rhs(stree_Get_Unop_Op(t));
			symtabptr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Exp(UnarySub,symtabptr,binop1,NULL);
			appendToInstructionList(newCode);
			break;
		case ArraySubscript:
			printf("still needs to be done\n");
		default:
			return symtabptr;
	}
	return symtabptr;
} // tac_assignments_rhs
void tac_assignments_lhs(tnode * t,symtabnode * smbtbl) {
	addrCode * newCode;
	symtabnode * symtabptr;
	symtabnode * derefLength;
	symtabnode * addr;

	printf("...LHS...\n");
	switch(stree_Get_TreeNodeType(t)) {
		case Var:
			printf("newAssg:%s = %s\n",sym_Get_Name(stree_Get_Var(t)),sym_Get_Name(smbtbl));
			newCode = newInstr_Assg_Simple(Assignment,smbtbl,stree_Get_Var(t));
			appendToInstructionList(newCode);
			break;
		case ArraySubscript:
			// evaluate subscript
			symtabptr = tac_assignments_rhs(stree_Get_ArraySubscript_Subscript(t));
			// get deref length
			derefLength = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Array_Len(ArrayLen,derefLength,symtabptr,stree_Get_ArraySubscript_Array(t));
			appendToInstructionList(newCode);
			// get addr loc
			addr = SymTab_Insert(global_local_symbtbl,temp_create_str(),1);
			newCode = newInstr_Array_Addr(ArrayAddr,addr,derefLength,stree_Get_ArraySubscript_Array(t));
			appendToInstructionList(newCode);
			// assign
			newCode = newInstr_Array_Assg(ArrayAssg,addr,smbtbl);
			appendToInstructionList(newCode);
			break;
		default:
			return;
	}
} // tac_assignments_lhs


addrCode * newInstr_Array_Assg(enum threeAddrType type, symtabnode * dest, symtabnode * src1) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->dest = dest;
	newCode->next = NULL;
	return newCode;
} // newInstr

addrCode * newInstr_Array_Addr(enum threeAddrType type, symtabnode * dest, symtabnode * src1,symtabnode * array) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = array;
	newCode->src2 = src1;
	newCode->dest = dest;
	newCode->next = NULL;
	return newCode;
} // newInstr


addrCode * newInstr_Exp(enum threeAddrType type, symtabnode * dest, symtabnode * src1, symtabnode * src2) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->src2 = src2;
	newCode->dest = dest;
	newCode->next = NULL;
	return newCode;

} // newInstr

addrCode * newInstr_Array_Len(enum threeAddrType type, symtabnode * dest, symtabnode * src1,symtabnode * array) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->dest = dest;
	if (sym_Get_ArrayElemType(array) == t_Int)
		newCode->flag = 4; // size of indiv elem
	newCode->next = NULL;
	return newCode;

} // newInstr






void funCallArguments(tnode * t) {
	while (t != NULL) {
		tnode * tmp = stree_Get_List_Head(t);
		switch(stree_Get_TreeNodeType(tmp)) {
			// add node to list that will be traversed to create argument 
			// assembly code out of it.
			case Intcon:
				printf("param Intcon:%d\n",stree_Get_Intcon(tmp)); 
				appendToInstructionList(newInstr_Param_Num(Param,stree_Get_Intcon(tmp)));
				break;
			case Stringcon:
				printf("param Stringcon:%s\n",stree_Get_Stringcon(tmp));
				appendToInstructionList(newInstr_Param_String(Param,stree_Get_Stringcon(tmp)));
				break;
			case Charcon:
				printf("param Charcon:%c\n",stree_Get_Charcon(tmp));
				appendToInstructionList(newInstr_Param_Char(Param,stree_Get_Charcon(tmp)));
				break;
			case Var:
				printf("param Var:%s\n",sym_Get_Name(stree_Get_Var(tmp)));
				// create new instr var param
				// get symtabnode for var
				appendToInstructionList(newInstr_Param_Var(Param,stree_Get_Var(tmp)));
				break;
			case ArraySubscript:
				printf("param Array DOESNOT WORK:%s\n",sym_Get_Name(stree_Get_ArraySubscript_Array(tmp)));
				//appendToInstructionList(newInstr_Param_Array(Param,stree_Get_ArraySubscript_Array(tmp),tac_assignments_rhs(stree_Get_ArraySubscript_Subscript(tmp))));

				break;
			default:
				printf("ERROR: NO USE FOR FUNCTION ARGUMENT\n");
				return;
		}
		t = stree_Get_List_Rest(t);
	}
} // funCallArguments

void appendToInstructionList(addrCode * newInstr) {
	addrCode * curr = head;
	addrCode ** ref = &head;
	if (curr == NULL) {
		curr = newInstr;
		*(ref) = curr;
		printf("created new instr head\n");
	} else {
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = newInstr;
		printf("appended new instr\n");
	}
} // appendToInstructionList

void addrCode_functionCall(tnode * t) {

} // addrCode_functionCall
	
void addrCode_arguments(tnode * args) {
	if (stree_Get_TreeNodeType(args) == STnodeList) {
		while (args != NULL) {
			//printf("args:%s\n",stree);
			args = stree_Get_List_Rest(args);
		}
	}
} // addrCode_arguments
// NOTE: ASSUMES THIS IS THE FIRST TIME WE HAVE SEEN THIS FUNCTION
void addrCode_methodEnter(tnode * t) {
	struct addrCode * newCode = (addrCode *)malloc(sizeof(addrCode));
	newCode->type = stree_Get_TreeNodeType(t);
	newCode->dest = stree_Get_FunDef_SymTabEntry(t);
	stree_Set_AuxInfo(t,newCode);
	printf("Set: (%s)'s aux info - FunDef\n",sym_Get_Name(stree_Get_FunDef_SymTabEntry(t)));
} // addrCode_methodEnter


char * temp_create_str() {
	char * ptr = (char*)malloc(sizeof(char)*25);
	snprintf(ptr,sizeof(&ptr),"tmp%d_",tmp_counter++);
	printf("newTemp:%s\n",ptr);
	return ptr;
} // temp_create_str

char * temp_create_label() {
	char * ptr = (char*)malloc(sizeof(char)*25);
	snprintf(ptr,sizeof(&ptr),"L%d_",tmp_counter_lbl++);
	printf("newTempLabel:%s\n",ptr);
	return ptr;
} // temp_create_str














addrCode * newInstr(enum threeAddrType type, symtabnode * src1, symtabnode * src2, symtabnode * dest) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->flag = 0;
	newCode->src1 = src1;
	newCode->src2 = src2;
	newCode->dest = dest;
	newCode->next = NULL;
	return newCode;

} // newInstr
addrCode * newInstr_Param_Num(enum threeAddrType type, int val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->flag = 1;
	newCode->num = val;
	newCode->next = NULL;
	return newCode;
}
addrCode * newInstr_Param_Char(enum threeAddrType type, char val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->flag = 2;
	newCode->ch = val;
	newCode->next = NULL;
	return newCode;
}
addrCode * newInstr_Param_String(enum threeAddrType type, char * val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->flag = 3;
	newCode->string = val;
	newCode->next = NULL;
	return newCode;
}
addrCode * newInstr_Param_Var(enum threeAddrType type, symtabnode * t) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->symType = sym_Get_Type(t);
	newCode->src1 = t;
	newCode->flag = 4;
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Param_Array(enum threeAddrType type, symtabnode * t,symtabnode * subscript) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->symType = sym_Get_Type(t);
	newCode->src1 = t;
	newCode->src2 = subscript;
	newCode->flag = 4;
	newCode->next = NULL;
	return newCode;
}
addrCode * newInstrEnter(enum threeAddrType type, symtabnode * functionSymbolTable,symtabnodelist ** local_symtbl) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->local_symtbl = local_symtbl;
	newCode->functionName = sym_Get_Name(functionSymbolTable);
	newCode->src1 = functionSymbolTable;
	newCode->next = NULL;
	return newCode;	
}
addrCode * newInstrLeave(enum threeAddrType type, symtabnode * functionSymbolTable) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->functionName = sym_Get_Name(functionSymbolTable);
	newCode->next = NULL;
	return newCode;	
}
addrCode * newInstrCall(enum threeAddrType type, symtabnode * t) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->functionName = sym_Get_Name(t);
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Assg_Num(enum threeAddrType type, symtabnode * src1, int val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	sym_Set_Type(src1,t_Int);
	newCode->flag = 1;
	newCode->num = val;
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Assg_Char(enum threeAddrType type, symtabnode * src1, char val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->flag = 2;
	sym_Set_Type(src1,t_Char);
	newCode->ch = val;
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Assg_String(enum threeAddrType type, symtabnode * src1, char * val) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->flag = 3;
	newCode->string = val;
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Assg_Simple(enum threeAddrType type, symtabnode * src1, symtabnode * dest) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->type = type;
	newCode->src1 = src1;
	newCode->dest = dest;
	newCode->flag = 4;
	newCode->next = NULL;
	return newCode;		
}
addrCode * newInstr_Arg(enum threeAddrType type, symtabnode * t, int size) {
	struct addrCode * newCode = (addrCode*)malloc(sizeof(addrCode));
	newCode->argSpace = size;
	newCode->type = type;
	newCode->src1= t;
	newCode->next = NULL;
	return newCode;
}


void print_threeInstruction(addrCode * code) {
	printf("\n----------------------- 3 addr code -----------------------\n");
	while (code != NULL) {
		switch(code->type) {
			case Enter:
				printf("Enter _%s\n",code->functionName);
				break;
			case Leave:
				printf("Leave _%s\n",code->functionName);
				break;
			case Call:
				printf("Call _%s\n",code->functionName);
				break;
			case Param:
				if (code->flag == 1) {
					printf("Param %d\n",code->num);
				} else if (code->flag == 2) {
					printf("Param %c\n",code->ch);
				} else if (code->flag == 3) {
					printf("Param %s\n",code->string);
				} else if (code->flag == 4) {
					printf("Param %s\n",sym_Get_Name(code->src1));
				} else {}
			break;
			case Assignment:
				if (code->flag == 1) {
					printf("%s = %d\n",sym_Get_Name(code->src1),code->num);
				} else if (code->flag == 2) {
					printf("%s = %c\n",sym_Get_Name(code->src1),code->ch);
				} else if (code->flag == 3) {
					printf("%s = %s\n",sym_Get_Name(code->src1),code->string);
				} else if (code->flag == 4) {
					printf("%s = %s\n",sym_Get_Name(code->dest),sym_Get_Name(code->src1));
				}
				break;
			case Arg: 
				printf("Arg %s @ %d\n",sym_Get_Name(code->src1),code->argSpace);
				break;
			default:

				break;
		}
		code = code->next;
	}
} // print_threeInstruction


void print_assembly(addrCode * code) {
int strLabelCount = 0;
char * currFun;
symtabnodelist ** local_symtbl;
printf("\n----------------------- assembly -----------------------\n");
printf(".text\n");
while (code != NULL) {
		switch(code->type) {
			case Enter:
				currFun = code->functionName;
				local_symtbl = code->local_symtbl;
				printf("_%s:\n",code->functionName);
				printf("\tla $sp, -8($sp)\n");
				printf("\tsw $fp, 4($sp)\n");
				printf("\tsw $ra, 0($sp)\n");
				printf("\tla $fp, 0($sp)\n");
				printf("\tla $sp, -4($sp)\n");
				break;
			case Leave:
				printf("\tla $sp, 0($fp)\n");
				printf("\tlw $ra, 0($sp)\n");
				printf("\tlw $fp, 4($sp)\n");
				printf("\tla $sp, 8($sp)\n");
				printf("\tjr $ra\n");
				printf("\n\n");
				print_local_variables(currFun,local_symtbl);
				break;
			case Call:
				printf("\tjal _%s\n",code->functionName);
				printf("\tla $sp, 4($sp)\n");
				break;
			case Param:
				if (code->flag == 1) {
					printf("\tla $t0, %d\n",code->num);
					printf("\tla $sp, -4($sp)\n");
					printf("\tsw $t0, 0($sp)\n");
				} else if (code->flag == 2) {
					if (code->ch == '\n')  {
						printf("\tli $t0, 0x0A\n");
					} else {
						printf("\tli $t0, '%c'\n",code->ch);						
					}
					printf("\tla $sp, -4($sp)\n");
					printf("\tsw $t0, 0($sp)\n");
				} else if (code->flag == 3) {
					char * labelPtr = create_str_label(strLabelCount++);
					printf("\tla $t0, _%s\n",labelPtr);
					printf("\tla $sp, -4($sp)\n");
					printf("\tsw $t0, 0($sp)\n");
					printf("\n");
					printf(".data\n");
					printf("_%s: .asciiz \"%s\"\n",labelPtr,code->string);
					printf(".text\n");
				} else if (code->flag == 4) {
					// needs to be checked for variable type. (different instructions)
					if (code->symType == t_Int) {
						if (sym_Get_Scope(code->src1) == 1)
							printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
						else
							printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));
						printf("\tla $sp, -4($sp)\n");
						printf("\tsw $t0, 0($sp)\n");						
					} else if (code->symType == t_Char) { 
						if (sym_Get_Scope(code->src1) == 1) 
							printf("\tlb $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1)); // test 23
						else 
							printf("\tlb $t0, _%s\n",sym_Get_Name(code->src1)); // test 23

						printf("\tla $sp, -4($sp)\n");
						printf("\tsw $t0, 0($sp)\n");	
					} else if (code->symType == t_Array) {
						printf("needs work\n");
						printf("\tlw $t0,_%s\n",sym_Get_Name(code->src1));
						printf("\tla $sp, -4($sp)\n");
						printf("\tsw $t0, 0($sp)\n");

					} else {
						printf("default,bad\n");
						printf("\tla $t0, -4($fp)\n");
						printf("\tla $sp, -4($sp)\n");
						printf("\tsw $t0, 0($sp)\n");
					}
				} else {}
			break;
			case Assignment:
				if (code->flag == 1) {
					printf("\tli $t0, %d\n",code->num);
					printf("\tsw $t0, -4($fp)\n");
					printf("\tsw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));

				} else if (code->flag == 2) {
					if (code->ch == '\n') {
						printf("\tli $t0, 0x0A\n");
					} else {
						printf("\tli $t0, '%c'\n",code->ch);
					}
					printf("\tsw $t0, -4($fp)\n");
					printf("\tsw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));

				} else if (code->flag == 3) {
					// dont have to worry about? for micro C
				} else if (code->flag == 4) {
					// var
					if (sym_Get_Type(code->src1) == t_Int && sym_Get_Type(code->dest) == t_Char) {
						if (sym_Get_Scope(code->src1) == 1)
							printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src1));
						else 
							printf("\tlw $t1, _%s\n",sym_Get_Name(code->src1));

						if (sym_Get_Scope(code->dest) == 1)
							printf("\tsb $t1 _%s_%s\n",currFun,sym_Get_Name(code->dest));
						else
							printf("\tsb $t1 _%s\n",sym_Get_Name(code->dest));

					} else if (sym_Get_Type(code->src1) == t_Char && sym_Get_Type(code->dest) == t_Int) {
						if (sym_Get_Scope(code->src1) == 1)
							printf("\tlb $t1, _%s_%s\n",currFun,sym_Get_Name(code->src1));
						else
							printf("\tlb $t1, _%s\n",sym_Get_Name(code->src1));

						if (sym_Get_Scope(code->dest) == 1)
	        				printf("\tsw $t1, _%s_%s\n",currFun,sym_Get_Name(code->dest));
	        			else
	        				printf("\tsw $t1, _%s\n",sym_Get_Name(code->dest));
					} else if (sym_Get_Type(code->src1) == t_Char) {
						printf("\tlw $t1, -4($fp)\n");
						printf("\tsb $t1, _%s\n",sym_Get_Name(code->dest));
					} else {
						if (sym_Get_Scope(code->src1) == 1) {
							printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src1));
						} else {
							printf("\tlw $t1, _%s\n",sym_Get_Name(code->src1));
						}
						if (sym_Get_Scope(code->dest) == 1) {
							printf("\tsw $t1, _%s_%s\n",currFun,sym_Get_Name(code->dest));
						} else {
							printf("\tsw $t1, _%s\n",sym_Get_Name(code->dest));
						}
					}
				}
				break;
			case Arg:
				if (sym_Get_Type(code->src1) == t_Int) {
					printf("\tlw $t0, %d($fp)\n",code->argSpace);
					if (sym_Get_Scope(code->src1) == 1)
						printf("\tsw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
					else
						printf("\tsw $t0, _%s\n",sym_Get_Name(code->src1));
				}
				if (sym_Get_Type(code->src1) == t_Char) {
					printf("\tlw $t0, %d($fp)\n",code->argSpace);
					if (sym_Get_Scope(code->src1) == 1)
						printf("\tsb $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
					else
						printf("\tsb $t0, _%s\n",sym_Get_Name(code->src1));
				}
				break;
			case Add:
				if (sym_Get_Scope(code->src1) == 1) 
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else 
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));

				if (sym_Get_Scope(code->src2) == 1)
					printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				else 
					printf("\tlw $t1, _%s\n",sym_Get_Name(code->src2));

				printf("\tadd $t3, $t0, $t1\n");
				if (sym_Get_Scope(code->dest) == 1)
					printf("\tsw $t3, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				else 
					printf("\tsw $t3, _%s\n",sym_Get_Name(code->dest));
				break;
			case Sub:
				if (sym_Get_Scope(code->src1) == 1) 
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else 
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));

				if (sym_Get_Scope(code->src2) == 1)
					printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				else 
					printf("\tlw $t1, _%s\n",sym_Get_Name(code->src2));

				printf("\tsub $t3, $t0, $t1\n");
				if (sym_Get_Scope(code->dest) == 1)
					printf("\tsw $t3, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				else 
					printf("\tsw $t3, _%s\n",sym_Get_Name(code->dest));
				break;
			case UnarySub:
				if (sym_Get_Scope(code->src1) == 1) 
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else 
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));
				printf("\tli $t1, 0\n");
				printf("\tsub $t3, $t1, $t0\n");
				if (sym_Get_Scope(code->dest) == 1)
					printf("\tsw $t3, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				else 
					printf("\tsw $t3, _%s\n",sym_Get_Name(code->dest));
				break;
			case Mul:
				if (sym_Get_Scope(code->src1) == 1) 
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else 
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));

				if (sym_Get_Scope(code->src2) == 1)
					printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				else 
					printf("\tlw $t1, _%s\n",sym_Get_Name(code->src2));

				printf("\tmul $t3, $t0, $t1\n");
				if (sym_Get_Scope(code->dest) == 1)
					printf("\tsw $t3, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				else 
					printf("\tsw $t3, _%s\n",sym_Get_Name(code->dest));
				break;
			case Division:
				if (sym_Get_Scope(code->src1) == 1) 
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else 
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));

				if (sym_Get_Scope(code->src2) == 1)
					printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				else 
					printf("\tlw $t1, _%s\n",sym_Get_Name(code->src2));

				printf("\tdiv $t3, $t0, $t1\n");
				if (sym_Get_Scope(code->dest) == 1)
					printf("\tsw $t3, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				else 
					printf("\tsw $t3, _%s\n",sym_Get_Name(code->dest));
				break;
			case ArrayLen:
							printf("array\n");
				printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				printf("\tli $t4, %d\n",code->flag);
				printf("\tmul $t1, $t0, $t4\n");
				printf("\tsw $t1, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				break;
			case ArrayAddr:
				if (sym_Get_Scope(code->src1) == 1)
					printf("\tla $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else
					printf("\tla $t0, _%s\n",sym_Get_Name(code->src1));

				printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				printf("\tadd $t2, $t1, $t0\n");
				printf("\tsw $t2, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				break;
			case ArrayAssg:
				printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->dest));
				printf("\tlw $t2, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				printf("\tsw $t2, 0($t1)\n");
				break;
			case Cond:
				if (sym_Get_Scope(code->src1) == 1)
					printf("\tlw $t0, _%s_%s\n",currFun,sym_Get_Name(code->src1));
				else
					printf("\tlw $t0, _%s\n",sym_Get_Name(code->src1));
				if (sym_Get_Scope(code->src2) == 1)
					printf("\tlw $t1, _%s_%s\n",currFun,sym_Get_Name(code->src2));
				else
					printf("\tlw $t1, _%s\n",sym_Get_Name(code->src2));
				if (code->condType == LessThan)
					printf("\tblt $t0, $t1, _%s\n",code->trueDest);
				else if (code->condType == GreaterThan)
					printf("\tbgt $t0, $t1, _%s\n",code->trueDest);
				else if (code->condType == GreaterThanEq)
					printf("\tbge $t0, $t1, _%s\n",code->trueDest);
				else if (code->condType == LessThanEq)
					printf("\tble $t0, $t1, _%s\n",code->trueDest);
				else if (code->condType == Eq)
					printf("\tbeq $t0, $t1, _%s\n",code->trueDest);
				else if (code->condType == NotEq)
					printf("\tbne $t0, $t1, _%s\n",code->trueDest);

				break;
			case Dest:
				printf("\t_%s:\n",code->endDest);
				break;
			case JumpDest:
				printf("\tj _%s\n",code->endDest);
				break;
			default:
			return;
		}
		printf("\n");
		code = code->next;
	}
	// call main
	printf("main:\n");
	printf("\tj _main\n");
	printf("\n");
	printf("_print_int:\n");
	printf("\tli $v0, 1\n");
	printf("\tlw $a0, 0($sp)\n");
	printf("\tsyscall\n");
	printf("\tjr $ra\n");
	printf("\n");
	printf("_print_string:\n");
	printf("\tli $v0, 4\n");
	printf("\tlw $a0, 0($sp)\n");
	printf("\tsyscall\n");
	printf("\tjr $ra\n");


	// print data 
	print_global_variables(global_global_symbtbl);






} // print_assembly


char * create_str_label(int n) {
	char * ptr = (char*)malloc(sizeof(char)*25);
	snprintf(ptr,sizeof(&ptr),"StrLbl%d_",n);
	return ptr;
} // temp_create_str


void print_global_variables(symtabnodelist ** global_symtbl) {
	printf(".data\n");
	symtabnodelist * list = sym_Get_AllEntries(global_symtbl);
	while (list != NULL) {
		symtabnode * node = list->stnode;
		//printf("symItem:%s\n",sym_Get_Name(node));

		switch(sym_Get_Type(node)) {
			case t_Int:
				printf(".align 2\n");
				printf("_%s: .space 4\n",sym_Get_Name(node));
				break;
			case t_Char:
				printf(".align 2\n");
				printf("_%s: .space 4\n",sym_Get_Name(node));
				break;
			case t_Array:
				printf(".align 2\n");
				if (sym_Get_ArrayElemType(node) == t_Int)
					printf("_%s: .space %d\n",sym_Get_Name(node),(sym_Get_ArraySize(node)*4));
				if (sym_Get_ArrayElemType(node) == t_Char)
					printf("_%s: .space %d\n",sym_Get_Name(node),(sym_Get_ArraySize(node)*1));

				break;	
			default:
				break;
		}
		list = list->next;
	}
} // print_global_variables


void print_local_variables(char * currFun, symtabnodelist ** local_symtbl) {
	printf(".data\n");
	symtabnodelist * list = sym_Get_AllEntries(local_symtbl);
	while (list != NULL) {
		symtabnode * node = list->stnode;
		//printf("symItem:%s\n",sym_Get_Name(node));

		switch(sym_Get_Type(node)) {
			case t_Int:
				printf(".align 2\n");
				printf("_%s_%s: .space 4\n",currFun,sym_Get_Name(node));
				break;
			case t_Char:
				printf(".align 2\n");
				printf("_%s_%s: .space 4\n",currFun,sym_Get_Name(node));
				break;
			default:
				break;
		}
		list = list->next;
	}
	printf(".text\n");
} // print_local_variables