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

#define SRC_LIMIT_BYTES 200000 // buffer size
#define ROW_LIMIT 1000 // max number of rows in the lexeme table and token list
#define ID_MAX_LEN 11 // max length of identifiers exlcude null char
#define NUM_MAX_LEN 5 // max length of numbers

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
    char id[ID_MAX_LEN + 1]; // to include the /0
    int num;
} TokenRow;

// Scanner context
typedef struct {
    const char *srcCode; // full source buffer
    size_t len; // number of bytes in srcBuffer
    size_t pos; // current index in srcBuffer

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

// Key words table - immutable
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

// Get keywords count
static const int KW_COUNT = (int)(sizeof(kw_table)/sizeof(kw_table[0]));

//Forward Declarations
static int kw_lookup(const char *s);

static int is_letter(int c);
static int is_digitc(int c);
static int at_end(const ScanCtx *scanner);
static int peekc(const ScanCtx *scanner, size_t k);
static void step(ScanCtx *scanner, size_t k);

static void emit_lex(ScanCtx *scanner, const char *lx, TokenType tk);
static void emit_tok_simple(ScanCtx *scanner, TokenType tk);
static void emit_tok_ident(ScanCtx *scanner, const char *id);
static void emit_tok_number(ScanCtx *scanner, int value);

static void skip_whitespace_and_comments(ScanCtx *scanner);
static void scan_ident_or_kw(ScanCtx *scanner);
static void scan_number(ScanCtx *scanner);
static void scan_symbol_or_error(ScanCtx *scanner);

// Find keywords
static int kw_lookup(const char *s){

    // loop through each keyword
    for(int i=0; i < KW_COUNT; i++)
        // Checker if scanner matches keyword, if it does return token
        if (strcmp(s, kw_table[i].kw ) ==0)
            return kw_table[i].tok;
    return 0;
}

// Understood
static int is_letter(int c){
    return isalpha((unsigned char) c); // returns true if char is a letter
}

// Understood
static int is_digitc(int c){
    return isdigit((unsigned char) c); // returns true if char is a letter
}

// Understood
static int at_end(const ScanCtx *scanner){
    return scanner->pos >= scanner->len; // checks if we have reached EOF
}

// Understood
static int peekc(const ScanCtx *scanner, size_t k){
    size_t j = scanner->pos + k; // get position of next character

    // if char is within buffer, return it
    if (j < scanner->len)
        return (unsigned char) scanner->srcCode[j]; // return char at position j from buffer
    else
        return 0; //return 0 if we reach EOF
}

// Understood
static void step(ScanCtx *scanner, size_t k){
    scanner->pos += k; // move position by k
}

// helpers to emit lexeme table and token list rows
static void emit_lex(ScanCtx *scanner, const char *lx, TokenType tk){
    if (scanner->lex_n >= ROW_LIMIT)
        return;
    strncpy(scanner->lex[scanner->lex_n].lexeme, lx, sizeof(scanner->lex[0].lexeme)-1);
    scanner->lex[scanner->lex_n].lexeme[sizeof(scanner->lex[0].lexeme)-1] = '\0';
    scanner->lex[scanner->lex_n].token = tk;
    scanner->lex_n++;
}

static void emit_tok_simple(ScanCtx *scanner, TokenType tk){
    if(scanner->tok_n >= ROW_LIMIT)
        return;
    scanner->tok[scanner->tok_n].token = tk;
    scanner->tok[scanner->tok_n].has_attr = 0;
    scanner->tok_n++;
}

static void emit_tok_ident(ScanCtx *scanner, const char *id){
    if(scanner->tok_n >= ROW_LIMIT)
        return;
    scanner->tok[scanner->tok_n].token = identsym;
    scanner->tok[scanner->tok_n].has_attr = 1;
    strncpy(scanner->tok[scanner->tok_n].id, id, ID_MAX_LEN);
    scanner->tok[scanner->tok_n].id[ID_MAX_LEN] = '\0';
    scanner->tok_n++;
}

static void emit_tok_number(ScanCtx *scanner, int value){
    if(scanner->tok_n >= ROW_LIMIT)
        return;
    scanner->tok[scanner->tok_n].token = numbersym;
    scanner->tok[scanner->tok_n].has_attr = 1;
    scanner->tok[scanner->tok_n].num = value;
    scanner->tok_n++;
}

// Understood
// scanning functions
// skip spaces/tabs/newlines/comments.
static void skip_whitespace_and_comments(ScanCtx *scanner){

    while(1){
        //whitespace
        while(!at_end(scanner)){
            int c = peekc(scanner,0); // returns top most char from buffer
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r'){
                step(scanner,1);
            } else {
                break;
            }
        }
        //block comment
        if (!at_end(scanner) && peekc(scanner,0) == '/' && peekc(scanner,1) == '*'){
            step(scanner,2); // move position by 2 to be in front of /*
            int closed = 0; // flag
            while(!at_end(scanner)){
                if (peekc(scanner,0) == '*' && peekc(scanner,1) == '/'){
                    step(scanner,2); // move position by 2 to be in front of /*
                    closed = 1; // set flag to 1
                    break;
                }
                step(scanner,1); // increment position until */ is found
            }
            if(!closed){
                //Unterminated comment: report and allow main loop to finish at EOF.
                printf("Error: Unterminated comment\n");
                return;
            }
            continue; // loop again to absorb more whitespace and comments
        }
        // not whitespace or comment
        break;
    }
}

