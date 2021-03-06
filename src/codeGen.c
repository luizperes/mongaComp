#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#if !defined(tree_h)
	#include "tree.h"
	#define tree_h
#endif

void codeDefVar(DefVar* dv);
void codeDefFunc(DefFunc* df);
void codeDefVarList(DefVarL* dvl);
void codeDefList(Def* d);
void codeNameList(NameL* nl,Type* t,int scope);
void codeType(Type* t);
void codeParams(Parameter* params);
void codeForAllocParams(Parameter* params);
void codeCommandList(CommandL* cl);
int codeExpAccess(Exp* e);
void codeBlock(Block* b);
int codeExp(Exp* e);
void codeVar(Var* v);
void codeConstant(Constant* c);
void codeExpList(ExpList* el);
int codeAccessElemPtr(Exp* e);
char* stringForType(Type* t);

static FILE* output = NULL;
static int currentFunctionTIndex = 0;
static int currentBrIndex = 0;

static int currentStringConstant = 0;
static int declareTop = 0;
char* stringsToDeclare[100];

static int currentFuncHasReturn = 0;


void setCodeOutput(FILE* out) {
	output = out;
} 

static void defaultOutput() {
	output = stdout;
}


static Parameter* currentParameters = NULL;

char* stringForType(Type* t) {
	char* tStr = NULL;
	if(t == NULL)
		return "void";
	switch(t->tag) {
		case base:
			switch(t->b) {
				case WInt:
					return "i32";
				break;
				case WFloat:
					return "float";
				break;
				case WChar:
					return "i8";
				break;
			}
		break;
		case array:
			
			tStr = stringForType(t->of);
			int tlen = strlen(tStr);
			char* new = malloc(tlen+2);
			strcpy(new,tStr);
			//free(tStr); // no need anymore
			new[tlen] = '*';
			new[tlen+1] = '\0';
			return new;

		break;
	}
}

static char* stringForDefaultValue(Type* t) {
	if(!t) {
		printf(";That's probably an error\n");
		return "void";
	}
	if(t->tag == base) {
		if(t->b == WInt || t->b == WChar) {
			return "0";
		}
		else {
			return "0.0";
		}
	} else {
		return "null";
	}

}
static void codeDefaultReturn(Type* t) {
	if(!t) {
		fprintf(output, "ret void\n");
		return;
	}
	char* tStr = stringForType(t);
	char* value = stringForDefaultValue(t);
	fprintf(output, "ret %s %s\n", tStr,value);
}

static void pushStringToDeclare(char* str) {
	//printf("%s\n",str );
	char* nstr = malloc(strlen(str)+1);
	strcpy(nstr,str);
	nstr[strlen(str)] = '\0';
	stringsToDeclare[declareTop] = nstr ;
	declareTop++;
}
static void declateStringsToDeclare() {
	int x = currentStringConstant - declareTop;
	for(int i = 0;i<declareTop;i++,x++) {
		//printf("ihihiz\n");
		int len = strlen(stringsToDeclare[i])+1;
		fprintf(output, "@.cstr.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\"\n",
		x+1,
		len,
		 stringsToDeclare[i]);
		free(stringsToDeclare[i]);
		stringsToDeclare[i] = NULL;
	}
	declareTop = 0;
}

char* stringForVarAddress(const char* name,int scope) {
	char string[50] = "no string yet";
	if(strlen(name) >= 50) {
		printf("SevereError. var name is to big\n");
	}
	if(scope == 0) {
		sprintf(string,"@g%s",name);
	}
	else {
		sprintf(string,"%%l%d%s",scope,name);
	}
	
	char* str = (char*)malloc(strlen(string)+1);
	strcpy(str,string);
	str[strlen(string)] = '\0';
	return str;
}
static void codeExtraDeclares() {
	//fprintf(output, "declare noalias i8* @malloc(i64)\n" );
	fprintf(output, "; generated with mongaComp \n" );
	fprintf(output, "declare i8* @malloc(i32)\n" ); //malloc 
	fprintf(output, "declare i32 @printf(i8* nocapture readonly, ...)\n" );
	fprintf(output, "declare i32 @puts(i8* nocapture readonly)\n" );
	fprintf(output, "@.intprintstr = private unnamed_addr constant [3 x i8] c\"%%d\\00\"\n" );
	fprintf(output, "@.floatprintstr = private unnamed_addr constant [3 x i8] c\"%%f\\00\"\n" );
	fprintf(output, "@.charprintstr = private unnamed_addr constant [3 x i8] c\"%%c\\00\"\n" );
	fprintf(output, "@.strprintstr = private unnamed_addr constant [3 x i8] c\"%%s\\00\"\n" );
	fprintf(output, "; End of monga dependencies \n" );
}

