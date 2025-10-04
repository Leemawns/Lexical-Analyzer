/*
Assignment :
lex - Lexical Analyzer for PL /0

Author : Eric Lemons, Leonardo Faife

Language : C ( only )

To Compile :
    gcc -O2 -std=c11 -o lex lex.c

To Execute ( on Eustis ):
    ./lex <input file>

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

// used for bufferes and lexemes
#define SRC_LIMIT_BYTES 200000
#define LEXEME_CAP 1000
#define TOKEN_CAP 1000
#define ID_MAX_LEN 11
#define NUM_MAX_LEN 5

// token codes (from the assignment pdf)
typedef enum {
    skipsym = 1, // Skip / ignore token
    identsym, // Identifier
    numbersym, // Number
    plussym, // +
    minussym, // -
    multsym, // *
    slashsym, // /
    eqsym, // =
    neqsym, // <>
    lessym, // <
    leqsym, // <=
    gtrsym, // >
    geqsym, // >=
    lparentsym, // (
    rparentsym, // )
    commasym, // ,
    semicolonsym, // ;
    periodsym, // .
    becomessym, // :=
    beginsym, // begin
    endsym, // end
    ifsym, // if
    fisym, // fi
    thensym, // then
    whilesym, // while
    dosym, // do
    callsym, // call
    constsym, // const
    varsym, // var
    procsym, // procedure
    writesym, // write
    readsym, // read
    elsesym, // else
    evensym // even
} TokenType ;

// reserved words
static const char *resWords[] = {
    "begin",
    "end",
    "if",
    "fi",
    "then",
    "while",
    "do",
    "call",
    "const",
    "var",
    "procedure",
    "write",
    "read",
    "else",
    "even"
};
// token mapping for the reserved words
static const TokenType resWordTokens[] = {
    beginsym, 
    endsym, 
    ifsym, 
    fisym, 
    thensym, 
    whilesym, 
    dosym,
    callsym, 
    constsym, 
    varsym, 
    procsym, 
    writesym, 
    readsym, 
    elsesym, 
    evensym
};
// number of reserved words:
static const int numResWords = (int)(sizeof(resWords)/sizeof(resWords[0]));

// The single character token lookup table (the ASCII one)
static TokenType singleCharTokens[256];

// rows for the lexeme table
typedef struct {
    char lexeme[32];
    TokenType token;
    //error handling:
    int is_error;
    char errortext[32]; // this for things like "Number too long" and "Identifier too long"
} LexemeRow;

// entries for the token list
typedef struct {
    TokenType token;
    int has_attr;         // 1 if id/number
    char id[ID_MAX_LEN+1];
    int num;
} TokenRow;

// scanner holding source and outputs
typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    LexemeRow ltab[LEXEME_CAP]; int lcount;
    TokenRow tlist[TOKEN_CAP]; int tcount;
} Scanner;

// just general helper functions
// required missing filename error handling
static void usage_fail(void) { 
    printf("File not found\n"); 
    exit(1); 
}
//required fil cant be read error
static void open_fail(void) {
    printf("File not read\n");  
    exit(1); 
}
// returns 1 if we've gone through all the characters in the source
static int at_end(const Scanner *S) {
    return S->pos >= S->len; 
}
//returns the character at current position + k, or 0 if past end
static int peekc(const Scanner *S, size_t k) {
    size_t j = S->pos + k; // index to be used to look ahead in the array
    if(j < S->len) {
        return S->src[j];
    } else {
        return 0;
    }
}
// skips ahead from the current positon by k spaces
static void step(Scanner *S, size_t k) { 
    S->pos += k; 
}

// init single character token table in order to map ASCII chars to tokens
static void init_single_char_tokens(void) {
    for(int i = 0; i < 256; i++) {
        singleCharTokens[i] = 0;
    }
    singleCharTokens[(unsigned char)'+'] = plussym;
    singleCharTokens[(unsigned char)'-'] = minussym;
    singleCharTokens[(unsigned char)'*'] = multsym;
    singleCharTokens[(unsigned char)'/'] = slashsym;
    singleCharTokens[(unsigned char)'='] = eqsym;
    singleCharTokens[(unsigned char)'('] = lparentsym;
    singleCharTokens[(unsigned char)')'] = rparentsym;
    singleCharTokens[(unsigned char)','] = commasym;
    singleCharTokens[(unsigned char)';'] = semicolonsym;
    singleCharTokens[(unsigned char)'.'] = periodsym;
}

// returns the token for a reserved word string s, or 0 if not reserved
static TokenType reserved_lookup(const char *s) {
    for (int i = 0; i < numResWords; i++) { // goes 1 by 1 for each word
        if (strcmp(s, resWords[i]) == 0) // if  it matches a word, then:
            return resWordTokens[i]; // return the token
    }
    return (TokenType)0; // not a reserved word
}

// then we have emit helpers
// append a row to the lexeme table
static void emit_lexeme(Scanner *S, const char *lx, TokenType tk) {
    if (S->lcount >= LEXEME_CAP) 
        return;
    strncpy(S->ltab[S->lcount].lexeme, lx, sizeof(S->ltab[0].lexeme)-1); // copies the lexeme
    S->ltab[S->lcount].lexeme[sizeof(S->ltab[0].lexeme)-1] = '\0'; // mark the end of the lexem with nul char
    S->ltab[S->lcount].token = tk; // store the token
    S->ltab[S->lcount].is_error = 0; // normal row, no error
    S->ltab[S->lcount].errortext[0] = '\0';
    S->lcount++; // increment the count
}

// appends a token without an attribute to the token list. should be self-evident
static void emit_token_simple(Scanner *S, TokenType tk) {
    if (S->tcount >= TOKEN_CAP) 
        return;
    S->tlist[S->tcount].token = tk;
    S->tlist[S->tcount].has_attr = 0;
    S->tcount++;
}

// appends an identifier token with an atribute
static void emit_token_ident(Scanner *S, const char *name) {
    if (S->tcount >= TOKEN_CAP) 
        return;
    S->tlist[S->tcount].token = identsym;
    S->tlist[S->tcount].has_attr = 1; // attribute check
    strncpy(S->tlist[S->tcount].id, name, ID_MAX_LEN); //r est is the same
    S->tlist[S->tcount].id[ID_MAX_LEN] = '\0';
    S->tcount++;
}

// appends a number token with its integer value attribute. similar to previous function.
static void emit_token_number(Scanner *S, int val) {
    if (S->tcount >= TOKEN_CAP) return;
    S->tlist[S->tcount].token = numbersym;
    S->tlist[S->tcount].has_attr = 1;
    S->tlist[S->tcount].num = val;
    S->tcount++;
}

//was getting an error when outputting the error lexemes, so I made a separate function:
static void emit_lexeme_error(Scanner *S, const char *lx, const char *message){
    if (S->lcount >= LEXEME_CAP)
        return;
    strncpy(S->ltab[S->lcount].lexeme, lx, sizeof(S->ltab[0].lexeme)-1);
    S->ltab[S->lcount].lexeme[sizeof(S->ltab[0].lexeme)-1] = '\0';
    S->ltab[S->lcount].token = skipsym; // it's not really necessary since it isn't printed, but for the sake of continuity
    S->ltab[S->lcount].is_error = 1; // mark the row as an error
    strncpy(S->ltab[S->lcount].errortext, message, sizeof(S->ltab[0].errortext)-1);
    S->ltab[S->lcount].errortext[sizeof(S->ltab[0].errortext)-1] = '\0';
    S->lcount++;
}

// "Scanning Routines" from the lecture
// skip any number of whitespace and /* something */ (block comments)
static void skip_whitespace_and_comments(Scanner *S) {
    while(1) {
        /* whitespace */
        while (!at_end(S)) {
            int c = peekc(S,0);
            if (c==' ' || c=='\t' || c=='\n' || c=='\r') // if the current character is a space, tab, newline, or return character
                step(S,1); // skip the character
            else 
                break; // stop if it isn't a whitespace character
        }
        
        // check if first two characters are /*
        if (!at_end(S) && peekc(S,0)=='/' && peekc(S,1)=='*') { 
            step(S,2); // skip those characters
            int closed = 0;
            while (!at_end(S)) { // scan and consume the */ characters
                if (peekc(S,0)=='*' && peekc(S,1)=='/') { 
                    step(S,2); 
                    closed=1; 
                    break; 
                }
                step(S,1);
            }
            if (!closed) { // if we reached EOF before seeing the */, error
                // i guess the assignment says we don't need to do this one. only number size,
                // identifier size, and invalid symbols. not unfinished comments.
                return;
            }
            continue; // at the end of a comment, loop to skip more whitespace or comments
        }
        break;
    }
}

