/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"

Token *currentToken;

// Token {
//   char string[MAX_IDENT_LEN + 1];
//   int lineNo, colNo;
//   TokenType tokenType;
//   int value;
// };

Token *lookAhead;

extern Type* intType;
extern Type* charType;
extern SymTab* symtab;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead; // Gán lại currentToken cho chuẩn
  lookAhead = getValidToken(); // Tìm Token mới
  free(tmp); // Hủy currentToken cũ
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void) {
  Object* program;

  // Prog ::= KW_PROGRAM Ident SB_SEMICOLON Block SB_PERIOD

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string); // Tạo đối tượng chương trình với tên là currentToken (hiện tại là Ident)

  // program {
  //   char name[MAX_IDENT_LEN]= Ident;
  //   enum ObjectKind kind = OBJ_PROGRAM;
  //   progAttrs->scope {
  //       ObjectNode *objList = NULL;
  //       Object *owner = program;
  //       struct Scope_ *outer = NULL;
  //   }
  // }

  // symtab {
  //   Object* program = program;
  //   Scope* currentScope = NULL;
  //   ObjectNode *globalObjectList;
  // }

  enterBlock(program->progAttrs->scope); // Gán currentScope

  //   currentScope {
  //       ObjectNode *objList = NULL;
  //       Object *owner = program;
  //       struct Scope_ *outer = NULL;
  //   }

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  exitBlock();
}

// Block ::= KW_CONST ConstDecl ConstDecls Block2
// Block ::= Block2
// currentToken = SB_SEMICOLON
// lookAhead = KW_CONST _OR_ KW_TYPE (Block2)
void compileBlock(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) { // Nếu có khai báo hằng
    eat(KW_CONST);
    // ConstDecl ::= Ident SB_EQUAL Constant SB_SEMICONLON
    // currentToken = KW_CONST
    // lookAhead = TK_IDENT (Ident)
    do {
      eat(TK_IDENT);
      // ConstDecls = ConstDecl ConstDecls  _OR_ ConstDecls = empty
      // currentToken = TK_IDENT (Ident)
      // lookAhead = <tùy theo chương trình khai báo 1 hay nhiều hằng>

      // Check if a constant identifier is fresh in the block ~ Kiểm tra xem mã định danh hằng số có mới trong khối hay không
      checkFreshIdent(currentToken->string);

      // Nếu định danh hợp lệ -> continue
      // Create a constant object ~ Tạo đối tượng hằng
      constObj = createConstantObject(currentToken->string);
      
      eat(SB_EQ);

      // currentToken = SB_EQ
      // lookAhead = (Constant)
      // Get the constant value ~ Tạo giá trị hằng cho định danh
      constValue = compileConstant();
      constObj->constAttrs->value = constValue; // Gán vào đối tượng hằng
      // Declare the constant object ~ Khai báo đối tượng hằng
      declareObject(constObj);

      // symtab {
      //   Object* program = program;
      //   Scope* currentScope =  {
      //       ObjectNode *objList = constObj (chuyển sang kiểu ObjectNode);
      //       Object *owner = program;
      //       struct Scope_ *outer = NULL;
      //   }
      //   ObjectNode *globalObjectList;
      // }
      
      eat(SB_SEMICOLON);
      // currentToken = SB_SEMICOLON
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock2(); // Sau đó compile Block2
  } 
  else compileBlock2(); // Không có khai báo hằng thì compile Block2
}