void codeTree() {
	if(output==NULL) {
		defaultOutput();
	}
	currentStringConstant = 0;
	printf("generating code for tree\n");
	codeExtraDeclares();
	codeDefList(globalTree->next);
} 
void codeDefVar(DefVar* dv) {
	
	codeNameList(dv->nl,dv->t,dv->scope);	

}
void codeDefFunc(DefFunc* df) {

	char* typeStr = stringForType(df->retType);
	if(df->b) {
		currentFunctionTIndex = 0;
		currentParameters = df->params;
		declareTop = 0;
		currentFuncHasReturn = 0;
		currentBrIndex = 0;
		fprintf(output, "define %s @%s(", typeStr,df->id);
		codeParams(df->params);
		fprintf(output, ")\n{\n");
		codeForAllocParams(df->params);
		codeBlock(df->b);
		if(df->retType == NULL) {
			fprintf(output, "ret void\n");
		} //probably missing a ret in the end of void func
		else if(currentFuncHasReturn == 0) {
			fprintf(stderr, "Warning: missing return in function %s\n",df->id);
			fprintf(stderr, "I will be overwritten by i deafult return\n");
		}
		codeDefaultReturn(df->retType);
		fprintf(output, "}\n");
		currentFunctionTIndex = 0;
		currentParameters = NULL;
		declateStringsToDeclare();
		currentFuncHasReturn = 0;
		currentBrIndex = 0;
	}
	else {
		fprintf(output, "declare %s @%s(", typeStr,df->id);
		codeParams(df->params);
		fprintf(output, ")\n");
	}
}
void codeDefVarList(DefVarL* dvl) {

	DefVarL* p = dvl;
	while(p) {
		codeDefVar(p->dv);
		p=p->next;
	}
}
void codeDefList(Def* d) {
	if(!d)
		return;
	//printf("coding DefList\n");
	switch (d->tag) {
		case DVar:
			codeDefVar(d->u.v);
		break;
		case DFunc:
			codeDefFunc(d->u.f);
		break;
	}
	codeDefList(d->next);

}
void codeNameList(NameL* nl,Type* t,int scope) {
	char* tStr = stringForType(t);
	char* vStr = stringForDefaultValue(t);
	NameL* p = nl;
	if(scope) {
		while(p) {
			char* string = stringForVarAddress(p->name,scope);
			fprintf(output, "%s = alloca %s\n", 
				string,  
				tStr );
			free(string);
			p=p->next;
		}
	} else {
		while(p) {
			char* string = stringForVarAddress(p->name,scope);

			fprintf(output, "%s = global %s %s \n", 
				string,  
				tStr,
				vStr );
			free(string);
			p=p->next;
		}
	}
}
void codeType(Type* t);
void codeParams(Parameter* params) {
	if(!params)
		return;
	char * tStr = stringForType(params->t);
	fprintf(output,"%s",tStr);
	if(params->next) {
		fprintf(output, "," );
		codeParams(params->next);
	}
}
void codeForAllocParams(Parameter* params) {
	if(!params)
		return;
	
	int index = currentFunctionTIndex;
	Parameter* p = params;
	while(p) {
		char * tStr = stringForType(p->t);
		fprintf(output,"%%t%d = alloca %s\n", currentFunctionTIndex++, tStr);
		p = p->next;
	}
	p = params;
	int i=0;
	while(p) {
		char * tStr = stringForType(p->t);
		fprintf(output,"store %s %%%d, %s* %%t%d\n", tStr, i++, tStr, index++);
		p = p->next;
	}

}
void codeForAssign() {

}
/* 
 %t315 = getelementptr i32* @g25, i32 0
 ~= i315 = &g25[0];
*/
// void getelementptr(char* str) {
// 	fprintf(output, "%s\n", );
// }
char* adressOfLeftAssign(Exp* e) {
	//printf("addr left assig\n");
	if(e->tag == ExpVar) {
		//printf("e->var.id %s\n",e->var->id);
		
		if (e->var->declaration == NULL){
			Parameter* p = currentParameters;
			int t=0;
			while(p) {
				if(strcmp(e->var->id,p->id)==0)
					break;
				p=p->next;
				t++;
			}
			char * str = (char*)malloc(t/10+3);
			sprintf(str,"%%t%d",t);
			return str;
		}
		else {
			int scope = e->var->declaration->scope;
			char* varAddr = stringForVarAddress(e->var->id,scope);
			return varAddr;
		}
	}
	else if(e->tag == ExpAccess) {
		int i1 = codeExpAccess(e)-1;  //received getElemPtr
		char * str = (char*)malloc(i1/10+3);
		sprintf(str,"%%t%d",i1);
		return str;
	}
	else {
		fprintf(output, ";SevereError\n");
	}
	return NULL;
}
int codeCond(Exp* e) {
	int i1;
	//fprintf(output, ";begin codecond\n");
	i1 = codeExp(e);
	
	char* tStr = stringForType(e->type);
	currentFunctionTIndex++;
	fprintf(output, "%%t%d = icmp ne %s %%t%d, 0\n",
	currentFunctionTIndex,
	tStr,
	i1 );
	//fprintf(output, ";end codecond\n");
	i1 = currentFunctionTIndex;
	
	return i1;
}
void codeCommandList(CommandL* cl) {
	if(!cl)
		return;
	CommandL* c = cl;
	//printf("CommandL\n");
	int i1,i2;
	int b1,b2,b3;
	while(c) {
		//printf("cl\n");
		switch(c->tag) {
			case CWhile:
			 	i1 = codeCond(c->condExp);
				b1 = currentBrIndex++;
				b2 = currentBrIndex++;
				fprintf(output, "br i1 %%t%d, label %%b%d, label %%b%d\n",
				 i1,
				 b1,
				 b2);
				fprintf(output, "b%d:\n",b1 );
				codeCommandList(c->cmdIf );
				i2 = codeCond(c->condExp);
				fprintf(output, "br i1 %%t%d, label %%b%d, label %%b%d\n",
				 i2,
				 b1,
				 b2);
				fprintf(output, "b%d:\n",b2 );
				// leaveScope();
			break;
			case CIf:
				i1 = codeCond(c->condExp);
				b1 = currentBrIndex++;
				b2 = currentBrIndex++;
				fprintf(output, "br i1 %%t%d, label %%b%d, label %%b%d\n",
				 i1,
				 b1,
				 b2);
				fprintf(output, "b%d:\n",b1 );
				codeCommandList(c->cmdIf );
				fprintf(output, "br label %%b%d\n",b2 );
				fprintf(output, "b%d:\n",b2 );				// leaveScope();
			break;
			case CIfElse:
				i1 = codeCond(c->condExp);
				b1 = currentBrIndex++;
				b2 = currentBrIndex++;
				b3 = currentBrIndex++;
				fprintf(output, "br i1 %%t%d, label %%b%d, label %%b%d\n",
				 i1,
				 b1,
				 b2);
				fprintf(output, "b%d:\n",b1 );
				codeCommandList(c->cmdIf );
				fprintf(output, "br label %%b%d\n",b3 );
				fprintf(output, "b%d:\n",b2 );
				codeCommandList(c->cmdElse );
				fprintf(output, "br label %%b%d\n",b3 );
				fprintf(output, "b%d:\n",b3 );
			break;
			case CReturn:	
				//printf("cret\n");
				currentFuncHasReturn = 1;
				if(c->retExp == NULL) {
					fprintf(output, "ret void\n");
				}
				else {
				char * tStr = stringForType(c->retExp->type);
				i1 = codeExp(c->retExp);
				fprintf(output, "ret %s %%t%d\n",tStr,i1);
				}
			break;
			case CAssign:
				 i1 = codeExp(c->expRight);
				 char* tStr = stringForType(c->expLeft->type);
				 char* addr = adressOfLeftAssign(c->expLeft);
				 
				 fprintf(output, "store %s %%t%d, %s* %s \n",
				 	tStr,i1,tStr,addr);
			break;
			case CBlock:
				//printf("cblock\n");
				codeBlock((Block*)c->block );
				// leaveScope();
			break;
			case CCall:
				//printf("ccall\n");
				codeExp(c->expRight);
			break;
			case CPrint:
				i1 =  codeExp(c->printExp);
				if(c->printExp->type == NULL) {
					fprintf(output, ";printing void expression is unavaible\n" );
				}
				else if(c->printExp->type->tag == base) {
					switch(c->printExp->type->b) {
						case WInt:
						fprintf(output, "tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.intprintstr, i64 0, i64 0), i32 %%t%d)\n",
						i1 );
						
						break;
						case WFloat:
						currentFunctionTIndex++;
						fprintf(output, "%%t%d = fpext float %%t%d to double\n", 
							currentFunctionTIndex,
							i1 );
						 
						fprintf(output, "tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.floatprintstr, i64 0, i64 0), double %%t%d)\n",
						currentFunctionTIndex );
						break;
						case WChar:
						fprintf(output, "tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.charprintstr, i64 0, i64 0), i8 %%t%d)\n",
						i1 );
						break;
					}
				}
				else if(c->printExp->type->of->tag == base &&
					c->printExp->type->of->b == WChar) {
					fprintf(output, "tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.strprintstr, i64 0, i64 0), i8* %%t%d)\n",
						i1 );
					//fprintf(output, "tail call i32 @puts(i8* %%t%d)\n", i1);
					
				}
				else {
					fprintf(output, ";printing non string array is unavaible\n" );
				}
				// if (!checkPrintability(c->printExp)) {
				// 	typeError("Expression is not printable");
				// }
			break;
		}
		c = c->next;
	}
}
void codeBlock(Block* b) {
	codeDefVarList(b->dvl);
	codeCommandList(b->cl);
}
int codeBinExp(Exp* e ,char * cmd) {
	int te1,te2; 
	te1 = codeExp(e->bin.e1 );
	te2 = codeExp(e->bin.e2 );
	// te2 = currentFunctionTIndex-1;
	// te1 = te2-1;
	char * tStr = stringForType(e->type);
	currentFunctionTIndex++;
	int index = currentFunctionTIndex;
	fprintf(output, "%%t%d = %s %s %%t%d, %%t%d\n",
		currentFunctionTIndex++,cmd, tStr, te1,te2);
	return index;
}
int codeCallExp(Exp* e) {
	int toCall = -1;
	int size=0;
	ExpList *p = e->call.expList;
	//calculate size
	while(p) {
		size++;
		p = p->next;
	}
	//generate code for arguments
	p = e->call.expList;
	int * args = (int*)malloc(sizeof(int)*size);
	int i=0;
	while(p) {
		int index = codeExp(p->e);
		args[i] = index;
		i++;
		p=p->next;
	}

	if(e->type == NULL) {
		fprintf(output, "call void @%s(",
			e->call.id);
	}
	else {
		char* fTypeStr = stringForType(e->type);
		 toCall = ++currentFunctionTIndex; 
		fprintf(output, "%%t%d = call %s @%s(",
			toCall,
			fTypeStr,
			e->call.id);
	}
	p = e->call.expList;
	i=0;
	while(p) {
		char* tStr = stringForType(p->e->type);
		fprintf(output, "%s %%t%d",tStr,args[i]);
		if(p->next)
			fprintf(output, ", ");
		p = p->next;
		i++;
	}
	fprintf(output, ")\n" );
	return toCall;		
}
char* hexaStringForFloat(float c) {
	double f = (double)c;
	int i;
	unsigned char buff[sizeof(double)];
	memcpy(buff,&f,sizeof(double));

	char* string = (char*)malloc(sizeof(double)+3);
	char temp[3] = "0x";
	strcpy(string,temp);
	for(i = sizeof(double)-1;i >= 0;i--){
        sprintf(temp,"%02X",buff[i]);
        strcat(string,temp);
    }
    return string;

}
char* stringForConstant(Constant* c) {
	//char str[40] = "no string given";
	char* str;
	double nd;
	int exponent = 0;
	if(!c)
		return NULL;
	switch(c->tag) {
		case KInt:
			str = (char*)malloc(40);
			sprintf(str, "%d", c->u.i);
		break;
		case KFloat:
			nd = frexp(c->u.d, &exponent);
			str = hexaStringForFloat(c->u.d);
		break;
		case KStr:
			str = (char*)c->u.str;
		break;
	}
	return &str[0];
}
int codeExpPrim(Exp* e) {
	currentFunctionTIndex++;
	char* tStr = stringForType(e->type);
	if(e->c->tag == KStr) {
		currentStringConstant++;
		char* cStr = stringForConstant(e->c);
		int len = strlen(cStr) + 1;
		fprintf(output, "%%t%d = getelementptr inbounds [%d x i8], [%d x i8]* @.cstr.%d, i64 0, i64 0\n",
		currentFunctionTIndex,
		len,
		len,
		currentStringConstant );
		
		pushStringToDeclare(cStr);

		return currentFunctionTIndex;
	}
	
	char* cStr = stringForConstant(e->c);
	// fprintf(output, "%%t%d = alloca %s* \n",currentFunctionTIndex);
	// fprintf(output, "store i8* %0, i8** %2, align 8\n", );
	if(e->c->tag == KFloat) {
		fprintf(output, "%%t%d= fadd %s 0.0 , %s\n",
			currentFunctionTIndex,
			tStr,
			cStr );
	} else {
		fprintf(output, "%%t%d= add nsw %s 0 , %s\n",
			currentFunctionTIndex,
			tStr,
			cStr );
	}
	int index = currentFunctionTIndex++;
	return index;
}
int getAddressOfVar(Var* id) {
	return -1;
}
int codeExpVar(Exp* e) {
	currentFunctionTIndex++;
	char* tStr = stringForType(e->type);
	if(e->var->declaration == NULL)
	{
		//printf(";params\n");
		Parameter* p = currentParameters;
		int t=0;
		while(p) {
			if(strcmp(e->var->id,p->id)==0)
				break;
			p=p->next;
			t++;
		}

		fprintf(output,"%%t%d = load %s, %s* %%t%d\n", 
				currentFunctionTIndex,
				 tStr,
				 tStr,
				 t);
	}
	else {
		int scope = e->var->declaration->scope;
		if(scope == 0) {
			char* varAddr = stringForVarAddress(e->var->id,scope);
			fprintf(output,"%%t%d = load %s, %s* %s\n", 
						currentFunctionTIndex,
						 tStr,
						 tStr,
						 varAddr);
			//fprintf(output, ";global\n");
		} 
		else {
			char* varAddr = stringForVarAddress(e->var->id,scope);
			fprintf(output,"%%t%d = load %s, %s* %s\n", 
						currentFunctionTIndex,
						 tStr,
						 tStr,
						 varAddr);

			//fprintf(output, ";scope not zero\n");
		} 
		//printType(e->type,0);
	}
	return currentFunctionTIndex;
}
int codeExpUnary(Exp* e) {
	char* tStr = stringForType(e->type);
	int i1,i2;
	i1 = codeExp(e->unary.e);
	currentFunctionTIndex++;
	switch(e->unary.op) {
		case MINUS:
			if(e->type->b == WFloat) {
				fprintf(output, "%%t%d = fsub %s 0.0, %%t%d\n",
				 currentFunctionTIndex,
				 tStr,
				 i1);
				
			}
			else {
				fprintf(output, "%%t%d = sub nsw %s 0, %%t%d\n",
				 currentFunctionTIndex,
				 tStr,
				 i1);
			}
		break;
		case NOT:
			if(e->type->b == WFloat) {
				fprintf(output, "%%t%d = fcmp oeq float %%t%d, 0.0\n",
				 currentFunctionTIndex,
				 i1);
			i2 = currentFunctionTIndex++;
  			fprintf(output, "%%t%d = uitofp i1 %%t%d to float\n",
				currentFunctionTIndex,
				i2);
			}
			else {
				fprintf(output, "%%t%d = icmp eq %s %%t%d, 0\n",
				 currentFunctionTIndex,
				 tStr,
				 i1);
				i2 = currentFunctionTIndex++;
				fprintf(output, "%%t%d = zext i1 %%t%d to %s\n",
				currentFunctionTIndex,
				i2,
				tStr );
			}
		break; 
	}
	return currentFunctionTIndex;
}
int codeExpCast(Exp* e) {
	int i1 = codeExp(e->cast.e);
	char* orTStr = stringForType(e->cast.e->type);
	char* toTStr = stringForType(e->type);
	currentFunctionTIndex++;
	if(e->type->b == WFloat) {
		fprintf(output, "%%t%d = sitofp %s %%t%d to float\n",
		currentFunctionTIndex,
		orTStr,
		i1 );
	}
	else if(e->cast.e->type->b == WFloat) {
		fprintf(output, "%%t%d = fptosi float %%t%d to %s\n",
		currentFunctionTIndex,
		i1,
		toTStr);
		
	}
	else {
		if(e->type->b == WChar) {
			fprintf(output, "%%t%d = trunc i32 %%t%d to i8\n",
			currentFunctionTIndex,
			i1 );
		}
		else if(e->cast.e->type->b == WChar) {
			fprintf(output, "%%t%d = sext i8 %%t%d to i32\n",
			currentFunctionTIndex,
			i1 );
		}
		else if(e->cast.e->type->b == e->type->b) {
			fprintf(output, ";cast useless\n");
			return i1;
		}
		else {
			fprintf(output, ";cast not implemented\n");
			return -1;
		}
	}
	return currentFunctionTIndex;
}
char* adressOfParameter(const char* id) {
	Parameter* p = currentParameters;
		int t=0;
		while(p) {
			if(strcmp(id,p->id)==0)
				break;
			p=p->next;
			t++;
		}
		char * str = (char*)malloc(t/10+3);
		sprintf(str,"%%t%d",t);
		return str;
		
}
char* addressOfVector(Exp* e) {
	//printf("addressOfVector\n");
	if(e->tag == ExpAccess) {
		//printf("gacr\n");
		return addressOfVector(e->access.varExp);
	}
	else if(e->tag == ExpVar) {
		//printf("gVar\n");

		if(e->var->declaration == NULL)
		{
			return adressOfParameter(e->var->id);
		}
		return adressOfLeftAssign(e);
	}
	else {
		return "%%SevereError";
	}
}
int codeGetElemPtr(Type* type,int arrayTemp,int indexTemp) {
	currentFunctionTIndex++;
	char* tStr = stringForType(type);
	fprintf(output, "%%t%d = getelementptr %s, %s* %%t%d, i64 %%t%d\n",
	currentFunctionTIndex,
	tStr,
	tStr,
	arrayTemp,
	indexTemp);
	return currentFunctionTIndex;
}
int codeAccessElemPtr(Exp* e) {
	//fprintf(output,";getelementptr\n");
	int i1 = codeExp(e->access.indExp);
	currentFunctionTIndex++;
	fprintf(output, "%%t%d = sext i32 %%t%d to i64\n",
			currentFunctionTIndex,
			i1 );
	
	char* tStr = stringForType(e->type);
	int index = currentFunctionTIndex++;
	char* str = addressOfVector(e->access.varExp);
	
	fprintf(output, "%%t%d = load %s*, %s** %s\n",
	currentFunctionTIndex,
	tStr,
	tStr,
	str );
	//fprintf(output,"; %s mark1\n",stringForType(e->type));

	int startArrayAddress = currentFunctionTIndex++;
	fprintf(output, "%%t%d = getelementptr %s, %s* %%t%d, i64 %%t%d\n",
	currentFunctionTIndex,
	tStr,
	tStr,
	startArrayAddress,
	index);
	return currentFunctionTIndex;
}
int codeExpAccess(Exp* e) {
	//fprintf(output,";Exp Access\n");

	int i1,i2;
	char* tStr;
	int access;
	switch (e->tag) {
		case ExpAccess :
		i1 = codeExp(e->access.indExp);
		currentFunctionTIndex++;
		fprintf(output, "%%t%d = sext i32 %%t%d to i64\n",
			currentFunctionTIndex,
			i1 );
		tStr = stringForType(e->type);
		int index = currentFunctionTIndex++;
		access = codeExp(e->access.varExp);
		currentFunctionTIndex++;

		i2 =  codeGetElemPtr(e->type,
			access,
			index);
		// char* str = stringForVarAddress(e->access.varExp);
		currentFunctionTIndex++;
		fprintf(output, "%%t%d = load %s, %s* %%t%d\n",
		currentFunctionTIndex,
		tStr,
		tStr,
		i2 );
		
		return currentFunctionTIndex;
		break;
		default:
		//fprintf(output, ";default reached\n" );
		tStr = stringForType(e->type);
		char* str = addressOfVector(e);
		currentFunctionTIndex++;
		fprintf(output, "%%t%d = load %s, %s* %s\n",
		currentFunctionTIndex,
		tStr,
		tStr,
		str );
		return currentFunctionTIndex;
		break;
	}
	// i1 = codeAccessElemPtr(e);
	// char* tStr = stringForType(e->type);
	// currentFunctionTIndex++;
	// fprintf(output, "%%t%d = load %s, %s* %%t%d\n",
	// currentFunctionTIndex,
	// tStr,
	// tStr,
	// i1);
	return currentFunctionTIndex;	
}
int sizeOfType(Type* t) {
	if(t->tag == base) {
		switch(t->b) {
			case WInt:
				return sizeof(int);
			break;
			case WFloat:
				return sizeof(float);
			break;
			case WChar:
				return sizeof(char);
			break;
		}
	}
	else {
		return sizeof(int*); //pointer size
	}
}
int codeExpNew(Exp* e) {
	int i1 = codeExp(e->eNew.e);
	char * tStr = stringForType(e->type);
	currentFunctionTIndex++;
	fprintf(output, "%%t%d = mul i32 %%t%d, %d\n",
		currentFunctionTIndex, 
		i1,
		sizeOfType(e->type));
	i1 = currentFunctionTIndex++;
	fprintf(output, " %%t%d = tail call i8* @malloc(i32 %%t%d)\n",
	currentFunctionTIndex,
	i1);
	int i2 = currentFunctionTIndex++;
	fprintf(output, "%%t%d = bitcast i8* %%t%d to %s\n",
	currentFunctionTIndex,
	i2,
	tStr );
	return currentFunctionTIndex;
}
void codeLabel(int label) {
	fprintf(output, "b%d:\n",label);
}
void codeBranches(int cond, int lt,int lf) {
	fprintf(output, "br i1 %%t%d, label %%b%d, label %%b%d\n", 
			cond,
			lt,
			lf);
}

