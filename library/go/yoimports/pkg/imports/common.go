package imports

import (
	"bytes"
	"go/ast"
	"go/parser"
	"go/printer"
	"go/token"
	"path"
	"strconv"
	"strings"
	"unicode"
	"unicode/utf8"

	"github.com/dave/dst"
	"github.com/dave/dst/decorator"
)

// Keep these in sync with cmd/gofmt/gofmt.go.
const (
	tabWidth    = 8
	printerMode = printer.UseSpaces | printer.TabIndent | printerNormalizeNumbers

	// printerNormalizeNumbers means to canonicalize number literal prefixes
	// and exponents while printing. See https://golang.org/doc/go1.13#gofmt.
	//
	// This value is defined in go/printer specifically for go/format and cmd/gofmt.
	printerNormalizeNumbers = 1 << 30
)

// We use custom printer instead of format.Node/format.Source
// because they broke comments order while sorting imports :(
var stdPrinter = printer.Config{
	Mode:     printerMode,
	Tabwidth: tabWidth,
}

func declImports(gen *dst.GenDecl, path string) bool {
	if gen.Tok != token.IMPORT {
		return false
	}

	for _, spec := range gen.Specs {
		impSpec := spec.(*dst.ImportSpec)
		if importPath(impSpec) == path {
			return true
		}
	}
	return false
}

func importPath(spec *dst.ImportSpec) string {
	t, err := strconv.Unquote(spec.Path.Value)
	if err == nil {
		return t
	}
	return ""
}

func importName(spec *dst.ImportSpec) string {
	if spec.Name == nil {
		return ""
	}
	return spec.Name.Name
}

func collapse(prev, next *dst.ImportSpec) bool {
	switch {
	case importPath(next) != importPath(prev):
		return false
	case importName(next) != importName(prev):
		return false
	default:
		return true
	}
}

func bodyOffset(src []byte) (int, error) {
	fset := token.NewFileSet()
	af, err := parser.ParseFile(fset, "", src, parser.ParseComments|parser.ImportsOnly)
	if err != nil {
		return 0, err
	}

	return fset.Position(af.End()).Offset, nil
}

func printAstFile(fset *token.FileSet, f *ast.File) ([]byte, error) {
	var out bytes.Buffer
	if err := stdPrinter.Fprint(&out, fset, f); err != nil {
		return nil, err
	}

	return out.Bytes(), nil
}

func printDstFile(f *dst.File) ([]byte, error) {
	var out bytes.Buffer
	fset, af, _ := decorator.RestoreFile(f)
	if err := stdPrinter.Fprint(&out, fset, af); err != nil {
		return nil, err
	}

	return out.Bytes(), nil
}

func importPathToAssumedName(importPath string) string {
	out := path.Base(importPath)
	if strings.HasPrefix(out, "v") {
		if _, err := strconv.Atoi(out[1:]); err == nil {
			dir := path.Dir(importPath)
			if dir != "." {
				out = path.Base(dir)
			}
		}
	}

	out = strings.TrimPrefix(out, "go-")
	if i := strings.IndexFunc(out, notIdentifier); i >= 0 {
		out = out[:i]
	}

	return out
}

func notIdentifier(ch rune) bool {
	switch {
	case 'a' <= ch && ch <= 'z':
		return false
	case 'A' <= ch && ch <= 'Z':
		return false
	case '0' <= ch && ch <= '9':
		return false
	case ch == '_':
		return false
	case ch >= utf8.RuneSelf && (unicode.IsLetter(ch) || unicode.IsDigit(ch)):
		return false
	default:
		return true
	}
}