// Block2 ::= KW_TYPE TypeDecl TypeDecls Block3
// Block2 ::= Block3
// currentToken = Sb_SEMICOLON
// lookAhead = KW_TYPE
void compileBlock2(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) { // Nếu có khai báo kiểu
    eat(KW_TYPE);
    // TypeDecl ::= Ident SB_EQUAL Type SB_SEMICOLON
    // currentToken = KW_TYPE
    // lookAhead = TK_IDENT (Ident)
    do {
      eat(TK_IDENT);
      // Check if a type identifier is fresh in the block
      checkFreshIdent(currentToken->string);
      // create a type object
      typeObj = createTypeObject(currentToken->string);
      
      eat(SB_EQ);
      // Get the actual type
      actualType = compileType();
      typeObj->typeAttrs->actualType = actualType;
      // Declare the type object
      declareObject(typeObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3(); // Sau đó compile Block3
  } 
  else compileBlock3(); // Không có khai báo kiểu thì compile Block3
}

// Block3 ::= KW_VAR VarDecl VarDecls Block4
// Block3 ::= Block4
// currentToken = SB_SEMICOLON
// lookAhead = KW_VAR
void compileBlock3(void) {
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) { // Nếu có khai báo biến
    eat(KW_VAR);
    // VarDecl ::= Ident SB_COLON Type SB_SEMICOLON
    // currentToken = KW_VAR
    // lookAhead = TK_IDENT (Ident)
    do {
      eat(TK_IDENT);
      // Check if a variable identifier is fresh in the block
      checkFreshIdent(currentToken->string);
      // Create a variable object      
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      // Get the variable type
      varType = compileType();
      varObj->varAttrs->type = varType;
      // Declare the variable object
      declareObject(varObj);
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
    
    compileBlock4(); // Sau đó compile Block4
  } 
  else compileBlock4(); // Không có khai báo biến thì compile Block4
}
// symtab {
//   Object* program = program;
//   Scope* currentScope = program {
//       ObjectNode *objList = 1 danh sách các đối tượng hằng, kiểu, biến được khai báo toàn cục (chuyển sang kiểu ObjectNode);
//       Object *owner = program;
//       struct Scope_ *outer = NULL;
//   }
//   ObjectNode *globalObjectList;
// }

// Block4 ::= SubDecls Block5 (khai báo hàm, thủ tục)
// Block4 ::= Block5
void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl(); // compile hàm
    else compileProcDecl(); // compile thủ tục
  }
}

// FunDecl ::= KW_FUNCTION Ident Params SB_COLON BasicType SB_SEMICOLON Block SB_SEMICOLON
void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  // currentToken = KW_FUNCTION
  // lookAhead = TK_IDENT
  eat(TK_IDENT);
  // currenrToken = TK_IDENT
  // Params ::= SB_LPAR Param Params2 SB_RPAR
  // Params ::= _rong_
  // lookAhead = SB_LPAR

  // Check if a function identifier is fresh in the block ~ Kiểm tra xem định danh hàm có mới không
  checkFreshIdent(currentToken->string);

  // Nếu định danh hợp lệ -> continue
  // create the function object ~ Tạo đối tượng hàm
  funcObj = createFunctionObject(currentToken->string);
  // Ident {
  //   name = Ident
  //   kind = OBJ_FUNCTION
  //   funcAttrs = {
  //     paramList = NULL
  //     returnType = NULL
  //     scope = {
  //       objList = NULL
  //         owner = Ident
  //         outer = program
  //     }
  //   }
  // }
  // declare the function object ~ Khai báo đối tượng hàm
  declareObject(funcObj);
  // enter the function's block 
  enterBlock(funcObj->funcAttrs->scope); // Gán phạm vi hiện tại của bảng kí tự bằng phạm vi của đối tượng hàm vừa khai báo
  // parse the function's parameters ~ Phân tích cú pháp các tham số của hàm
  compileParams();
  eat(SB_COLON);
  // currentToken = SB_COLON
  // lookAhead = BasicType
  // get the funtion's return type
  returnType = compileBasicType(); // return TP_INT _OR_ TP_CHAR
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock(); // compile thân hàm
  eat(SB_SEMICOLON);
  // exit the function block
  exitBlock(); // Chuyển phạm vi hiện tại trong bản ký tự (symtab->currentScope) ra phạm vi bên ngoài (symtab->currentScope->outer)
}

// ProcDecl ::= KW_PROCEDURE Ident Params SB_SEMICOLON Block SB_SEMICOLON
void compileProcDecl(void) {
  // TODO: check if the procedure identifier is fresh in the block
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);
  // Check if a procedure identifier is fresh in the block
  checkFreshIdent(currentToken->string);
  // create a procedure object
  procObj = createProcedureObject(currentToken->string);
  // declare the procedure object
  declareObject(procObj);
  // enter the procedure's block
  enterBlock(procObj->procAttrs->scope);
  // parse the procedure's parameters
  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  // exit the block
  exitBlock();
}

ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the constant identifier is declared ~ Tên 
    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue* compileConstant(void) {
  ConstantValue* constValue;
  // Constant ::= SB_PLUS Constant2
  // Constant ::= SB_MINUS Constant2
  // Constant ::= Constant2
  // Constant ::= ConstChar

  // Constant2 ::= ConstantIdent
  // Constant2 ::= Number
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    // currentToken = SB_PLUS
    // lookAhead = TK_IDENT (ConstantIdent) _OR_ TK_NUMBER (Number) | (Constant2)
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    // currentToken = SB_MINUS
    // lookAhead = TK_IDENT (ConstantIdent) _OR_ TK_NUMBER (Number) | (Constant2)
    constValue = compileConstant2();
    constValue->intValue = - constValue->intValue; // Khi const a = -b
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    // currentToken = TK_CHAR
    constValue = makeCharConstant(currentToken->string[0]); // Chỉ lấy kí tự đầu tiên
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    // currentToken = TK_NUMBER
    constValue = makeIntConstant(currentToken->value); // Khởi tạo giá trị hằng kiểu int
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // currentToken = TK_IDENT
    // check if the integer constant identifier is declared ~ kiểm tra xem định danh hằng số nguyên có được khai báo hay không
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value); // Tạo hằng có giá trị tương đương
    else
      error(ERR_UNDECLARED_INT_CONSTANT,currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type* compileType(void) {
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER);
    type =  makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the type idntifier is declared
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) { // Nếu có tham số thì compile tham số
    eat(SB_LPAR);
    // Param ::= Ident SB_COLON BasicType
    // Param ::= KW_VAR Ident SB_COLON BasicType
    // currentToken = SB_LPAR
    // lookAhead = TK_IDENT (Ident) _OR_ KW_VAR
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) { // Tiếp tục nếu có nhiều tham số
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
    // currentToken = SB_RPAR
    // lookAhead = SB_COLON
  }
}