int codeCondToValue(int b1,int b2,int b3) {
	fprintf(output, "b%d:\n", b1);
	fprintf(output, "br label %%b%d\n",
		b3);
	fprintf(output, "b%d:\n", b2);
	fprintf(output, "br label %%b%d\n",
		b3);
	fprintf(output, "b%d:\n", b3);
	currentFunctionTIndex++;
	fprintf(output, "%%t%d = phi i32 [ 1, %%b%d ], [0, %%b%d]\n",
	currentFunctionTIndex,
	b1,
	b2 );
	return currentFunctionTIndex;
}
int codeSimpleCompare(Exp* e,const char* oprStr ) {
	int i1,i2;
	i1 = codeExp(e->cmp.e1);
	i2 = codeExp(e->cmp.e2);
	currentFunctionTIndex++;
	fprintf(output, "%%t%d = icmp %s i32 %%t%d, %%t%d\n",
		currentFunctionTIndex,
		oprStr,
		i1,
		i2 );
	return currentFunctionTIndex;

}
int codeExpCompare(Exp* e) {
	int i1,i2;
	int b1 = currentBrIndex++;
	int b2 = currentBrIndex++;
	int b3 = currentBrIndex++;
	int ln;
	switch(e->cmp.op) {
		case GT:
			codeSimpleCompare(e,"sgt");
		break;
		case GTE:
			codeSimpleCompare(e,"sge");
		break;
		case LS:
			codeSimpleCompare(e,"slt");
		break;
		case LSE:
			codeSimpleCompare(e,"sle");
		break;
		case EqEq:
			codeSimpleCompare(e,"eq");
		break;
		case OR:
			i1 = codeExp(e->cmp.e1);
			currentFunctionTIndex++;
			fprintf(output, "%%t%d = icmp ne i32 %%t%d, 0\n",
			currentFunctionTIndex,
			i1);
			ln = currentBrIndex++;
			codeBranches(currentFunctionTIndex,b1,ln); 
			codeLabel(ln);
			i2 = codeExp(e->cmp.e2);
			currentFunctionTIndex++;
			fprintf(output, "%%t%d = icmp ne i32 %%t%d, 0\n",
			currentFunctionTIndex,
			i2);
			codeBranches(currentFunctionTIndex,b1,b2);
			return codeCondToValue(b1,b2,b3);
		break;
		case AND:
			i1 = codeExp(e->cmp.e1);
			currentFunctionTIndex++;
			fprintf(output, "%%t%d = icmp ne i32 %%t%d, 0\n",
			currentFunctionTIndex,
			i1);
			ln = currentBrIndex++;
			codeBranches(currentFunctionTIndex,ln,b2); 
			codeLabel(ln);
			i2 = codeExp(e->cmp.e2);
			currentFunctionTIndex++;
			fprintf(output, "%%t%d = icmp ne i32 %%t%d, 0\n",
			currentFunctionTIndex,
			i2);
			codeBranches(currentFunctionTIndex,b1,b2);
			return codeCondToValue(b1,b2,b3);
		break;
	}
	codeBranches(currentFunctionTIndex,b1,b2);
	return codeCondToValue(b1,b2,b3);
}

