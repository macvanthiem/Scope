/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "error.h"

extern SymTab* symtab;
extern Token* currentToken;

Object* lookupObject(char *name) {
  Scope* scope = symtab->currentScope; // Lấy phạm vi hiện tại từ bảng ký tự
  Object* obj;

  while (scope != NULL) {
    obj = findObject(scope->objList, name);
    if (obj != NULL) return obj; // Nếu tìm thấy thì trả về đối tượng đó
    scope = scope->outer; // Nếu phạm vi hiện tại không có thì chuyển ra phạm vi rộng hơn bao bọc phạm vi hiện tại
  }
  // Nếu vẫn không thấy thì sẽ tìm kiếm trong globalObjectList của bảng ký hiệu 
  obj = findObject(symtab->globalObjectList, name);
  if (obj != NULL) return obj;
  return NULL;
}

void checkFreshIdent(char *name) {
  if (findObject(symtab->currentScope->objList, name) != NULL) // Nếu định danh đã tồn tại thì dừng chương trình, thông báo lỗi.
    error(ERR_DUPLICATE_IDENT, currentToken->lineNo, currentToken->colNo);
}

Object* checkDeclaredIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL) {
    error(ERR_UNDECLARED_IDENT,currentToken->lineNo, currentToken->colNo);
  }
  return obj;
}

Object* checkDeclaredConstant(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL) // Có tồn tại định danh
    error(ERR_UNDECLARED_CONSTANT,currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_CONSTANT) // Có phải là hằng số
    error(ERR_INVALID_CONSTANT,currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredType(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_TYPE,currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_TYPE)
    error(ERR_INVALID_TYPE,currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredVariable(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_VARIABLE,currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_VARIABLE)
    error(ERR_INVALID_VARIABLE,currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredFunction(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_FUNCTION,currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_FUNCTION)
    error(ERR_INVALID_FUNCTION,currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredProcedure(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_PROCEDURE,currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_PROCEDURE)
    error(ERR_INVALID_PROCEDURE,currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredLValueIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_IDENT,currentToken->lineNo, currentToken->colNo);

  switch (obj->kind) {
    case OBJ_VARIABLE:
    case OBJ_PARAMETER:
      break;
    case OBJ_FUNCTION:
      if (obj != symtab->currentScope->owner)
        error(ERR_INVALID_IDENT,currentToken->lineNo, currentToken->colNo);
      break;
    default:
      error(ERR_INVALID_IDENT,currentToken->lineNo, currentToken->colNo);
  }

  return obj;
}