void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind;
  // lookAhead = TK_IDENT (Ident) _OR_ KW_VAR
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    paramKind = PARAM_VALUE;
    break;
  case KW_VAR:
    eat(KW_VAR);
    // currentToken = KW_VAR
    // lookAhead = TK_IDENT
    paramKind = PARAM_REFERENCE;
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  // currentToken = TK_IDENT
  // lookAhead = SB_COLON

  // check if the parameter identifier is fresh in the block ~ Kiểm tra xem định danh này có mới trong phạm vi hàm không 
  checkFreshIdent(currentToken->string);
  // Nếu định danh hợp lệ -> continue
  // Tạo đối tượng tham số có thành phần owner là hàm có tham số hiện tại
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  // currentToken = SB_COLON
  // lookAhead = BasicType
  type = compileBasicType(); // return TP_INT _OR_ TP_CHAR
  param->paramAttrs->type = type;
  declareObject(param);
}

// Statements ::= Statement Statements2
// Statements2 ::= SB_SEMICOLON Statement Statements2
// Statements2 ::= _rong_
void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) { // tiếp tục compile nếu có nhiều Statement
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

// Statement ::= AssignSt
// Statement ::= CallSt
// Statement ::= GroupSt
// Statement ::= IfSt
// Statement ::= WhileSt
// Statement ::= ForSt
// Statement ::= _rong_
void compileStatement(void) {
  switch (lookAhead->tokenType) {
    case TK_IDENT:
      compileAssignSt();
      break;
    case KW_CALL:
      compileCallSt();
      break;
    case KW_BEGIN:
      compileGroupSt();
      break;
    case KW_IF:
      compileIfSt();
      break;
    case KW_WHILE:
      compileWhileSt();
      break;
    case KW_FOR:
      compileForSt();
      break;
      // EmptySt needs to check FOLLOW tokens
    case SB_SEMICOLON:
    case KW_END:
    case KW_ELSE:
      break;
      // Error occurs
    default:
      error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
      break;
  }
}

void compileLValue(void) {
  Object* var;

  eat(TK_IDENT);
  // currentToken = TK_IDENT
  // check if the identifier is a function identifier, or a variable identifier, or a parameter ~ Kiểm tra định danh là hàm, biến hay tham số 
  var = checkDeclaredLValueIdent(currentToken->string);
  if (var->kind == OBJ_VARIABLE)
    compileIndexes();
}

// AssignSt ::= TK_IDENT Indexes SB_ASSIGN Expession
// AssignSt ::= TK_IDENT SB_ASSIGN Expession
void compileAssignSt(void) {
  compileLValue();
  eat(SB_ASSIGN);
  compileExpression();
}

// CallSt ::= KW_CALL ProcedureIdent Arguments
void compileCallSt(void) {
  eat(KW_CALL);
  eat(TK_IDENT);
  // check if the identifier is a declared procedure ~ Kiểm tra xem mã định danh có phải là một thủ tục được khai báo hay không
  checkDeclaredProcedure(currentToken->string);
  compileArguments();
}

// GroupSt ::= KW_BEGIN Statements KW_END
void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

// IfSt ::= KW_IF Condition KW_THEN Statement ElseSt
void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE) 
    compileElseSt();
}

// ElseSt ::= KW_ELSE statement
// ElseSt ::= _rong_
void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

