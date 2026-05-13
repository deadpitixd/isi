#define __VP_DISABLE_ERRORLOG
#include "vapor.h"
#include <stdbool.h>
#include <time.h>

#define workDir "vapor/"
#define testtxtpath workDir "test/test.txt"
#define testinipath workDir "test/test.ini"

// Disables the descriptions of the test on their beginning and end.
#define VP_TEST_NOSHOW_DESC

int passed;
int failed;
int totalTest;

void printLog(int status, const char* testDesc){
    if (status==1) passed += 1; else failed += 1;
    printf("[%s%s%s] %s\n",status==1 ? vp_green  : vp_red, status==1 ? "Passed" : "Failed", vp_reset ,  testDesc);
}

int checkDependencies()
{
    int returnNum = 1;
    // Doesn't use vapor functions for safety
    // checks for test.txt
    FILE *fptr = fopen(testtxtpath, "r");
    if (!fptr){
        printf("Couldn't find required dependency 'test.txt' in %s.\n", testtxtpath);
        returnNum = 0;
    }
    fclose(fptr);
    // end
    return returnNum;
}
void funcWrapper(void (*func)(void), const char* str){
    #ifndef VP_TEST_NOSHOW_DESC
    printf("'%s%s%s' test.\n",vp_blue,str,vp_reset);
    clock_t start = clock();
    #endif
    func();
    #ifndef VP_TEST_NOSHOW_DESC
    clock_t end = clock();
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("'%s' test time: %s%f%s seconds%s\n",str,vp_blue,time_taken,vp_green,vp_reset);
    #endif
}

void fileHandlingTest(){
    char buffer[1024] = "test";
    // File should not exist
    vp_readfileS(buffer, "null filename", 1024);
    printLog(buffer[0] == '\0', "Empty filename handling.");
    strcpy(buffer, "");
    vp_readfileS(buffer, testtxtpath, 1024);
    printLog(strcmp(buffer,"text")==0, "File reading.");
    vp_RWwritefile(testtxtpath, "rewrittenfiletest");
    vp_readfileS(buffer, testtxtpath, 1024);
    printLog(strcmp(buffer, "rewrittenfiletest")==0, "File writing w/ mode + reading.");

    vp_RWwritefile(testtxtpath, "text");
}
void colorTest(){
    // test if everything returns 255 128 255
    vp_rgb_t x = {255,128,255};
    printLog(x.r == 255 && x.g == 128 && x.b == 255? 1 : 0, "RGB Reading.");
    //HSV convertion.
    printLog(vp_rgbnormalize(x.r) == 1.0 && vp_rgbnormalize(x.g) == x.g / 255.0 ? 1 : 0, "RGB to HSV conversion.");
}
void iniTest(){
    vp_iniUseFile(testinipath);
    vp_iniPrepareBuffer();

    printLog(strcmp(vp_iniGetString("Name","Player"),"Vapor")==0 ? 1 : 0, "Accessing strings.");
    printLog(vp_floatcmp(vp_iniGetNumber("Health","Player"),100.0, vp_EPSILON) ? 1 : 0, "Accessing numbers.");
    printLog(vp_iniGetBool("Fullscreen","Graphics")==1 ? 1 : 0, "Accessing booleans.");
    printLog(vp_iniGetNumber("Health","Pslayer")==0.0 ? 1 : 0, "Error checking.");

    vp_iniClean();
}

int main(){
    if (checkDependencies() == 0) return -1;
    clock_t start = clock();
    // File library
    funcWrapper(fileHandlingTest, "File Handling");
    // Color library
    funcWrapper(colorTest, "Color Handling");
    // INI lib
    funcWrapper(iniTest, "INI File Handling");
    clock_t end = clock();
    totalTest = failed + passed;
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Tests finished in '%s%f seconds%s'. %s%d %sPassed, %s%d %sFailed%s, Success rate: %f .\n[%s%d%s] Tests total.\n",vp_blue, time_taken, vp_reset, vp_blue, passed, vp_bright_green
        ,vp_blue, failed,vp_red, vp_reset, passed == 0 || failed == 0 ? passed > failed ? passed : failed : (double)passed / (double)failed ,vp_blue, totalTest,vp_reset);
}