// scans in identifiers or reserved words that start with a letter
static void scan_identifier_or_reserved(Scanner *S) {
    char buf[ID_MAX_LEN+1]; // buffer for spelling
    char full[32]; // error handling
    int k = 0; // keeps track of the number of characters stored
    int f = 0; // error handling, same as number too long
    size_t start = S->pos;  // to compute length

    while (!at_end(S)) {
        int c = peekc(S,0);
        if (isalpha(c) || isdigit(c)) { // as long as there's a letter or digit, then the lexeme continues
            if (k < ID_MAX_LEN) // check the length cap and store it if it's under the cap
                buf[k++] = (char)c; 
            if(f < (int)sizeof(full)-1)
                full[f++] = (char)c;
            step(S,1);
        } else 
            break; // stoped when reaching a character that isnt a char or digit
    }
    buf[k] = '\0'; // nul to the end of of the buffer.
    full[f] = '\0';

    if ((int)(S->pos - start) > ID_MAX_LEN) { // if the length is bigger than the cap, error
        emit_lexeme_error(S, full, "Identifier too long");
        emit_token_simple(S, skipsym);
        return;
    }

    // consult the reserved words table. if reserved, add it to the lexeme table (with no attr) and token list.
    // if not reserved then it's an identifier so append a lexeme table row and add to table list with a string attr
    TokenType kw = reserved_lookup(buf); 
    if (kw) {
        emit_lexeme(S, buf, kw);
        emit_token_simple(S, kw);
    } else {
        emit_lexeme(S, buf, identsym);
        emit_token_ident(S, buf);
    }
}

