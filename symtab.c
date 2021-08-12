/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "error.h"

void freeObject(Object* obj);
void freeScope(Scope* scope);
void freeObjectList(ObjectNode *objList);
void freeReferenceList(ObjectNode *objList);

SymTab* symtab;
Type* intType;
Type* charType;

/******************* Type utilities ******************************/

Type* makeIntType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_INT;
  return type;
}

Type* makeCharType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_CHAR;
  return type;
}

Type* makeArrayType(int arraySize, Type* elementType) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_ARRAY;
  type->arraySize = arraySize;
  type->elementType = elementType;
  return type;
}

Type* duplicateType(Type* type) {
  Type* resultType = (Type*) malloc(sizeof(Type));
  resultType->typeClass = type->typeClass;
  if (type->typeClass == TP_ARRAY) {
    resultType->arraySize = type->arraySize;
    resultType->elementType = duplicateType(type->elementType);
  }
  return resultType;
}

int compareType(Type* type1, Type* type2) {
  if (type1->typeClass == type2->typeClass) {
    if (type1->typeClass == TP_ARRAY) {
      if (type1->arraySize == type2->arraySize)
	return compareType(type1->elementType, type2->elementType);
      else return 0;
    } else return 1;
  } else return 0;
}

void freeType(Type* type) {
  switch (type->typeClass) {
  case TP_INT:
  case TP_CHAR:
    free(type);
    break;
  case TP_ARRAY:
    freeType(type->elementType);
    freeType(type);
    break;
  }
}

/******************* Constant utility ******************************/

ConstantValue* makeIntConstant(int i) {
  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = TP_INT;
  value->intValue = i;
  return value;
}

ConstantValue* makeCharConstant(char ch) {
  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = TP_CHAR;
  value->charValue = ch;
  return value;
}

ConstantValue* duplicateConstantValue(ConstantValue* v) {

  // ConstantValue {
  //   enum TypeClass type;
  //   union {
  //     int intValue;
  //     char charValue;
  //   };
  // };

  ConstantValue* value = (ConstantValue*) malloc(sizeof(ConstantValue));
  value->type = v->type;
  if (v->type == TP_INT) 
    value->intValue = v->intValue;
  else
    value->charValue = v->charValue;
  return value;
}

/******************* Object utilities ******************************/

Scope* createScope(Object* owner, Scope* outer) {
  Scope* scope = (Scope*) malloc(sizeof(Scope));
  scope->objList = NULL;
  scope->owner = owner;
  scope->outer = outer;
  return scope;
}

Object* createProgramObject(char *programName) {
  Object* program = (Object*) malloc(sizeof(Object));
  strcpy(program->name, programName);
  program->kind = OBJ_PROGRAM;
  program->progAttrs = (ProgramAttributes*) malloc(sizeof(ProgramAttributes));
  program->progAttrs->scope = createScope(program,NULL);
  symtab->program = program;

  return program;
}

Object* createConstantObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_CONSTANT;
  obj->constAttrs = (ConstantAttributes*) malloc(sizeof(ConstantAttributes));
  return obj;
}

Object* createTypeObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_TYPE;
  obj->typeAttrs = (TypeAttributes*) malloc(sizeof(TypeAttributes));
  return obj;
}

Object* createVariableObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_VARIABLE;
  obj->varAttrs = (VariableAttributes*) malloc(sizeof(VariableAttributes));
  obj->varAttrs->scope = symtab->currentScope;
  return obj;
}

Object* createFunctionObject(char *name) {
  // Object {
  //   char name[MAX_IDENT_LEN];
  //   enum ObjectKind kind;
  //   union {
  //     ConstantAttributes* constAttrs;
  //     VariableAttributes* varAttrs;
  //     TypeAttributes* typeAttrs;
  //     FunctionAttributes* funcAttrs;
  //     ProcedureAttributes* procAttrs;
  //     ProgramAttributes* progAttrs;
  //     ParameterAttributes* paramAttrs;
  //   };
  // };
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_FUNCTION;
  // FunctionAttributes {
  //   struct ObjectNode_ *paramList;
  //   Type* returnType;
  //   struct Scope_ *scope;
  // };
  obj->funcAttrs = (FunctionAttributes*) malloc(sizeof(FunctionAttributes));
  obj->funcAttrs->paramList = NULL;
  obj->funcAttrs->scope = createScope(obj, symtab->currentScope);
  return obj;
}

Object* createProcedureObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PROCEDURE;
  obj->procAttrs = (ProcedureAttributes*) malloc(sizeof(ProcedureAttributes));
  obj->procAttrs->paramList = NULL;
  obj->procAttrs->scope = createScope(obj, symtab->currentScope);
  return obj;
}

Object* createParameterObject(char *name, enum ParamKind kind, Object* owner) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PARAMETER;
  obj->paramAttrs = (ParameterAttributes*) malloc(sizeof(ParameterAttributes));
  obj->paramAttrs->kind = kind;
  obj->paramAttrs->function = owner;
  return obj;
}

