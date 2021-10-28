/* 
 * $TSUKUBA_Release: Omni OpenMP Compiler 3 $
 * $TSUKUBA_Copyright:
 *  PLEASE DESCRIBE LICENSE AGREEMENT HERE
 *  $
 */
/**
 * \file c-main.c
 * C_Front main
 */

#if !defined(__CMAKE_BUILD__) || defined(NDEBUG)

#include "c-expr.h"
#include "c-comp.h"
#include "c-lexyacc.h"
#include "c-option.h"
#include "c-ptree.h"

extern unsigned int s_dispParseTree;


/**
 * C_Front main.
 */


/**
 * \brief
 * C_Front main function
 */
int
main(int argc, char** argv)
{
    if(procOptions(argc, argv) == 0)
        return CEXIT_CODE_ERR;

    setTimestamp();
    initStaticData();
    FILE *fpIn, *fpOut;

    if(s_inFile) {
        fpIn = fopen(s_inFile, "r");
        if(fpIn == NULL) {
            perror(CERR_501);
            return CEXIT_CODE_ERR;
        }
    } else {
        fpIn = stdin;
    }

    int convertedFileId = 0;

    CExpr *expr = execParse(fpIn);
    if (s_dispParseTree & 0x1)
        dispParseTree(stderr, expr, "execParse");

    if(s_inFile)
        fclose(fpIn);

    if(s_hasError || expr == NULL)
        goto end;

    reduceExpr(expr);
    //dispParseTree(stderr, expr, "reduceExpr");

    if(s_hasError)
        goto end;

    if(s_verbose)
        printf("compiling ...\n");

    compile(expr);
    if (s_dispParseTree & 0x1)
      dispParseTree(stderr, expr, "compile");

    if(s_hasError)
        goto end;

    convertSyntax(expr);
    //dispParseTree(stderr, expr, "convertSyntax");

    collectTypeDesc(expr);
    if (s_dispParseTree & 0x1)
        dispParseTree(stderr, expr, "collectTypeDesc");

    if(s_hasError)
        goto end;

    if(s_outFile) {
        fpOut = fopen(s_outFile, "w");
        if(fpOut == NULL) {
            perror(CERR_502);
            return CEXIT_CODE_ERR;
        }
    } else {
        fpOut = stdout;
    }

    convertFileIdToNameTab();
    convertedFileId = 1;
    outputXcodeML(fpOut, expr);

    if(s_outFile)
        fclose(fpOut);

  end:

    if(s_hasError || s_hasWarn) {
        CCOL_SL_REVERSE(&s_errorList);
        if(convertedFileId == 0)
            convertFileIdToNameTab();
        printErrors(stderr);
    }

    fflush(stdout);
    fflush(stderr);

    if(s_verbose)
        printf("completed\n");

    return s_hasError ? CEXIT_CODE_ERR : CEXIT_CODE_OK;
}

#endif //!defined(__CMAKE_BUILD__) || defined(NDEBUG)

