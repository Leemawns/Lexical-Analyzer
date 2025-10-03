/*
Assignment :
lex - Lexical Analyzer for PL /0

Author : Eric Lemons, Leonardo Faife

Language : C ( only )

To Compile :
    gcc - O2 - std = c11 -o lex lex . c

To Execute ( on Eustis ):
    ./ lex < input file >

where :
    <input file> is the path to the PL /0 source program

Notes :
- Implement a lexical analyser for the PL /0 language.
- The program must detect errors such as
    - numbers longer than five digits
    - identifiers longer than eleven characters
    - invalid characters.
- The output format must exactly match the specification.
- Tested on Eustis.

Class : COP 3402 - System Software - Fall 2025

Instructor : Dr . Jie Lin

Due Date : Friday , October 3 , 2025 at 11:59 PM ET
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SRC_LIMIT_BYTES 200000
#define ROW_LIMIT 1000
#define ID_MAX_LEN 11
#define NUM_MAX_LEN 5

// Token enumeration given from the hw pdf
typedef enum {
    skipsym = 1 , // Skip / ignore token
    identsym , // Identifier
    numbersym , // Number
    plussym , // +
    minussym , // -
    multsym , // *
    slashsym , // /
    eqsym , // =
    neqsym , // <>
    lessym , // <
    leqsym , // <=
    gtrsym , // >
    geqsym , // >=
    lparentsym , // (
    rparentsym , // )
    commasym , // ,
    semicolonsym , // ;
    periodsym , // .
    becomessym , // :=
    beginsym , // begin
    endsym , // end
    ifsym , // if
    fisym , // fi
    thensym , // then
    whilesym , // while
    dosym , // do
    callsym , // call
    constsym , // const
    varsym , // var
    procsym , // procedure
    writesym , // write
    readsym , // read
    elsesym , // else
    evensym // even
} TokenType ;

//Output row structures

typedef struct {
    char lexeme[32]; // lexeme as printed in the Lexeme Table
    TokenType token; // token code for lexem table row
} LexemeRow;

typedef struct {
    TokenType token; // token code for token list
    int has_attr; // 1 if id/number requires attribute printing
    char id[ID_MAX_LEN + 1];
    int num;
} TokenRow;

// Scanner context

typedef struct {
    const char *src; // full source buffer
    size_t len; // number of bytes in src
    size_t pos; // current index in src

    //lexeme table rows
    LexemeRow lex[ROW_LIMIT];
    int lex_n;

    //token list rows
    TokenRow tok[ROW_LIMIT];
    int tok_n;
} ScanCtx;

// Reserved word table, converst string to token
typedef struct {
    const char *kw;
    TokenType tok;
} KwEntry;

static const KwEntry kw_table[] = {
    {"begin", beginsym},
    {"end", endsym},
    {"if", ifsym},
    {"fi", fisym},
    {"then", thensym},
    {"while", whilesym},
    {"do", dosym},
    {"call", callsym},
    {"const", constsym},
    {"var", varsym},
    {"procedure", procsym},
    {"write", writesym},
    {"read", readsym},
    {"else", elsesym},
    {"even", evensym},
};

static const int KW_COUNT = (int)(sizeof(kw_table)/sizeof(kw_table[0]));

//Forward Declarations
static void bail_not_found(void);
static void bail_not_read(void);
static int kw_lookup(const char *s);

static int is_letter(int c);
static int is_digitc(int c);
static int at_end(const ScanCtx *S);
static int peekc(const ScanCtx *S, size_t k);
static void step(ScanCtx *S, size_t k);

static void emit_lex(ScanCtx *S, const char *lx, TokenType tk);
static void emit_tok_simple(ScanCtx *S, TokenType tk);
static void emit_tok_ident(ScanCtx *S, const char *id);
static void emit_tok_number(ScanCtx *S, int value);

static void skip_whitespace_and_comments(ScanCtx *S);
static void scan_ident_or_kw(ScanCtx *S);
static void scan_number(ScanCtx *S);
static void scan_symbol_or_error(ScanCtx *S);

//utility and/or low level functions
Static void bail_not_found(void) { // Print "File not found" if wrong argc
    printf("File not found\n"); 
    exit(1);
}
Static void bail_not_read(void) { // Print "File not read" if fopen fails
    printf("File not read\n"); 
    exit(1);
}

static int kw_lookup(const char *s){
    for(int i=0; i<KW_COUNT; i++)
        if (strcmp(s,kw_table[i].kw)==0)
            return kw_table[i].tok;
    return 0;
}

static int is_letter(int c){
    return isalpha((unsigned char)c);
}
static int is_digitc(int c){
    return isdigit((unsigned char)c);
}

static int at_end(const ScanCtx *S){
    return S->pos >= S->len;
}

static int peekc(const ScanCtx *S, size_t k){
    size_t j = S->pos + k;
    if (j < S->len)
        return (unsigned char)S->src[j];
    else
        return 0;
}

static void step(ScanCtx *S, size_t k){
    S->pos += k;
}

// helpers to emit lexeme table and token list rows
static void emit_lex(ScanCtx *S, const char *lx, TokenType tk){
    if (S->lex_n >= ROW_LIMIT)
        return;
    strncpy(S->lex[S->lex_n].lexeme, lx, sizeof(S->lex[0].lexeme)-1);
    S->lex[S->lex_n].lexeme[sizeof(S->lex[0].lexeme)-1] = '\0';
    S->lex[S->lex_n].token = tk;
    S->lex_n++;
}

static void emit_tok_simple(ScanCtx *S, TokenType tk){
    if(S->tok_n >= ROW_LIMIT)
        return;
    S->tok[S->tok_n].token = tk;
    S->tok[S->tok_n].has_attr = 0;
    S->tok_n++;
}

static void emit_tok_ident(ScanCtx *S, const char *id){
    if(S->tok_n >= ROW_LIMIT)
        return;
    S->tok[S->tok_n].token = identsym;
    S->tok[S->tok_n].has_attr = 1;
    strncpy(S->tok[S->tok_n].id, id, ID_MAX_LEN);
    S->tok[S->tok_n].id[ID_MAX_LEN] = '\0';
    S->tok_n++;
}

static void emit_tok_number(ScanCtx *S, int value){
    if(S->tok_n >= ROW_LIMIT)
        return;
    S->tok[S->tok_n].token = numbersym;
    S->tok[S->tok_n].has_attr = 1;
    S->tok[S->tok_n].num = value;
    S->tok_n++;
}

// scanning functions
// skip spaces/tabs/newlines/comments.
static void skip_whitespace_and_comments(ScanCtx *S){
    while(1){
        //whitespace
        while(!at_end(S)){
            int c = peekc(S,0);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r'){
                step(S,1);
            } else {
                break;
            }
        }
        //block comment
        if (!at_end(S) && peekc(S,0) == '/' && peekc(S,1) == '*'){
            step(S,2);
            int closed = 0;
            while(!at_end(S)){
                if (peekc(S,0) == '*' && peekc(S,1) == '/'){
                    step(S,2);
                    closed = 1;
                    break;
                }
                step(S,1);
            }
            if(!closed){
                //Unterminated comment: report and allow main loop to finish at EOF.
                printf("Error: Unterminated comment\n");
                return;
            }
            continue; // loop again to absorb more whitespace and comments
        }
        //not whitespace or comment
        break;
    }
}

/* scan an identifier or reserved word 
- Start with letter, continue with letters/digits
- If length > 11, then errer, discard lexeme (no tokens emitted)
- Else if matches keyword, emit keyword token
- Else, emit identsym with attribute (the spelling)
*/
static void scan_ident_or_kw(ScanCtx *S){

}