int codeExp(Exp* e) {
	int result =-1;
	if(!e)
		return -1;
	switch(e->tag) {
		case ExpAdd:
			if(e->type->b == WFloat) 
				result = codeBinExp(e,"fadd");
			else
				result = codeBinExp(e,"add nsw");
		break;
		case ExpSub:
			if(e->type->b == WFloat) 
				result = codeBinExp(e,"fsub");
			else
				result =codeBinExp(e,"sub nsw");
		break;
		case ExpMul:
			if(e->type->b == WFloat) 
				result = codeBinExp(e,"fmul");
			else
				result = codeBinExp(e,"mul nsw");
		break;
		case ExpDiv:
			if(e->type->b == WFloat) 
				result = codeBinExp(e,"fdiv");
			else
				result = codeBinExp(e,"sdiv");
		break;
		case ExpCall:
			result = codeCallExp(e);
		break;
		case ExpVar:
			result = codeExpVar(e);
		break;
		case ExpUnary:
			result = codeExpUnary(e);
		break;
		case ExpPrim:
			result = codeExpPrim(e);
			// e->type = typeOfConstant(e->c);
		break;
		case ExpNew:
			result = codeExpNew(e);
			// if(!checkTypeIndex(e->eNew.e)) {
			// 	typeError("Index of array is not an int");
			// }
			// e->type = typeOfNew(e);
		break;
		case ExpCmp:
			result = codeExpCompare(e);

		break;
		case ExpAccess:
			result = codeExpAccess(e);
			// typeExp(e->access.varExp);
			// typeExp(e->access.indExp);
			// if(!checkTypeIndex(e->access.indExp)) {
			// 	typeError("Index of array is not an int");
			// }
			// e->type = e->access.varExp->type->of;
		break;
		case ExpCast:
			result = codeExpCast(e);
			// if(!checkTypeCast(e)) {
			// 	// printType(e->type,0);
			// 	// printType(e->cast.type,0);
			// 	// printType(e->cast.e->type,0);
			// 	typeError("Cast not avaible for these types");
			// }
			// e->type = e->cast.type;
		break;
	}

	return result;
}
void codeVar(Var* v) {
	char* tStr = stringForType(v->type);
	fprintf(output, "%%L%s = alloca %s\n",
	v->id,  tStr );
}
void codeConstant(Constant* c);
void codeExpList(ExpList* el) {
	char * tStr;
	if(!el)
		return;
	ExpList *p = el;
	while(p) {
		int index = codeExp(p->e);
		tStr = stringForType(p->e->type);
		fprintf(output, "%s %%t%d",tStr,index);
		if(p->next)
			fprintf(output, ", ");
		p = p->next;
	}
	return;

}