// scans a number with one or more digits
static void scan_number(Scanner *S) {
    char buf[NUM_MAX_LEN+1]; // same setup as previous
    char full[32];
    int k = 0;
    int f = 0; // track fullness in full array
    size_t start = S->pos;

    while (!at_end(S) && isdigit(peekc(S,0))) {
        int ch = peekc(S,0);
        if (k < NUM_MAX_LEN) 
            buf[k++] = (char)ch;
        if (f < (int)sizeof(full)-1)
            full[f++] = (char)ch;
        step(S,1);
    }
    buf[k] = '\0';
    full[f] = '\0';

    if ((int)(S->pos - start) > NUM_MAX_LEN) { // checks if too many digits, error
        emit_lexeme_error(S, full, "Number too long"); // got this from the assignment pdf
        emit_token_simple(S, skipsym);
        return;
    }

    //add to lexeme table, convert it with atoi and add it to the token list
    emit_lexeme(S, buf, numbersym); 
    emit_token_number(S, atoi(buf));
}

// scans symbols and reports invalid single characters
static void scan_symbol_or_error(Scanner *S) {
    int c = peekc(S,0);

    // handle multiple character operators first like :=, >=, <=, etc.
    if (c == ':') {
    if (peekc(S,1) == '=') {
        emit_lexeme(S, ":=", becomessym);
        emit_token_simple(S, becomessym);
        step(S,2);
    } else { // invalid symbol, mark it
        char lx[2] = {':', 0};
        emit_lexeme_error(S, lx, "Invalid symbol");
        emit_token_simple(S, skipsym);
        step(S,1);
    }
    return;
}
    if (c == '<') {
        if (peekc(S,1) == '=') { 
            emit_lexeme(S,"<=",leqsym); 
            emit_token_simple(S,leqsym); 
            step(S,2); 
            return; 
        }
        if (peekc(S,1) == '>') { 
            emit_lexeme(S,"<>",neqsym); 
            emit_token_simple(S,neqsym); 
            step(S,2); 
            return; 
        }
        emit_lexeme(S,"<",lessym); 
        emit_token_simple(S,lessym); 
        step(S,1); 
        return;
    }
    if (c == '>') {
        if (peekc(S,1) == '=') { 
            emit_lexeme(S,">=",geqsym); 
            emit_token_simple(S,geqsym); 
            step(S,2); 
            return; 
        }
        emit_lexeme(S,">",gtrsym); 
        emit_token_simple(S,gtrsym); 
        step(S,1); 
        return;
    }

    // single character tokens (+ - * / = ( ) , ; .)
    TokenType t = singleCharTokens[(unsigned char)c];
    if (t != 0) {
        char lx[2] = {(char)c, 0}; // (c is int convert it to char)
        emit_lexeme(S, lx, t);
        emit_token_simple(S, t);
        step(S,1); // consume character
        return;
    }

    // if any other symbol, invalid. mark error
    if (!at_end(S)) {
        char lx[2] = {(char)peekc(S,0), 0};
        emit_lexeme_error(S, lx, "Invalid symbol");
        emit_token_simple(S, skipsym);
        step(S,1);
    }
}