/* Scan a number
- One or more digits
- If more than 5 digits, then error, discard lexeme (no tokens emitted)
- Else emit numbersym with integer value
- If digits are followed by a letter, this code will stop at the
    first letter. the subsequent identifier will be tokenized in the main loop
*/
static void scan_number(ScanCtx *S){

}

/*Scanning symbols and operators, and two-character look ahead cases
- ":=" is becomessym. If ":" not followed by "=", print error
- "<=" "<>" "<", leq neq less
- ">=" ">", geq gtr
- All single character operators like: + - * / = ( ) , ; .
- Any other character, invalid symbol and print error, consume one character.
*/
static void scan_symbol_or_error(ScanCtx *S){

}








int main(int argc, char **argv){
    if(argc!=2) 
        bail_not_found();

    // First read entire file
    FILE *fp = fopen(argv[1],"rb");
    if(!fp) 
        bail_not_read();

    static char buffer[SRC_LIMIT_BYTES];
    size_t n = fread(buffer, 1, SRC_LIMIT_BYTES, fp);
    fclose(fp);
    buffer[n]='\0';

    // Then print source program as read
    printf("Source Program:\n\n%s\n", buffer);

    // Then initialize scanner
    ScanCtx S;
    S.src = buffer;
    S.len = n;
    S.pos = 0;
    S.lex_n = 0;
    S.tok_n = 0;

    // Then main scanner loop, going left to right over the file
    while(!at_end(&s)) {
        skip_whitespace_and_comments(&S);
        if(at_end(&S))
            break;

        int c = peekc(&S,0);

        if(is_letter(c)){
            scan_ident_or_kw(&S);
            continue;
        }
        if (is_digitc(c)){
            scan_number(&S);
            continue;
        }
        scan_symbol_or_error(&S);
    }

    // Then print lexeme table
    printf("\nLexeme Table:\n");
    printf("lexeme token type\n");
    for(int i=0; i<S.lex_n; i++){
        printf("%-7s %d\n", S.lex[i].lexeme, S.lex[i].token);
    }

    // lastly print token list
    printf("\nToken List:\n");
    for(int i=0; i<S.tok_n; i++){
        TokenRow *t = &S.tok[i];
        if(t->token == identsym){
            printf("%d %s ", identsym, t->id);
        } else if(t->token == numbersym){
            printf("%d %d ", numbersym, t->num);
        } else {
            printf("%d ", t->token);
        }
    }
    printf("\n");
    return 0;
}