/* scan an identifier or reserved word 
- Start with letter, continue with letters/digits
- If length > 11, then errer, discard lexeme (no tokens emitted)
- Else if matches keyword, emit keyword token
- Else, emit identsym with attribute (the spelling)
*/
static void scan_ident_or_kw(ScanCtx * scanner) {

    // while(!at_end(scanner)){
    //     LexemeRow * l;
    //     char getIdentifier[ID_MAX_LEN];
    //     int c = peekc(scanner,0); // returns top most char from buffer
    //     if (is_letter(c)){
    //         step(scanner,1);
    //         if (is_letter(peekc(scanner, 0)) || is_digitc(peekc(scanner, 0))) {
    //             if (step(scanner, 1) == " ") {
    //                 for (int i = 0; i < scanner->pos + 1; i++) {
    //                     l->lexeme[i] = 

    //                 }
    //             }

    //         else if (kw_lookup(c))
    //         }
         
    //     } else {
    //         break;
    //     }
    // }

}

/* Scan a number
- One or more digits
- If more than 5 digits, then error, discard lexeme (no tokens emitted)
- Else emit numbersym with integer value
- If digits are followed by a letter, this code will stop at the
    first letter. the subsequent identifier will be tokenized in the main loop
*/
static void scan_number(ScanCtx *scanner){

}

/*Scanning symbols and operators, and two-character look ahead cases
- ":=" is becomessym. If ":" not followed by "=", print error
- "<=" "<>" "<", leq neq less
- ">=" ">", geq gtr
- All single character operators like: + - * / = ( ) , ; .
- Any other character, invalid symbol and print error, consume one character.
*/
static void scan_symbol_or_error(ScanCtx *scanner){

}








int main(int argc, char **argv){

    if(argc != 2) return 1;

    // First read entire file
    FILE *fp = fopen(argv[1],"r");
    if (fp == NULL) {
        printf("File not found!");
        return 1;
    }

    static char buffer[SRC_LIMIT_BYTES]; // local buffer to read the file into
    size_t n = fread(buffer, 1, SRC_LIMIT_BYTES, fp); // read the file into the buffer and store size of bytes read
    fclose(fp);
    buffer[n]='\0'; // add null terminator to the buffer

    // Then print source program as read
    printf("Source Program:\n\n%s\n", buffer);

    // Then initialize scanner
    ScanCtx scanner;
    scanner.srcCode = buffer; // SrcCode points to the buffer
    scanner.len = n; // set the length of the source code to the # of bytes read
    scanner.pos = 0; // set the position to 0
    scanner.lex_n = 0; // set the number of lexeme table rows to 0
    scanner.tok_n = 0; // set the number of token list rows to 0

    // Then main scanner loop, going left to right over the file
    while(!at_end(&scanner)) {

        // To have scanner to not have spaces/tabs/newlines/comments.
        skip_whitespace_and_comments(&scanner); 

        if(at_end(&scanner)) // checks position of scanner / check if we reach EOF, break
            break;

        int c = peekc(&scanner,0); // reset position and get char at top

        // if char is a letter, look for identifier or keyword
        if(is_letter(c)){
            scan_ident_or_kw(&scanner);
            continue;
        }
        if (is_digitc(c)){
            scan_number(&scanner);
            continue;
        }
        scan_symbol_or_error(&scanner);
    }

    // Then print lexeme table
    printf("\nLexeme Table:\n");
    printf("lexeme token type\n");
    for(int i=0; i < scanner.lex_n; i++){
        printf("%-7s %d\n", scanner.lex[i].lexeme, scanner.lex[i].token);
    }

    // lastly print token list
    printf("\nToken List:\n");
    for(int i=0; i<scanner.tok_n; i++){
        TokenRow *t = &scanner.tok[i];
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