int main(int argc, char **argv) {
    if (argc != 2) 
        usage_fail();

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) 
        open_fail();

    static char buffer[SRC_LIMIT_BYTES + 1]; // source storage plus an odditonal space for nul
    size_t n = fread(buffer, 1, SRC_LIMIT_BYTES, fp); // read up until the cap
    fclose(fp);
    buffer[n] = '\0'; //mark the end with nul

    printf("Source Program:\n\n%s\n", buffer);

    init_single_char_tokens();

    // init scanner and set pointers and counters
    Scanner S;
    S.src = buffer; 
    S.len = n; 
    S.pos = 0;
    S.lcount = 0; 
    S.tcount = 0;

    // main loop, walk through the source until the end from left to right
    // you can read the function names and understand what's happening, scans
    // for certain characters, and does soemthing different depending on the result.
    while (!at_end(&S)) {
        skip_whitespace_and_comments(&S);
        if (at_end(&S)) 
            break;

        int c = peekc(&S,0);
        if (isalpha(c)) { 
            scan_identifier_or_reserved(&S); 
            continue; 
        }
        if (isdigit(c)) { 
            scan_number(&S); 
            continue; 
        }
        scan_symbol_or_error(&S);
    }

    // lexeme table print, I know it's not necessary but I tried to make the
    // formatting as close to the example output as possible.
    printf("\nLexeme Table:\n");
    printf("\nlexeme  token type\n");
    for (int i = 0; i < S.lcount; i++) {
        if(S.ltab[i].is_error) // if true, print message instead of token code
            printf("%-7s %s\n", S.ltab[i].lexeme, S.ltab[i].errortext);
        else
            printf("%-7s %d\n", S.ltab[i].lexeme, S.ltab[i].token);
    }

    // lastly, the token list
    printf("\nToken List:\n\n");
    for (int i = 0; i < S.tcount; i++) {
        TokenRow *t = &S.tlist[i];
        if (t->token == identsym) { // for identifiers, print code + name
            printf("%d %s ", identsym, t->id);
        } else if (t->token == numbersym) { // for nunbers, print code + value
            printf("%d %d ", numbersym, t->num);
        } else { // any other token, print only code
            printf("%d ", t->token);
        }
    }
    printf("\n");

    return 0;
}