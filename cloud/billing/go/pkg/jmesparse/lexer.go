package jmesparse

import (
	"bytes"
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"unicode/utf8"
)

type Token struct {
	TokenType TokType
	Value     string
	Position  int
	Length    int
}

type TokType int

const eof = -1

// Lexer contains information about the expression being tokenized.
type Lexer struct {
	expression string       // The expression provided by the user.
	currentPos int          // The current position in the string.
	lastWidth  int          // The width of the current rune.  This
	buf        bytes.Buffer // Internal buffer used for building up values.
}

// SyntaxError is the main error used whenever a lexing or parsing error occurs.
type SyntaxError struct {
	Msg        string // Error message displayed to user
	Expression string // Expression that generated a SyntaxError
	Offset     int    // The location in the string where the error occurred
}

func (e SyntaxError) Error() string {
	// In the future, it would be good to underline the specific
	// location where the error occurred.
	return "SyntaxError: " + e.Msg
}

// HighlightLocation will show where the syntax error occurred.
// It will place a "^" character on a line below the expression
// at the point where the syntax error occurred.
func (e SyntaxError) HighlightLocation() string {
	return e.Expression + "\n" + strings.Repeat(" ", e.Offset) + "^"
}

//go:generate stringer -type=TokType
const (
	TUnknown TokType = iota
	TStar
	TDot
	TFilter
	TFlatten
	TLparen
	TRparen
	TLbracket
	TRbracket
	TLbrace
	TRbrace
	TOr
	TPipe
	TNumber
	TUnquotedIdentifier
	TQuotedIdentifier
	TComma
	TColon
	TLT
	TLTE
	TGT
	TGTE
	TEQ
	TNE
	TJSONLiteral
	TStringLiteral
	TCurrent
	TExpref
	TAnd
	TNot
	TEOF
)

var basicTokens = map[rune]TokType{
	'.': TDot,
	'*': TStar,
	',': TComma,
	':': TColon,
	'{': TLbrace,
	'}': TRbrace,
	']': TRbracket, // tLbracket not included because it could be "[]"
	'(': TLparen,
	')': TRparen,
	'@': TCurrent,
}

// Bit mask for [a-zA-Z_] shifted down 64 bits to fit in a single uint64.
// When using this bitmask just be sure to shift the rune down 64 bits
// before checking against identifierStartBits.
const identifierStartBits uint64 = 576460745995190270

// Bit mask for [a-zA-Z0-9], 128 bits -> 2 uint64s.
var identifierTrailingBits = [2]uint64{287948901175001088, 576460745995190270}

var whiteSpace = map[rune]bool{
	' ': true, '\t': true, '\n': true, '\r': true,
}

func (t Token) String() string {
	return fmt.Sprintf("Token{%+v, %s, %d, %d}",
		t.TokenType, t.Value, t.Position, t.Length)
}

// NewLexer creates a new JMESPath lexer.
func NewLexer() *Lexer {
	lexer := Lexer{}
	return &lexer
}

func (lexer *Lexer) next() rune {
	if lexer.currentPos >= len(lexer.expression) {
		lexer.lastWidth = 0
		return eof
	}
	r, w := utf8.DecodeRuneInString(lexer.expression[lexer.currentPos:])
	lexer.lastWidth = w
	lexer.currentPos += w
	return r
}

func (lexer *Lexer) back() {
	lexer.currentPos -= lexer.lastWidth
}

func (lexer *Lexer) peek() rune {
	t := lexer.next()
	lexer.back()
	return t
}

