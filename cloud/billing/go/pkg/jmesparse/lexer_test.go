package jmesparse

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

var lexingTests = []struct {
	expression string
	expected   []Token
}{
	{"*", []Token{{TStar, "*", 0, 1}}},
	{".", []Token{{TDot, ".", 0, 1}}},
	{"[?", []Token{{TFilter, "[?", 0, 2}}},
	{"[]", []Token{{TFlatten, "[]", 0, 2}}},
	{"(", []Token{{TLparen, "(", 0, 1}}},
	{")", []Token{{TRparen, ")", 0, 1}}},
	{"[", []Token{{TLbracket, "[", 0, 1}}},
	{"]", []Token{{TRbracket, "]", 0, 1}}},
	{"{", []Token{{TLbrace, "{", 0, 1}}},
	{"}", []Token{{TRbrace, "}", 0, 1}}},
	{"||", []Token{{TOr, "||", 0, 2}}},
	{"|", []Token{{TPipe, "|", 0, 1}}},
	{"29", []Token{{TNumber, "29", 0, 2}}},
	{"2", []Token{{TNumber, "2", 0, 1}}},
	{"0", []Token{{TNumber, "0", 0, 1}}},
	{"-20", []Token{{TNumber, "-20", 0, 3}}},
	{"foo", []Token{{TUnquotedIdentifier, "foo", 0, 3}}},
	{`"bar"`, []Token{{TQuotedIdentifier, "bar", 0, 3}}},
	// Escaping the delimiter
	{`"bar\"baz"`, []Token{{TQuotedIdentifier, `bar"baz`, 0, 7}}},
	{",", []Token{{TComma, ",", 0, 1}}},
	{":", []Token{{TColon, ":", 0, 1}}},
	{"<", []Token{{TLT, "<", 0, 1}}},
	{"<=", []Token{{TLTE, "<=", 0, 2}}},
	{">", []Token{{TGT, ">", 0, 1}}},
	{">=", []Token{{TGTE, ">=", 0, 2}}},
	{"==", []Token{{TEQ, "==", 0, 2}}},
	{"!=", []Token{{TNE, "!=", 0, 2}}},
	{"`[0, 1, 2]`", []Token{{TJSONLiteral, "[0, 1, 2]", 1, 9}}},
	{"'foo'", []Token{{TStringLiteral, "foo", 1, 3}}},
	{"'a'", []Token{{TStringLiteral, "a", 1, 1}}},
	{`'foo\'bar'`, []Token{{TStringLiteral, "foo'bar", 1, 7}}},
	{"@", []Token{{TCurrent, "@", 0, 1}}},
	{"&", []Token{{TExpref, "&", 0, 1}}},
	// Quoted identifier unicode escape sequences
	{`"\u2713"`, []Token{{TQuotedIdentifier, "✓", 0, 3}}},
	{`"\\"`, []Token{{TQuotedIdentifier, `\`, 0, 1}}},
	{"`\"foo\"`", []Token{{TJSONLiteral, "\"foo\"", 1, 5}}},
	{"`foo`", []Token{{TJSONLiteral, "foo", 1, 3}}}, // Python like strings used in skus
	// Combinations of tokens.
	{"foo.bar", []Token{
		{TUnquotedIdentifier, "foo", 0, 3},
		{TDot, ".", 3, 1},
		{TUnquotedIdentifier, "bar", 4, 3},
	}},
	{"foo[0]", []Token{
		{TUnquotedIdentifier, "foo", 0, 3},
		{TLbracket, "[", 3, 1},
		{TNumber, "0", 4, 1},
		{TRbracket, "]", 5, 1},
	}},
	{"foo[?a<b]", []Token{
		{TUnquotedIdentifier, "foo", 0, 3},
		{TFilter, "[?", 3, 2},
		{TUnquotedIdentifier, "a", 5, 1},
		{TLT, "<", 6, 1},
		{TUnquotedIdentifier, "b", 7, 1},
		{TRbracket, "]", 8, 1},
	}},
}

func TestCanLexTokens(t *testing.T) {
	assert := assert.New(t)
	lexer := NewLexer()
	for _, tt := range lexingTests {
		tokens, err := lexer.tokenize(tt.expression)
		if assert.NoError(err) {
			errMsg := fmt.Sprintf("Mismatch expected number of tokens: (expected: %s, actual: %s)",
				tt.expected, tokens)
			tt.expected = append(tt.expected, Token{TEOF, "", len(tt.expression), 0})
			if assert.Equal(len(tt.expected), len(tokens), errMsg) {
				for i, token := range tokens {
					expected := tt.expected[i]
					assert.Equal(expected, token, "Token not equal")
				}
			}
		}
	}
}

var lexingErrorTests = []struct {
	expression string
	msg        string
}{
	{"'foo", "Missing closing single quote"},
	{"[?foo==bar?]", "Unknown char '?'"},
}

func TestLexingErrors(t *testing.T) {
	assert := assert.New(t)
	lexer := NewLexer()
	for _, tt := range lexingErrorTests {
		_, err := lexer.tokenize(tt.expression)
		assert.Error(err, fmt.Sprintf("Expected lexing error: %s", tt.msg))
	}
}

var (
	exprIdentifier          = "abcdefghijklmnopqrstuvwxyz"
	exprSubexpr             = "abcdefghijklmnopqrstuvwxyz.abcdefghijklmnopqrstuvwxyz"
	deeplyNested50          = "j49.j48.j47.j46.j45.j44.j43.j42.j41.j40.j39.j38.j37.j36.j35.j34.j33.j32.j31.j30.j29.j28.j27.j26.j25.j24.j23.j22.j21.j20.j19.j18.j17.j16.j15.j14.j13.j12.j11.j10.j9.j8.j7.j6.j5.j4.j3.j2.j1.j0"
	deeplyNested50Pipe      = "j49|j48|j47|j46|j45|j44|j43|j42|j41|j40|j39|j38|j37|j36|j35|j34|j33|j32|j31|j30|j29|j28|j27|j26|j25|j24|j23|j22|j21|j20|j19|j18|j17|j16|j15|j14|j13|j12|j11|j10|j9|j8|j7|j6|j5|j4|j3|j2|j1|j0"
	deeplyNested50Index     = "[49][48][47][46][45][44][43][42][41][40][39][38][37][36][35][34][33][32][31][30][29][28][27][26][25][24][23][22][21][20][19][18][17][16][15][14][13][12][11][10][9][8][7][6][5][4][3][2][1][0]"
	deepProjection104       = "a[*].b[*].c[*].d[*].e[*].f[*].g[*].h[*].i[*].j[*].k[*].l[*].m[*].n[*].o[*].p[*].q[*].r[*].s[*].t[*].u[*].v[*].w[*].x[*].y[*].z[*].a[*].b[*].c[*].d[*].e[*].f[*].g[*].h[*].i[*].j[*].k[*].l[*].m[*].n[*].o[*].p[*].q[*].r[*].s[*].t[*].u[*].v[*].w[*].x[*].y[*].z[*].a[*].b[*].c[*].d[*].e[*].f[*].g[*].h[*].i[*].j[*].k[*].l[*].m[*].n[*].o[*].p[*].q[*].r[*].s[*].t[*].u[*].v[*].w[*].x[*].y[*].z[*].a[*].b[*].c[*].d[*].e[*].f[*].g[*].h[*].i[*].j[*].k[*].l[*].m[*].n[*].o[*].p[*].q[*].r[*].s[*].t[*].u[*].v[*].w[*].x[*].y[*].z[*]"
	exprQuotedIdentifier    = `"abcdefghijklmnopqrstuvwxyz.abcdefghijklmnopqrstuvwxyz"`
	quotedIdentifierEscapes = `"\n\r\b\t\n\r\b\t\n\r\b\t\n\r\b\t\n\r\b\t\n\r\b\t\n\r\b\t"`
	rawStringLiteral        = `'abcdefghijklmnopqrstuvwxyz.abcdefghijklmnopqrstuvwxyz'`
)

func BenchmarkLexIdentifier(b *testing.B) {
	runLexBenchmark(b, exprIdentifier)
}

func BenchmarkLexSubexpression(b *testing.B) {
	runLexBenchmark(b, exprSubexpr)
}

func BenchmarkLexDeeplyNested50(b *testing.B) {
	runLexBenchmark(b, deeplyNested50)
}

func BenchmarkLexDeepNested50Pipe(b *testing.B) {
	runLexBenchmark(b, deeplyNested50Pipe)
}

func BenchmarkLexDeepNested50Index(b *testing.B) {
	runLexBenchmark(b, deeplyNested50Index)
}

func BenchmarkLexQuotedIdentifier(b *testing.B) {
	runLexBenchmark(b, exprQuotedIdentifier)
}

func BenchmarkLexQuotedIdentifierEscapes(b *testing.B) {
	runLexBenchmark(b, quotedIdentifierEscapes)
}

func BenchmarkLexRawStringLiteral(b *testing.B) {
	runLexBenchmark(b, rawStringLiteral)
}

func BenchmarkLexDeepProjection104(b *testing.B) {
	runLexBenchmark(b, deepProjection104)
}

func runLexBenchmark(b *testing.B, expression string) {
	lexer := NewLexer()
	for i := 0; i < b.N; i++ {
		_, _ = lexer.tokenize(expression)
	}
}
