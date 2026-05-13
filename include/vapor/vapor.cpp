/*
    Version of Vapor modified for C++
*/
#include "vapor.h"

[[deprecated("Might not even work")]]
vp_rgb_t vp_rgb(uint8_t x, uint8_t y, uint8_t z){
    vp_rgb_t temp = {x,y,z};
    return temp;
}

void vp_throw(const char* msg, int status){
    if (status != 0){
        printf("%sVapor Error:%s '%s'%s, stopping program.\n",vp_red,vp_blue,msg,vp_reset);
        exit(status);
    } 
    #ifndef __VP_DISABLE_ERRORLOG
    printf("%sVapor Error:%s '%s'%s.\n",vp_red,vp_blue,msg,vp_reset);
    #endif
}

vp_result vp_Mwritefile(const char* fileName, const char* data, const char* mode){
    FILE* fptr = fopen(fileName, mode);
    if (fptr == NULL)
        return vp_FAIL;

    fputs(data, fptr);
    fclose(fptr);
    return vp_OK;
}

vp_result vp_RWwritefile(const char* fileName, const char* data){
    return vp_Mwritefile(fileName, data, "w");
}

vp_result vp_Awritefile(const char* fileName, const char* data){
    return vp_Mwritefile(fileName, data, "a");
}

size_t vp_readfileM(char* ptr, const char* fileName, const size_t bufSize, const char* mode){
    FILE *fptr = fopen(fileName, mode);

    if (!fptr){
        ptr[0] = '\0';
        return 0;
    }

    size_t bytesRead = fread(ptr, 1, bufSize - 1, fptr);
    ptr[bytesRead] = '\0';

    fclose(fptr);
    return bytesRead;
}

size_t vp_readfileS(char* ptr, const char* fileName, const size_t bufSize){
    return vp_readfileM(ptr,fileName,bufSize,"r");
}

size_t vp_getFileSize(const char* fileName){
    FILE *fptr = fopen(fileName, "rb");

    if (!fptr){
        return 0;
    }
    fseek(fptr, 0L, SEEK_END);
    size_t size = ftell(fptr);
    fclose(fptr);
    return size;
}

/* =========================
   Math
   ========================= */

double vp_mind(double x, double b){
    return x < b ? x : b;
}

int vp_mini(int x, int b){
    return x < b ? x : b;
}

double vp_maxd(double x, double b){
    return x > b ? x : b;
}

int vp_maxi(int x, int b){
    return x > b ? x : b;
}

double vp_clampd(double x, double min, double max){
    return vp_maxd(min, vp_mind(x, max));
}

int vp_clampi(int x, int min, int max){
    return vp_maxi(min, vp_mini(x, max));
}

double vp_lerpd(double a, double b, double weight){
    return a + (b - a) * weight;
}

double vp_lerpd_c(double a, double b, double weight){
    return vp_lerpd(a, b, vp_clampd(weight, 0.0, 1.0));
}

int vp_floatcmp(float a, float b, double deadPoint){
    return (fabsf(a - b) < deadPoint);
}

int vp_doublecmp(double a, double b, double deadPoint){
    return (fabs(a - b) < deadPoint);
}


/*
double vp_calculate_pi(int iterations)
{
    double a = 1.0;
    double b = 1.0 / sqrt(2.0);
    double t = 0.25;
    double p = 1.0;

    for (int i = 0; i < iterations; i++) {
        double a_next = (a + b) / 2.0;
        double b_next = sqrt(a * b);
        double t_next = t - p * (a - a_next) * (a - a_next);
        double p_next = 2.0 * p;

        a = a_next;
        b = b_next;
        t = t_next;
        p = p_next;
    }

    return ((a + b) * (a + b)) / (4.0 * t);
}
*/

typedef struct vp_strPair{
    const char* a;
    const char* b;
} vp_strPair_t;

int vp_isNumber(const char* s) {
    if (s == NULL || s[0] == '\0' || isspace(s[0])) return 0;
    
    char* endPtr;
    strtof(s, &endPtr);
    
    return *endPtr == '\0';
}


/* =========================
   INI functions
   ========================= */

#ifndef __VAPOR_H_EXC_INI

#define __VP_INIBUF_ADD 1024
#define __VP_INIKEYSIZE 2062

enum VP_iniType{
    vp_num,
    vp_text,
    vp_bool
};

typedef struct vp_iniObj{
    char key[__VP_INIKEYSIZE];
    // Not the greatest code but i think it could be worse
    char value[__VP_INIKEYSIZE];
    // Currently unused
    //short dataType;
} vp_iniObj_t;

char* iniBuffer;
size_t iniBufSize = 0;
vp_iniObj_t *vp_iniVariables;
size_t vp_iniVariableAmount = 0;
vp_result vp_iniFreed=vp_OK;

