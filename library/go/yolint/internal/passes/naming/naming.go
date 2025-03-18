package naming

import (
	"errors"
	"fmt"
	"go/ast"
	"go/token"
	"strings"
	"unicode"

	"golang.org/x/tools/go/analysis"
	"golang.org/x/tools/go/ast/astutil"
	"golang.org/x/tools/go/ast/inspector"

	"a.yandex-team.ru/library/go/yolint/internal/passes/nogen"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	Name = "naming"
)

var Analyzer = &analysis.Analyzer{
	Name: Name,
	Doc:  `naming complains if any use underscores or incorrect known initialisms`,
	Run:  run,
	Requires: []*analysis.Analyzer{
		nogen.Analyzer,
	},
}

func run(pass *analysis.Pass) (interface{}, error) {
	nogenFiles := lintutils.ResultOf(pass, nogen.Name).(*nogen.Files)

	ins := inspector.New(nogenFiles.List())

	// We filter only files.
	nodeFilter := []ast.Node{
		(*ast.File)(nil),
	}

	ins.Preorder(nodeFilter, func(node ast.Node) {
		lintNames(pass, node)
	})

	return nil, nil
}

func hasUnderscore(s string) bool {
	return strings.Contains(s, "_")
}

func inMixedCaps(s string) bool {
	var hasLower, hasUpper bool
	for _, r := range s {
		if unicode.IsLower(r) {
			hasLower = true
		}
		if unicode.IsUpper(r) {
			hasUpper = true
		}
		if hasLower && hasUpper {
			return true
		}
	}
	return false
}

func inAllCaps(s string) bool {
	for _, r := range s {
		if unicode.IsLower(r) {
			return false
		}
	}
	return true
}

// knownNameExceptions is a set of names that are known to be exempt from naming checks.
// This is usually because they are constrained by having to match names in the
// standard library.
var knownNameExceptions = map[string]bool{
	"LastInsertId": true, // must match database/sql
	"kWh":          true,
}

// lintNames examines all names in the file.
// It complains if any use underscores or incorrect known initialisms.
func lintNames(pass *analysis.Pass, node ast.Node) {
	f := node.(*ast.File)

	// Package names need slightly different handling than other names.
	if strings.Contains(f.Name.Name, "_") && !strings.HasSuffix(f.Name.Name, "_test") {
		pass.Reportf(f.Name.End(), "don't use an underscore in package name")
	}
	if inAllCaps(f.Name.Name) {
		pass.Reportf(f.Name.End(), "don't use MixedCaps in package name; %s should be %s", f.Name.Name, strings.ToLower(f.Name.Name))
	}

	check := func(id *ast.Ident, thing string) error {
		if id.Name == "_" {
			return nil
		}
		if knownNameExceptions[id.Name] {
			return nil
		}

		// Handle two common styles from other languages that don't belong in Go.
		if len(id.Name) >= 5 && inAllCaps(id.Name) && strings.Contains(id.Name, "_") {
			capCount := 0
			for _, c := range id.Name {
				if 'A' <= c && c <= 'Z' {
					capCount++
				}
			}
			if capCount >= 2 {
				return errors.New("don't use ALL_CAPS in Go names; use CamelCase")
			}
		}
		if thing == "const" || (thing == "var" && isInTopLevel(f, id)) {
			if len(id.Name) > 2 && id.Name[0] == 'k' && id.Name[1] >= 'A' && id.Name[1] <= 'Z' {
				should := string(id.Name[1]+'a'-'A') + id.Name[2:]
				return fmt.Errorf("don't use leading k in Go names; %s %s should be %s", thing, id.Name, should)
			}
		}

		should := lintName(id.Name)
		if id.Name == should {
			return nil
		}

		if len(id.Name) > 2 && strings.Contains(id.Name[1:], "_") {
			return fmt.Errorf("don't use underscores in Go names; %s %s should be %s", thing, id.Name, should)
		}

		return fmt.Errorf("%s %s should be %s", thing, id.Name, should)
	}

	checkList := func(fl *ast.FieldList, thing string) []error {
		if fl == nil {
			return nil
		}

		var errs []error
		for _, field := range fl.List {
			for _, id := range field.Names {
				if err := check(id, thing); err != nil {
					errs = append(errs, err)
				}
			}
		}

		return errs
	}

	visitor := func(node ast.Node) bool {
		switch v := node.(type) {
		case *ast.AssignStmt:
			if v.Tok == token.ASSIGN {
				return true
			}
			for _, exp := range v.Lhs {
				if id, ok := exp.(*ast.Ident); ok {
					if err := check(id, "var"); err != nil {
						pass.Reportf(id.End(), err.Error())
					}
				}
			}
		case *ast.FuncDecl:
			if lintutils.IsTest(pass, f) && (strings.HasPrefix(v.Name.Name, "Example") || strings.HasPrefix(v.Name.Name, "Test") || strings.HasPrefix(v.Name.Name, "Benchmark")) {
				return true
			}

			thing := "func"
			if v.Recv != nil {
				thing = "method"
			}

			// Exclude naming warnings for functions that are exported to C but
			// not exported in the Go API.
			// See https://github.com/golang/lint/issues/144.
			if ast.IsExported(v.Name.Name) || !lintutils.IsCgoExported(v) {
				if err := check(v.Name, thing); err != nil {
					pass.Reportf(v.Name.End(), err.Error())
				}
			}

			errs := checkList(v.Type.Params, thing+" parameter")
			for _, err := range errs {
				pass.Reportf(v.Type.Params.End(), err.Error())
			}
			errs = checkList(v.Type.Results, thing+" result")
			for _, err := range errs {
				pass.Reportf(v.Type.Results.End(), err.Error())
			}
		case *ast.GenDecl:
			if v.Tok == token.IMPORT {
				return true
			}
			var thing string
			switch v.Tok {
			case token.CONST:
				thing = "const"
			case token.TYPE:
				thing = "type"
			case token.VAR:
				thing = "var"
			}
			for _, spec := range v.Specs {
				switch s := spec.(type) {
				case *ast.TypeSpec:
					if err := check(s.Name, thing); err != nil {
						pass.Reportf(s.Name.End(), err.Error())
					}
				case *ast.ValueSpec:
					for _, id := range s.Names {
						if err := check(id, thing); err != nil {
							pass.Reportf(id.End(), err.Error())
						}
					}
				}
			}
		case *ast.InterfaceType:
			// Do not check interface method names.
			// They are often constrainted by the method names of concrete types.
			for _, x := range v.Methods.List {
				ft, ok := x.Type.(*ast.FuncType)
				if !ok { // might be an embedded interface name
					continue
				}

				errs := checkList(ft.Params, "interface method parameter")
				for _, err := range errs {
					pass.Reportf(ft.Params.End(), err.Error())
				}
				errs = checkList(ft.Results, "interface method result")
				for _, err := range errs {
					pass.Reportf(ft.Results.End(), err.Error())
				}
			}
		case *ast.RangeStmt:
			if v.Tok == token.ASSIGN {
				return true
			}
			if id, ok := v.Key.(*ast.Ident); ok {
				if err := check(id, "range var"); err != nil {
					pass.Reportf(id.End(), err.Error())
				}
			}
			if id, ok := v.Value.(*ast.Ident); ok {
				if err := check(id, "range var"); err != nil {
					pass.Reportf(id.End(), err.Error())
				}
			}
		case *ast.StructType:
			for _, f := range v.Fields.List {
				for _, id := range f.Names {
					if err := check(id, "struct field"); err != nil {
						pass.Reportf(id.End(), err.Error())
					}
				}
			}
		}
		return true
	}

	ast.Inspect(f, visitor)
}

