#include <stdio.h>
#include <stdlib.h>

/* tracking line numbers for error reporting and newline replacements */
static int line = 1;
static int commentline = 0;

/* DFA states */
typedef enum {
STATE_CODE,
STATE_SLASH,
STATE_IN_COMMENT,
STATE_IN_COMMENT_STAR,
STATE_IN_STRING,
STATE_IN_STRING_ESC,
STATE_IN_CHAR,
STATE_IN_CHAR_ESC
} State;

/*writes one character output if not EOF */
static void print_char(int c) {
    if (c != EOF){
    putchar(c);
    }
}

/*  state handler functions - each takes the current character
    and returns the next state  */
/* normal code: print chars as they come, switch states if needed */
static State handle_code(int c) {
if (c == '/') return STATE_SLASH;
if (c == '"') { print_char(c); return STATE_IN_STRING; }
if (c == '\'') { print_char(c); return STATE_IN_CHAR; }
if (c == '\n') { print_char(c); line++; return STATE_CODE; }
if (c != EOF) print_char(c);
return STATE_CODE;
}

/* if see a slash - decide if it's a comment or just a slash */
static State handle_slash(int c) {
    if (c == '*') {
        print_char(' ');       /* replace comment with space */
        commentLine = line;    /* remember where comment started */
        return STATE_IN_COMMENT;
    }
    /* Not start of comment: output slash and handle c normally */
    print_char('/');
    if (c != EOF) {
        ungetc(c, stdin);  /* push back the lookahead character */
    }
    return STATE_CODE;
}

/* inside the comment, skip chars, but keep newlines */
static State handle_in_comment(int c) {
if (c == '*') return STATE_IN_COMMENT_STAR;
if (c == '\n') { print_char('\n'); line++; return STATE_IN_COMMENT; }
if (c == EOF) {
fprintf(stderr, "Error: line %d: unterminated comment\n", commentline);
exit(EXIT_FAILURE);
}
return STATE_IN_COMMENT;
}

/* if theres a '*' in a comment - then maybe its closing, maybe not */
static State handle_in_comment_star(int c) {
if (c == '/') return STATE_CODE;
if (c == '*') return STATE_IN_COMMENT_STAR;

if (c == '\n') { print_char('\n'); line++; return STATE_IN_COMMENT; }
if (c == EOF) {
fprintf(stderr, "Error: line %d: unterminated comment\n", commentline);
exit(EXIT_FAILURE);
}
return STATE_IN_COMMENT;
}

/* inside string literal - print until closing quote */
static State handle_in_string(int c) {
if (c == '\\') { print_char(c); return STATE_IN_STRING_ESC; }
if (c == '"') { print_char(c); return STATE_CODE; }
if (c == '\n') { print_char(c); line++; return STATE_IN_STRING; }
if (c != EOF) print_char(c);
return STATE_IN_STRING;
}

/* after backslash in string - just print and go back */
static State handle_in_string_esc(int c) {
if (c != EOF) print_char(c);
return STATE_IN_STRING;
}

/* inside a char literal - print until closing quote */
static State handle_in_char(int c) {
if (c == '\\') { print_char(c); return STATE_IN_CHAR_ESC; }
if (c == '\'') { print_char(c); return STATE_CODE; }
if (c == '\n') { print_char(c); line++; return STATE_IN_CHAR; }
if (c != EOF) print_char(c);
return STATE_IN_CHAR;
}

/* after a backslash in char literal */
static State handle_in_char_esc(int c) {
if (c != EOF) print_char(c);
return STATE_IN_CHAR;
}

/*  main function: to read input and drive the DFA  */
int main(void) {
State state = STATE_CODE;
int c;  
while ((c = getchar()) != EOF) {
    switch (state) {
        case STATE_CODE:          state = handle_code(c); break;
        case STATE_SLASH:         state = handle_slash(c); break;
        case STATE_IN_COMMENT:    state = handle_in_comment(c); break;
        case STATE_IN_COMMENT_STAR: state = handle_in_comment_star(c); break;
        case STATE_IN_STRING:     state = handle_in_string(c); break;
        case STATE_IN_STRING_ESC: state = handle_in_string_esc(c); break;
        case STATE_IN_CHAR:       state = handle_in_char(c); break;
        case STATE_IN_CHAR_ESC:   state = handle_in_char_esc(c); break;
    }
}

/* sending an error if the EOC is reached withint he comment */
if (state == STATE_IN_COMMENT || state == STATE_IN_COMMENT_STAR) {
    fprintf(stderr, "Error: line %d: unterminated comment\n", commentline);
    return EXIT_FAILURE;
}

return EXIT_SUCCESS; 

}