// tokenize takes an expression and returns corresponding tokens.
func (lexer *Lexer) tokenize(expression string) ([]Token, error) {
	var tokens []Token
	lexer.expression = expression
	lexer.currentPos = 0
	lexer.lastWidth = 0
loop:
	for {
		r := lexer.next()
		if identifierStartBits&(1<<(uint64(r)-64)) > 0 {
			t := lexer.consumeUnquotedIdentifier()
			tokens = append(tokens, t)
		} else if val, ok := basicTokens[r]; ok {
			// Basic single char token.
			t := Token{
				TokenType: val,
				Value:     string(r),
				Position:  lexer.currentPos - lexer.lastWidth,
				Length:    1,
			}
			tokens = append(tokens, t)
		} else if r == '-' || (r >= '0' && r <= '9') {
			t := lexer.consumeNumber()
			tokens = append(tokens, t)
		} else if r == '[' {
			t := lexer.consumeLBracket()
			tokens = append(tokens, t)
		} else if r == '"' {
			t, err := lexer.consumeQuotedIdentifier()
			if err != nil {
				return tokens, err
			}
			tokens = append(tokens, t)
		} else if r == '\'' {
			t, err := lexer.consumeRawStringLiteral()
			if err != nil {
				return tokens, err
			}
			tokens = append(tokens, t)
		} else if r == '`' {
			t, err := lexer.consumeLiteral()
			if err != nil {
				return tokens, err
			}
			tokens = append(tokens, t)
		} else if r == '|' {
			t := lexer.matchOrElse(r, '|', TOr, TPipe)
			tokens = append(tokens, t)
		} else if r == '<' {
			t := lexer.matchOrElse(r, '=', TLTE, TLT)
			tokens = append(tokens, t)
		} else if r == '>' {
			t := lexer.matchOrElse(r, '=', TGTE, TGT)
			tokens = append(tokens, t)
		} else if r == '!' {
			t := lexer.matchOrElse(r, '=', TNE, TNot)
			tokens = append(tokens, t)
		} else if r == '=' {
			t := lexer.matchOrElse(r, '=', TEQ, TUnknown)
			tokens = append(tokens, t)
		} else if r == '&' {
			t := lexer.matchOrElse(r, '&', TAnd, TExpref)
			tokens = append(tokens, t)
		} else if r == eof {
			break loop
		} else if _, ok := whiteSpace[r]; ok {
			// Ignore whitespace
		} else {
			return tokens, lexer.syntaxError(fmt.Sprintf("Unknown char: %s", strconv.QuoteRuneToASCII(r)))
		}
	}
	tokens = append(tokens, Token{TEOF, "", len(lexer.expression), 0})
	return tokens, nil
}

// Consume characters until the ending rune "r" is reached.
// If the end of the expression is reached before seeing the
// terminating rune "r", then an error is returned.
// If no error occurs then the matching substring is returned.
// The returned string will not include the ending rune.
func (lexer *Lexer) consumeUntil(end rune) (string, error) {
	start := lexer.currentPos
	current := lexer.next()
	for current != end && current != eof {
		if current == '\\' && lexer.peek() != eof {
			lexer.next()
		}
		current = lexer.next()
	}
	if lexer.lastWidth == 0 {
		// Then we hit an EOF so we never reached the closing
		// delimiter.
		return "", SyntaxError{
			Msg:        "Unclosed delimiter: " + string(end),
			Expression: lexer.expression,
			Offset:     len(lexer.expression),
		}
	}
	return lexer.expression[start : lexer.currentPos-lexer.lastWidth], nil
}

func (lexer *Lexer) consumeLiteral() (Token, error) {
	start := lexer.currentPos
	value, err := lexer.consumeUntil('`')
	if err != nil {
		return Token{}, err
	}
	value = strings.Replace(value, "\\`", "`", -1)
	return Token{
		TokenType: TJSONLiteral,
		Value:     value,
		Position:  start,
		Length:    len(value),
	}, nil
}

