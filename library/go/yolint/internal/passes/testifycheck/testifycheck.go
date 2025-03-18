package testifycheck

import (
	"fmt"
	"go/ast"
	"go/types"

	"golang.org/x/tools/go/analysis"
	"golang.org/x/tools/go/ast/inspector"
)

const (
	Name = "testifycheck"
)

var Analyzer = &analysis.Analyzer{
	Name: Name,
	Doc:  `check common errors when working with testify`,
	Run:  run,
}

func isTestifyPkg(path string) bool {
	switch path {
	case "github.com/stretchr/testify/assert":
		return true
	case "github.com/stretchr/testify/require":
		return true
	default:
		return false
	}
}

func isNilAssertion(pass *analysis.Pass, fn *ast.SelectorExpr) (pkgName, kind string, argIndex int, ok bool) {
	xType := pass.TypesInfo.TypeOf(fn.X)
	if ptrType, isPtr := xType.(*types.Pointer); isPtr {
		named, isNamed := ptrType.Elem().(*types.Named)
		if !isNamed {
			return
		}

		pkg := named.Obj().Pkg()
		if pkg == nil || !isTestifyPkg(pkg.Path()) {
			return
		}

		if named.Obj().Name() != "Assertions" {
			return
		}

		switch fn.Sel.Name {
		case "Nil", "NotNil", "Nilf", "NotNilf":
			return pkg.Name(), fn.Sel.Name, 0, true

		default:
			return
		}
	} else {
		obj := pass.TypesInfo.ObjectOf(fn.Sel)

		pkg := obj.Pkg()
		if pkg == nil || !isTestifyPkg(pkg.Path()) {
			return
		}

		switch obj.Name() {
		case "Nil", "NotNil", "Nilf", "NotNilf":
			return pkg.Name(), obj.Name(), 1, true

		default:
			return
		}
	}
}

func errorAssertFor(name string) string {
	switch name {
	case "Nil":
		return "NoError"
	case "Nilf":
		return "NoErrorf"
	case "NotNil":
		return "Error"
	case "NotNilf":
		return "Errorf"
	default:
		panic(fmt.Sprintf("unknown assertion: %s", name))
	}
}

func isErrorArg(pass *analysis.Pass, args []ast.Expr, argIndex int) bool {
	if len(args) < argIndex+1 {
		return false
	}

	argType := pass.TypesInfo.TypeOf(args[argIndex])
	return argType.String() == "error"
}

func run(pass *analysis.Pass) (interface{}, error) {
	ins := inspector.New(pass.Files)

	// We filter only function calls.
	nodeFilter := []ast.Node{
		(*ast.CallExpr)(nil),
	}

	ins.Preorder(nodeFilter, func(n ast.Node) {
		call := n.(*ast.CallExpr)

		fn, ok := call.Fun.(*ast.SelectorExpr)
		if !ok {
			return
		}

		pkg, kind, argIndex, ok := isNilAssertion(pass, fn)
		if ok && isErrorArg(pass, call.Args, argIndex) {
			pass.Reportf(call.Pos(), "use %s.%s instead of comparing error to nil", pkg, errorAssertFor(kind))
		}
	})

	return nil, nil
}