func isInTopLevel(f *ast.File, ident *ast.Ident) bool {
	path, _ := astutil.PathEnclosingInterval(f, ident.Pos(), ident.End())
	for _, f := range path {
		switch f.(type) {
		case *ast.File, *ast.GenDecl, *ast.ValueSpec, *ast.Ident:
			continue
		}
		return false
	}
	return true
}

// lintName returns a different name if it should be different.
func lintName(name string) (should string) {
	// Fast path for simple cases: "_" and all lowercase.
	if name == "_" {
		return name
	}
	allLower := true
	for _, r := range name {
		if !unicode.IsLower(r) {
			allLower = false
			break
		}
	}
	if allLower {
		return name
	}

	// Split camelCase at any lower->upper transition, and split on underscores.
	// Check each word for common initialisms.
	runes := []rune(name)
	w, i := 0, 0 // index of start of word, scan
	for i+1 <= len(runes) {
		eow := false // whether we hit the end of a word
		if i+1 == len(runes) {
			eow = true
		} else if runes[i+1] == '_' {
			// underscore; shift the remainder forward over any run of underscores
			eow = true
			n := 1
			for i+n+1 < len(runes) && runes[i+n+1] == '_' {
				n++
			}

			// Leave at most one underscore if the underscore is between two digits
			if i+n+1 < len(runes) && unicode.IsDigit(runes[i]) && unicode.IsDigit(runes[i+n+1]) {
				n--
			}

			copy(runes[i+1:], runes[i+n+1:])
			runes = runes[:len(runes)-n]
		} else if unicode.IsLower(runes[i]) && !unicode.IsLower(runes[i+1]) {
			// lower->non-lower
			eow = true
		}
		i++
		if !eow {
			continue
		}

		// [w,i) is a word.
		word := string(runes[w:i])
		if u := strings.ToUpper(word); commonInitialisms[u] {
			// Keep consistent case, which is lowercase only at the start.
			if w == 0 && unicode.IsLower(runes[w]) {
				u = strings.ToLower(u)
			}
			// All the common initialisms are ASCII,
			// so we can replace the bytes exactly.
			copy(runes[w:], []rune(u))
		} else if w > 0 && strings.ToLower(word) == word {
			// already all lowercase, and not the first word, so uppercase the first character.
			runes[w] = unicode.ToUpper(runes[w])
		}
		w = i
	}
	return string(runes)
}

// commonInitialisms is a set of common initialisms.
// Only add entries that are highly unlikely to be non-initialisms.
// For instance, "ID" is fine (Freudian code is rare), but "AND" is not.
var commonInitialisms = map[string]bool{
	"ACL":   true,
	"API":   true,
	"ASCII": true,
	"CPU":   true,
	"CSS":   true,
	"DNS":   true,
	"EOF":   true,
	"GUID":  true,
	"HTML":  true,
	"HTTP":  true,
	"HTTPS": true,
	"ID":    true,
	"IP":    true,
	"JSON":  true,
	"LHS":   true,
	"QPS":   true,
	"RAM":   true,
	"RHS":   true,
	"RPC":   true,
	"SLA":   true,
	"SMTP":  true,
	"SQL":   true,
	"SSH":   true,
	"TCP":   true,
	"TLS":   true,
	"TTL":   true,
	"UDP":   true,
	"UI":    true,
	"UID":   true,
	"UUID":  true,
	"URI":   true,
	"URL":   true,
	"UTF8":  true,
	"VM":    true,
	"XML":   true,
	"XMPP":  true,
	"XSRF":  true,
	"XSS":   true,
}