void freeObject(Object* obj) {
  switch (obj->kind) {
  case OBJ_CONSTANT:
    free(obj->constAttrs->value);
    free(obj->constAttrs);
    break;
  case OBJ_TYPE:
    free(obj->typeAttrs->actualType);
    free(obj->typeAttrs);
    break;
  case OBJ_VARIABLE:
    free(obj->varAttrs->type);
    free(obj->varAttrs);
    break;
  case OBJ_FUNCTION:
    freeReferenceList(obj->funcAttrs->paramList);
    freeType(obj->funcAttrs->returnType);
    freeScope(obj->funcAttrs->scope);
    free(obj->funcAttrs);
    break;
  case OBJ_PROCEDURE:
    freeReferenceList(obj->procAttrs->paramList);
    freeScope(obj->procAttrs->scope);
    free(obj->procAttrs);
    break;
  case OBJ_PROGRAM:
    freeScope(obj->progAttrs->scope);
    free(obj->progAttrs);
    break;
  case OBJ_PARAMETER:
    freeType(obj->paramAttrs->type);
    free(obj->paramAttrs);
  }
  free(obj);
}

void freeScope(Scope* scope) {
  freeObjectList(scope->objList);
  free(scope);
}

void freeObjectList(ObjectNode *objList) {
  ObjectNode* list = objList;

  while (list != NULL) {
    ObjectNode* node = list;
    list = list->next;
    freeObject(node->object);
    free(node);
  }
}

void freeReferenceList(ObjectNode *objList) {
  ObjectNode* list = objList;

  while (list != NULL) {
    ObjectNode* node = list;
    list = list->next;
    free(node);
  }
}

void addObject(ObjectNode **objList, Object* obj) {

  // ObjectNode {
  //   Object *object;
  //   struct ObjectNode_ *next;
  // };

  ObjectNode* node = (ObjectNode*) malloc(sizeof(ObjectNode));
  node->object = obj;
  node->next = NULL;
  if ((*objList) == NULL) // Nếu objList trống thì node vừa tạo ra sẽ đứng đầu danh sách
    *objList = node;
  else {
    ObjectNode *n = *objList; // Không thì thêm node vào cuối danh sách
    while (n->next != NULL) 
      n = n->next;
    n->next = node;
  }
}

// Đầu vào là danh sách đối tượng của phạm vi hiện tại của bảng kí hiệu và một định danh  
Object* findObject(ObjectNode *objList, char *name) {
  while (objList != NULL) {
    if (strcmp(objList->object->name, name) == 0) // Nếu định danh truyền vào đã xuất hiện trong danh sách đối tượng của phạm vị hiện tại của bảng ký tự
      return objList->object; // thì trả về đối tượng đó
    else objList = objList->next; // không thì tiếp tục duyệt
  }
  return NULL; // nếu không tìm thấy - định danh kia là mới thì trả về NULL
}

/******************* others ******************************/

void initSymTab(void) {

  Object* obj;
  Object* param;

  // SymTab {
  //   Object* program;
  //   Scope* currentScope;
  //   ObjectNode *globalObjectList;
  // };

  symtab = (SymTab*) malloc(sizeof(SymTab));  // Khởi tạo bảng ký tự

  symtab->globalObjectList = NULL;
  
  // FunctionAttributes {
  //   struct ObjectNode_ *paramList;
  //   Type* returnType;
  //   struct Scope_ *scope;
  // };

  // Tạo các global object
  obj = createFunctionObject("READC"); // Tạo đối tượng hàm đọc vào một ký tự từ bàn phím
  obj->funcAttrs->returnType = makeCharType();
  addObject(&(symtab->globalObjectList), obj); // Thêm đối tượng hàm vào globalObjectList (đưa về objectNode)

  obj = createFunctionObject("READI"); // Tạo đối tượng hàm đọc vào một số nguyên từ bàn phím
  obj->funcAttrs->returnType = makeIntType();
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITEI"); // Tạo đôí tượng thủ tục in ra màn hình một số nguyên
  param = createParameterObject("i", PARAM_VALUE, obj); // Tạo đối tượng tham số là con của đối tượng thủ tục trên
  param->paramAttrs->type = makeIntType();
  addObject(&(obj->procAttrs->paramList),param); // Thêm đối tượng tham số vào danh sách tham số của thủ tục (đưa về objectNode)
  addObject(&(symtab->globalObjectList), obj); // Thêm đối tượng thủ tục vào globalObjectList (đưa về objectNode)

  obj = createProcedureObject("WRITEC"); // Tạo đối tượng thủ tục in ra màn hình một ký tự
  param = createParameterObject("ch", PARAM_VALUE, obj);
  param->paramAttrs->type = makeCharType();
  addObject(&(obj->procAttrs->paramList),param);
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITELN"); // Tạo đối tượng thủ tục thực hiện in dấu xuống dòng khi in ra màn hình.
  addObject(&(symtab->globalObjectList), obj);

  intType = makeIntType();
  charType = makeCharType();
}

void cleanSymTab(void) {
  freeObject(symtab->program);
  freeObjectList(symtab->globalObjectList);
  free(symtab);
  freeType(intType);
  freeType(charType);
}

void enterBlock(Scope* scope) {
  symtab->currentScope = scope;
}

void exitBlock(void) {
  symtab->currentScope = symtab->currentScope->outer;
}

// Khai báo đối tượng hằng
void declareObject(Object* obj) {
  if (obj->kind == OBJ_PARAMETER) { // Nếu là tham số
    Object* owner = symtab->currentScope->owner; // program (hàm hoặc thủ tục tương ứng)
    switch (owner->kind) {
    case OBJ_FUNCTION: // Nếu là đối tượng hàm
      addObject(&(owner->funcAttrs->paramList), obj);
      break;
    case OBJ_PROCEDURE: // Nếu là đối tượng thủ tục
      addObject(&(owner->procAttrs->paramList), obj);
      break;
    default:
      break;
    }
  }
 
  addObject(&(symtab->currentScope->objList), obj);
}