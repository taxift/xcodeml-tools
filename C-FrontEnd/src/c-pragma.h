/* 
 * $TSUKUBA_Release: Omni OpenMP Compiler 3 $
 * $TSUKUBA_Copyright:
 *  PLEASE DESCRIBE LICENSE AGREEMENT HERE
 *  $
 */
/**
 * \file c-pragma.h
 */

#ifndef _C_PRAGMA_H_
#define _C_PRAGMA_H_

#include <strings.h>
#include "c-expr.h"
#include "c-lexyacc.h"

#define MAX_TOKEN_LEN 65536

#define PG_CONST 'N'
#define PG_ERR 'E'
#define PG_IDENT 'I'
#define PG_ANDAND 'A'
#define PG_OROR 'O'
#define PG_EQEQ  'P'
#define PG_NEQ 'Q'
#define PG_LTEQ 'T'
#define PG_GTEQ 'U'
#define PG_LTLT 'V'
#define PG_GTGT 'W'
#define PG_STREF 'S'
#define PG_COL2 'C'

/**
 * \brief
 * pragma kinds
 */
typedef enum {
    PK_NOT_PARSABLE,
    PK_PACK,
    PK_OMP,  // OpenMP
    PK_XMP,  // XcalableMP
    PK_ACC,  // OpenACC
} CPragmaKind;

extern CExpr*  pg_parse_expr();
extern void    pg_get_token();

extern char    *pg_cp;
extern char    pg_tok;
extern char    pg_tok_buf[];
extern CExpr*  pg_tok_val;

#define PG_IS_IDENT(name) (pg_tok == 'I' && strcasecmp(name,pg_tok_buf)==0)

extern CExpr*           lexParsePragma(char *p, int *token);
extern CExpr*           lexAllocDirective(const char *name, CDirectiveTypeEnum type);
extern CExpr*           lexLinemarker(const char *name, CDirectiveTypeEnum type, int *flag);

extern void lexPragmaInfo(CExpr *,CExpr *);

extern char* lexSkipWordP(char *p);
extern char* lexSkipWord(char *p);
extern char* lexSkipSpaceP(char *p);
extern char* lexSkipSpace(char *p);
extern char* lexSkipSharp(char *p);
extern char* lexConvertUnderscorePragma(char *p);

typedef struct pg_token_context {
  char *head;
  char *token;
  size_t token_len;
  size_t num_peek;
} pg_token_context_t;

int pg_token_context_init(pg_token_context_t* ctx);
int pg_peek_token(pg_token_context_t *ctx);
int pg_seek_token(pg_token_context_t *ctx);

#define PG_TOKEN_STR_IS_EQUAL(_token, _token_len, _str)     \
  (strncasecmp((_token), (_str),                            \
               strlen(_str) > (_token_len) ?                \
               strlen(_str) : (_token_len)) == 0)

#define PG_TOKEN_CONTEXT_STR_IS_EQUAL(_ctxp, _str)             \
  (PG_TOKEN_STR_IS_EQUAL((_ctxp)->token, (_ctxp)->token_len , (_str)))

#endif // _C_PRAGMA_H_