func (lexer *Lexer) consumeRawStringLiteral() (Token, error) {
	start := lexer.currentPos
	currentIndex := start
	current := lexer.next()
	for current != '\'' && lexer.peek() != eof {
		if current == '\\' && lexer.peek() == '\'' {
			chunk := lexer.expression[currentIndex : lexer.currentPos-1]
			lexer.buf.WriteString(chunk)
			lexer.buf.WriteString("'")
			lexer.next()
			currentIndex = lexer.currentPos
		}
		current = lexer.next()
	}
	if lexer.lastWidth == 0 {
		// Then we hit an EOF so we never reached the closing
		// delimiter.
		return Token{}, SyntaxError{
			Msg:        "Unclosed delimiter: '",
			Expression: lexer.expression,
			Offset:     len(lexer.expression),
		}
	}
	if currentIndex < lexer.currentPos {
		lexer.buf.WriteString(lexer.expression[currentIndex : lexer.currentPos-1])
	}
	value := lexer.buf.String()
	// Reset the buffer so it can reused again.
	lexer.buf.Reset()
	return Token{
		TokenType: TStringLiteral,
		Value:     value,
		Position:  start,
		Length:    len(value),
	}, nil
}

func (lexer *Lexer) syntaxError(msg string) SyntaxError {
	return SyntaxError{
		Msg:        msg,
		Expression: lexer.expression,
		Offset:     lexer.currentPos - 1,
	}
}

// Checks for a two char token, otherwise matches a single character
// token. This is used whenever a two char token overlaps a single
// char token, e.g. "||" -> tPipe, "|" -> tOr.
func (lexer *Lexer) matchOrElse(first rune, second rune, matchedType TokType, singleCharType TokType) Token {
	start := lexer.currentPos - lexer.lastWidth
	nextRune := lexer.next()
	var t Token
	if nextRune == second {
		t = Token{
			TokenType: matchedType,
			Value:     string(first) + string(second),
			Position:  start,
			Length:    2,
		}
	} else {
		lexer.back()
		t = Token{
			TokenType: singleCharType,
			Value:     string(first),
			Position:  start,
			Length:    1,
		}
	}
	return t
}

func (lexer *Lexer) consumeLBracket() Token {
	// There's three options here:
	// 1. A filter expression "[?"
	// 2. A flatten operator "[]"
	// 3. A bare rbracket "["
	start := lexer.currentPos - lexer.lastWidth
	nextRune := lexer.next()
	var t Token
	if nextRune == '?' {
		t = Token{
			TokenType: TFilter,
			Value:     "[?",
			Position:  start,
			Length:    2,
		}
	} else if nextRune == ']' {
		t = Token{
			TokenType: TFlatten,
			Value:     "[]",
			Position:  start,
			Length:    2,
		}
	} else {
		t = Token{
			TokenType: TLbracket,
			Value:     "[",
			Position:  start,
			Length:    1,
		}
		lexer.back()
	}
	return t
}

func (lexer *Lexer) consumeQuotedIdentifier() (Token, error) {
	start := lexer.currentPos
	value, err := lexer.consumeUntil('"')
	if err != nil {
		return Token{}, err
	}
	var decoded string
	asJSON := []byte("\"" + value + "\"")
	if err := json.Unmarshal([]byte(asJSON), &decoded); err != nil {
		return Token{}, err
	}
	return Token{
		TokenType: TQuotedIdentifier,
		Value:     decoded,
		Position:  start - 1,
		Length:    len(decoded),
	}, nil
}

func (lexer *Lexer) consumeUnquotedIdentifier() Token {
	// Consume runes until we reach the end of an unquoted
	// identifier.
	start := lexer.currentPos - lexer.lastWidth
	for {
		r := lexer.next()
		if r < 0 || r > 128 || identifierTrailingBits[uint64(r)/64]&(1<<(uint64(r)%64)) == 0 {
			lexer.back()
			break
		}
	}
	value := lexer.expression[start:lexer.currentPos]
	return Token{
		TokenType: TUnquotedIdentifier,
		Value:     value,
		Position:  start,
		Length:    lexer.currentPos - start,
	}
}

func (lexer *Lexer) consumeNumber() Token {
	// Consume runes until we reach something that's not a number.
	start := lexer.currentPos - lexer.lastWidth
	for {
		r := lexer.next()
		if r < '0' || r > '9' {
			lexer.back()
			break
		}
	}
	value := lexer.expression[start:lexer.currentPos]
	return Token{
		TokenType: TNumber,
		Value:     value,
		Position:  start,
		Length:    lexer.currentPos - start,
	}
}
