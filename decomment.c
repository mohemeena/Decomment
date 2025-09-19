#include <stdio.h>
#include <stdlib.h>

/* tracking line numbers for error reporting and newline replacements */
static int line = 1;
/* line where the current comment started (for error messages) */
static int commentLine = 0;

/* DFA states */
typedef enum {
    CODE,
    SLASH,
    INSIDE_COMMENT,
    STAR_INSIDE_COMMENT,
    INSIDE_STRING,
    ESCAPE_INSIDE_STRING,
    INSIDE_CHAR,
    ESCAPE_INSIDE_CHAR
} State;

/*
 * print_char
 * parameters: c - the character to write (int, may be EOF)
 * behavior: writes c to stdout using putchar if c != EOF; does no
 *           transformation.
 * global:   none modified. Writes to stdout.
 * returns:   void
 */
static void print_char(int c) {
    if (c != EOF) putchar(c);
}

/*
 * handle_code
 * parameters: c - most recent input character (int)
 * behavior:  handles characters when DFA is in CODE state. could get rid of the
 *            the character, update the line counter on newline,
 *            or transition to a different state.
 * globals:   reads/writes `line`; writes to stdout viwitha print_char.
 * returns:   the next state for the DFA.
 */
static State handle_code(int c) {
    if (c == '/') return SLASH;
    if (c == '"') { print_char(c); return INSIDE_STRING; }
    if (c == '\'') { print_char(c); return INSIDE_CHAR; }
    if (c == '\n') { print_char(c); line++; return CODE; }
    if (c != EOF) print_char(c);
    return CODE;
}

/*
 * handle_slash
 * parameters: c - the lookahead character after a '/'
 * behavior:  if they see a slash followed by star, treat as comment start: emit
 *            a single space, record the comment start line (commetline),
 *            and go to to INSIDE_COMMENT. otherwise emit the slash
 *            and push back the lookahead character so the caller will
 *            handle it normally.
 * global:   writes to stdout via print_char; updates `commentline`.
 * return:   INSIDE_COMMENT when a comment starts, otherwise CODE.
 */
static State handle_slash(int c) {
    if (c == '*') {
        print_char(' ');
        commentLine = line;
        return INSIDE_COMMENT;
    }
    /* always print the slash if it's not a comment start */
    print_char('/');
    if (c != EOF) ungetc(c, stdin);
    return CODE;
}

/*
 * handle_in_comment
 * parameters: c - the current character while inside a comment
 * behavior:  consume and ignore comment characters. keep newline
 *            characters, and increment `line`. if star is
 *            seen, move state to STAR_INSIDE_COMMENT to check for comment end.
 *            on EOF before closing, print error to stderr and exit.
 * globals:   reads `commentLine` for error messages; updates `line`;
 *            writes to stdout for newlines; may call exit().
 * returns:   STAR_INSIDE_COMMENT or INSIDE_COMMENT, does not return on EOF error.
 */
static State handle_inside_comment(int c) {
    if (c == '*') return STAR_INSIDE_COMMENT;
    if (c == '\n') { print_char('\n'); line++; return INSIDE_COMMENT; }
    if (c == EOF) {
        fprintf(stderr, "Error: line %d: unterminated comment\n", commentLine);
        exit(EXIT_FAILURE);
    }
    return INSIDE_COMMENT;
}

/*
 * handle_in_comment_star
 * parameters: c - the character following a '*' inside a comment
 * behavior:  if the next char is '/', the comment closes and the DFA
 *            returns to CODE. if it's another '*', remain in this star
 *            state. preserve newlines. on EOF, report error and exit.
 * globals:   may write to stdout and stderr; reads `commentLine` and
 *            updates `line`.
 * returns:   CODE, STAR_INSIDE_COMMENT, or INSIDE_COMMENT.
 */
static State handle_inside_comment_star(int c) {
    if (c == '/') return CODE;
    if (c == '*') return STAR_INSIDE_COMMENT;
    if (c == '\n') { print_char('\n'); line++; return INSIDE_COMMENT; }
    if (c == EOF) {
        fprintf(stderr, "Error: line %d: unterminated comment\n", commentLine);
        exit(EXIT_FAILURE);
    }
    return INSIDE_COMMENT;
}

/*
 * handle_in_string
 * parameters: c - the current character while inside a string literal
 * behavior:  print characters in the string literal to stdout. if a backslash
 *            is seen, switch to ESCAPE_INSIDE_STRING to handle escapes.
 *            on the closing quote, return to CODE. keep newlines and update
 *            the `line` counter.
 * globals:   writes to stdout; updates `line` on newline.
 * returns:   INSIDE_STRING, ESCAPE_INSIDE_STRING, or CODE.
 */
static State handle_inside_string(int c) {
    if (c == '\\') { print_char(c); return ESCAPE_INSIDE_STRING; }
    if (c == '"') { print_char(c); return CODE; }
    if (c == '\n') { print_char('\n'); line++; return INSIDE_STRING; }
    if (c != EOF) print_char(c);
    return INSIDE_STRING;
}

/*
 * handle_in_string_esc
 * Parameters: c - escaped character following a backslash inside a string
 * Behavior:  Emit the escaped character (so sequences like \n remain
 *            intact in output) and return to INSIDE_STRING.
 * Globals:   Writes to stdout.
 * Returns:   INSIDE_STRING.
 */
static State handle_inside_string_esc(int c) {
    if (c != EOF) print_char(c);
    return INSIDE_STRING;
}

/*
 * handle_in_char
 * parameters: c - current character inside a character literal
 * behavior:  show the character literal contents. if a backslash appears,
 *            switch to ESCAPE_INSIDE_CHAR. on closing single-quote,
 *            return to CODE. keep newlines.
 * globals:   writes to stdout and updates `line` on newline.
 * returns:   INSIDE_CHAR, ESCAPE_INSIDE_CHAR, or CODE.
 */
static State handle_inside_char(int c) {
    if (c == '\\') { print_char(c); return ESCAPE_INSIDE_CHAR; }
    if (c == '\'') { print_char(c); return CODE; }
    if (c == '\n') { print_char('\n'); line++; return INSIDE_CHAR; }
    if (c != EOF) print_char(c);
    return INSIDE_CHAR;
}

/*
 * handle_in_char_esc
 * parameters: c - escaped character inside a char literal
 * behavior:  show the escaped character and go abck to INSIDE_CHAR.
 * globals:   writes to stdout.
 * returns:   INSIDE_CHAR.
 */
static State handle_inside_char_esc(int c) {
    if (c != EOF) print_char(c);
    return INSIDE_CHAR;
}


/* main method */
int main(void) {
    State state = CODE;
    int c;
    while ((c = getchar()) != EOF) {
        switch (state) {
            case CODE:                 state = handle_code(c); break;
            case SLASH:                state = handle_slash(c); break;
            case INSIDE_COMMENT:       state = handle_inside_comment(c); break;
            case STAR_INSIDE_COMMENT:  state = handle_inside_comment_star(c); break;
            case INSIDE_STRING:        state = handle_inside_string(c); break;
            case ESCAPE_INSIDE_STRING: state = handle_inside_string_esc(c); break;
            case INSIDE_CHAR:          state = handle_inside_char(c); break;
            case ESCAPE_INSIDE_CHAR:   state = handle_inside_char_esc(c); break;
        }
    }

    if (state == INSIDE_COMMENT || state == STAR_INSIDE_COMMENT) {
        fprintf(stderr, "Error: line %d: unterminated comment\n", commentLine);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