// WhileSt ::= KW_WHILE Condition KW_DO Statement
void compileWhileSt(void) {
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

// ForSt ::= KW_FOR VariableIdent SB_ASSIGN Expression KW_TO Expression KW_DO Statement
void compileForSt(void) {
  eat(KW_FOR);
  eat(TK_IDENT);

  // check if the identifier is a variable
  checkDeclaredVariable(currentToken->string);

  eat(SB_ASSIGN);
  compileExpression();

  eat(KW_TO);
  compileExpression();

  eat(KW_DO);
  compileStatement();
}

void compileArgument(void) {
  compileExpression();
}

// Arguments ::= SB_LPAR Expession Arguments2 SB_RPAR
// Arguments ::= _rong_

// Arguments2 ::= SB_COMMA Expession Arguments2
// Arguments2 ::= _rong_
void compileArguments(void) {
  switch (lookAhead->tokenType) {
    case SB_LPAR:
      eat(SB_LPAR);
      compileArgument();

      while (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        compileArgument();
      }
      
      eat(SB_RPAR);
      break;
      // Check FOLLOW set 
    case SB_TIMES:
    case SB_SLASH:
    case SB_PLUS:
    case SB_MINUS:
    case KW_TO:
    case KW_DO:
    case SB_RPAR:
    case SB_COMMA:
    case SB_EQ:
    case SB_NEQ:
    case SB_LE:
    case SB_LT:
    case SB_GE:
    case SB_GT:
    case SB_RSEL:
    case SB_SEMICOLON:
    case KW_END:
    case KW_ELSE:
    case KW_THEN:
      break;
    default:
      error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void) {
  compileExpression();

  switch (lookAhead->tokenType) {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  compileExpression();
}

// Expression ::= SB_PLUS Expression2
// Expression ::= SB_MINUS Expression2
// Expression ::= Expression2
void compileExpression(void) {
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileExpression2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileExpression2();
    break;
  default:
    compileExpression2();
  }
}

// Expression2 ::= Term Expression3
void compileExpression2(void) {
  compileTerm();
  compileExpression3();
}


// Expression3 ::= SB_PLUS Term Expression3
// Expression3 ::= SB_MINUS Term Expression3
// Expression3 ::= _rong_
void compileExpression3(void) {
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileTerm();
    compileExpression3();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileTerm();
    compileExpression3();
    break;
    // check the FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
}

// Term ::= Factor Term2
void compileTerm(void) {
  compileFactor();
  compileTerm2();
}

// Term2 ::= SB_TIMES Factor Term2
// Term2 ::= SB_SLASH Factor Term2
// Term2 ::= _rong_
void compileTerm2(void) {
  switch (lookAhead->tokenType) {
    case SB_TIMES:
      eat(SB_TIMES);
      compileFactor();
      compileTerm2();
      break;
    case SB_SLASH:
      eat(SB_SLASH);
      compileFactor();
      compileTerm2();
      break;
      // check the FOLLOW set
    case SB_PLUS:
    case SB_MINUS:
    case KW_TO:
    case KW_DO:
    case SB_RPAR:
    case SB_COMMA:
    case SB_EQ:
    case SB_NEQ:
    case SB_LE:
    case SB_LT:
    case SB_GE:
    case SB_GT:
    case SB_RSEL:
    case SB_SEMICOLON:
    case KW_END:
    case KW_ELSE:
    case KW_THEN:
      break;
    default:
      error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}


// Factor ::= UnsignedConstant
// Factor ::= Variable
// Factor ::= FunctionApplication
// Factor ::= SB_LPAR Expression SB_RPAR

// UnsignedConstant ::= Number
// UnsignedConstant ::= ConstIdent
// UnsignedConstant ::= ConstChar

// currentToken = SB_ASSIGN
void compileFactor(void) {
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the identifier is declared ~ kiểm tra xem định danh đã được khai báo chưa
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind) {
      case OBJ_CONSTANT:
        break;
      case OBJ_VARIABLE: // Nếu là mảng
        compileIndexes();
        break;
      case OBJ_PARAMETER:
        break;
      case OBJ_FUNCTION: // Nếu là hàm
        compileArguments();
        break;
      default: 
        error(ERR_INVALID_FACTOR,currentToken->lineNo, currentToken->colNo);
        break;
    }
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileIndexes(void) {
  while (lookAhead->tokenType == SB_LSEL) { // Nếu là biến mảng
    eat(SB_LSEL);
    // currentToken = SB_LSEL
    // lookAhead = 
    compileExpression();
    eat(SB_RSEL);
  }
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  printObject(symtab->program,0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}