void vp_iniMakeBuffer(size_t size){
    if (iniBuffer != NULL)
        vp_iniClean();

    iniBufSize = size;
    
    vp_iniVariables = static_cast<vp_iniObj*>(malloc(sizeof(vp_iniObj_t) * __VP_INIBUF_ADD));
    iniBuffer = static_cast<char*>(malloc(size + 1)); 
    
    vp_iniFreed = vp_FAIL;
}
void vp_iniClean(){
    if (vp_iniFreed){
        return;
    }
    iniBufSize = 0;
    free(iniBuffer);
    free(vp_iniVariables);
    vp_iniFreed=vp_OK;
}

void vp_iniUseFile(const char* fileName){
    const size_t fileSize = vp_getFileSize(fileName);
    if (fileSize < 1){
        return;
    }
    char sourceBuffer[fileSize + 32];
    vp_iniMakeBuffer(fileSize);
    vp_readfileM(sourceBuffer,fileName,fileSize + 16, "rb");
    strcpy(iniBuffer, sourceBuffer);

}

//#define __VP_INI_DEBUG

void vp_iniPrepareBuffer(){
    // safety
    #ifndef __VP_ININOCLEANUP
    atexit(vp_iniClean);
    #endif
    
    // idk how much tokens i need
    char* tokens[__VP_INIBUF_ADD] = {0};
    size_t maxSize = iniBufSize + __VP_INIBUF_ADD;
    char curToken[__VP_INIBUF_ADD]="";
    size_t curT=0;
    // Lexer
    for (size_t i = 0; i < iniBufSize; i++){
        if (i > maxSize || i == maxSize){
            vp_throw("During parsing: Couldn't prepare buffer, preparation buffer overflow, try increasing __VP_INIBUF_ADD.", -1);
        }
        if (iniBuffer[i] == ' ' || iniBuffer[i] == '\n' || iniBuffer[i] == '=') {
            if (curToken[0] != '\0') {
                tokens[curT] = strdup(curToken);
                curT++;
                curToken[0] = '\0';
            }
            if (iniBuffer[i] == '=') {
                tokens[curT] = strdup("=");
                curT++;
            }
        }
        else
        {
            if (iniBuffer[i] == ';') {
                while (i + 1 < iniBufSize && iniBuffer[i + 1] != '\n') {
                    i++;
                }
                continue;
            }
            char str[2];
            str[0] = iniBuffer[i];
            str[1] = '\0';
            strncat(curToken,str,__VP_INIBUF_ADD-1);
        }
    }
    if (curToken[0] != '\0') {
        tokens[curT] = strdup(curToken);
        curT++;
    }
    // Sorting into variables / categories
    int assigning=0;
    size_t curVar=0;
    char curCategory[2048]="";
    for (size_t i = 0; i < curT; i++){
        if (tokens[i][0] == '[') {
            char* closeBracket = strchr(tokens[i], ']');
            
            if (closeBracket) {
                size_t length = closeBracket - (tokens[i] + 1);
                
                strncpy(curCategory, tokens[i] + 1, length);
                
                curCategory[length] = '\0';
            }
            #ifdef __VP_INI_DEBUG
            printf("category: %s\n", curCategory);
            #endif
        }
        if (tokens[i][0]=='='){
            assigning = 1;
            continue;
        }
        if (!assigning){
            int isGlobal = (curCategory[0] == '\0' || strcmp(curCategory, "(global)") == 0);
            if (isGlobal) {
                snprintf(vp_iniVariables[curVar].key, __VP_INIKEYSIZE, "__global_%s", tokens[i]);
            } else {
                snprintf(vp_iniVariables[curVar].key, __VP_INIKEYSIZE, "%s_%s", curCategory, tokens[i]);
            }
            continue;
        }
        else
        {
            strcpy(vp_iniVariables[curVar].value, tokens[i]);
            #ifdef __VP_INI_DEBUG
            printf("SUCCESS: Bound [%s] to value [%s]\n", 
                vp_iniVariables[curVar].key, 
                vp_iniVariables[curVar].value);
            #endif
            curVar++;
            assigning=0;
            continue;
        }
    }
    vp_iniVariableAmount = curVar;
    for (size_t i = 0; i < curT; i++) {
        free(tokens[i]);
    }
}

const char* vp_iniGetString(const char* key, const char* section){
    if (section[0]=='\0'){
        section = "__global";
    }
    char rkey[__VP_INIKEYSIZE];
    snprintf(rkey, __VP_INIKEYSIZE, "%s_%s", section, key);
    for (size_t i = 0; i < vp_iniVariableAmount; i++){
        if (strcmp(vp_iniVariables[i].key,rkey)==0){
            return vp_iniVariables[i].value;
        }
    }
    char message[__VP_INIKEYSIZE * 2 + 64];
    sprintf(message, "INI Warning: '%s' might not exist in '%s'.", key, section);
    vp_throw(message, 0);
    return "";
}
float vp_iniGetNumber(const char* key, const char* section){
    const char* value = vp_iniGetString(key,section);
    if (vp_isNumber(value))
        return atof(value);
    else
        return 0.0;
}
int vp_iniGetBool(const char* key, const char* section){
    const char* result = vp_iniGetString(key,section);
    if (strcmp(result, "true")==0){
        return 1;
    }
    if (strcmp(result, "false")==0){
        return 0;
    }
    return 0;
}


#endif