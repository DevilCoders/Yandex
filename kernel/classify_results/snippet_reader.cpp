#include "snippet_reader.h"
bool IsSnipBreakSymbol(char c) {
    // every valid break symbol is one of the lower 127 symbols of the ASCII-table
    // that is, it can be matched "as is" even in UTF-8
    // and even more, it cannot be a part of multibyte symbol, so we can check only one char
    // for further reference see http://en.wikipedia.org/wiki/UTF-8
    switch (c) {
        case ' ':
        case '!':
        case '"':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case '-':
        case '.':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '@':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '_':
        case '`':
        case '{':
        case '|':
        case '}':
        case '~':
            return true;
        default:
            return false;
    }
}
