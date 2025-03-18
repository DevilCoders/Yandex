package errcheck

import (
	"fmt"
	"go/ast"
	"go/types"

	"golang.org/x/tools/go/analysis"
	"golang.org/x/tools/go/ast/inspector"

	"a.yandex-team.ru/library/go/yolint/internal/passes/nogen"
	"a.yandex-team.ru/library/go/yolint/internal/passes/nolint"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	Name = "errcheck"

	errorTypeName   = "error"
	recoverFuncName = "recover"
)

var Analyzer = &analysis.Analyzer{
	Name: Name,
	Doc:  `errcheck checks that you checked errors`,
	Run:  run,
	Requires: []*analysis.Analyzer{
		nolint.Analyzer,
		nogen.Analyzer,
	},
}

func run(pass *analysis.Pass) (interface{}, error) {
	nogenFiles := lintutils.ResultOf(pass, nogen.Name).(*nogen.Files)

	nolintIndex := lintutils.ResultOf(pass, nolint.Name).(*nolint.Index)
	nolintNodes := nolintIndex.ForLinter(Name)

	ins := inspector.New(nogenFiles.List())

	// We filter only function calls.
	nodeFilter := []ast.Node{
		(*ast.CallExpr)(nil), // function call expression
	}

	ins.WithStack(nodeFilter, func(n ast.Node, push bool, stack []ast.Node) bool {
		// do not fall into leaf twice
		if !push {
			return false
		}

		call := n.(*ast.CallExpr)

		// skip nolint node
		if nolintNodes.Excluded(call) {
			return false
		}

		// skip direct check of whitelisted function calls
		if isExcluded(pass, call) {
			return false
		}

		// recover may return error hidden in interface
		if isRecover(call) && !errorConsumed(call, stack) {
			pass.Reportf(call.Lparen, "unhandled potential recover error")
		}

		if returnsError(pass, call) && !errorConsumed(call, stack) {
			pass.Reportf(call.Lparen, "unhandled error")
		}

		return true
	})

	return nil, nil
}

// checks if CallExpr has error type in any return arg
func returnsError(pass *analysis.Pass, call *ast.CallExpr) bool {
	// get extended info for CallExpr
	tnv, ok := pass.TypesInfo.Types[call.Fun]
	if !ok {
		return false
	}

	sig, ok := tnv.Type.(*types.Signature)
	if !ok || sig.Results() == nil {
		return false
	}

	results := sig.Results()
	for i := 0; i < results.Len(); i++ {
		if results.At(i).Type().String() == errorTypeName {
			return true
		}
	}

	return false
}

// checks if error consumed by first able to do so parent
func errorConsumed(call *ast.CallExpr, stack []ast.Node) bool {
	// select stack part without top and bottom (root *ast.File and *ast.callExpr itself)
	substack := stack[1 : len(stack)-1]

	// unwind stack till first parent
	for i := len(substack) - 1; i >= 0; i-- {
		switch substack[i].(type) {
		case *ast.AssignStmt, *ast.GenDecl, *ast.ReturnStmt, *ast.ValueSpec,
			*ast.SelectorExpr, *ast.BinaryExpr, *ast.KeyValueExpr, *ast.CallExpr,
			*ast.SendStmt, *ast.SwitchStmt, *ast.CompositeLit, *ast.RangeStmt:
			// error checked in: assignment, declaration, return, value specification,
			// selector expression, binary or key/value expression, channel send expression,
			// switch statement, composite literal, range statement
			return true
		case *ast.DeferStmt:
			// direct defer call on recover is prohibited
			return !isRecover(call)
		case *ast.BlockStmt:
			// if we hit block statement - error has not been checked
			return false
		}
	}

	return false
}

// checks if function is recover call
func isRecover(call *ast.CallExpr) bool {
	fn, ok := call.Fun.(*ast.Ident)
	return ok && fn.String() == recoverFuncName
}

var whitelist = map[string]struct{}{
	// bytes
	"(*bytes.Buffer).Write":       {},
	"(*bytes.Buffer).WriteByte":   {},
	"(*bytes.Buffer).WriteRune":   {},
	"(*bytes.Buffer).WriteString": {},

	// fmt
	"fmt.Print":   {},
	"fmt.Printf":  {},
	"fmt.Println": {},

	// math/rand
	"math/rand.Read":         {},
	"(*math/rand.Rand).Read": {},

	// strings
	"(*strings.Builder).Write":       {},
	"(*strings.Builder).WriteByte":   {},
	"(*strings.Builder).WriteRune":   {},
	"(*strings.Builder).WriteString": {},

	// hash
	"(hash.Hash).Write": {},
}

var printWhitelist = map[string]struct{}{
	"fmt.Fprint":   {},
	"fmt.Fprintln": {},
	"fmt.Fprintf":  {},
}

var memoryBuffers = map[string]struct{}{
	"*bytes.Buffer":    {},
	"*strings.Builder": {},
	"hash.Hash":        {},
}

func isStdoutOrStderr(pass *analysis.Pass, arg ast.Node) bool {
	if sel, ok := arg.(*ast.SelectorExpr); ok {
		argObj := pass.TypesInfo.ObjectOf(sel.Sel)
		argName := fmt.Sprintf("%s.%s", argObj.Pkg().Path(), argObj.Name())

		if argName == "os.Stderr" || argName == "os.Stdout" {
			return true
		}
	}

	return false
}

// checks if function in whitelist
func isExcluded(pass *analysis.Pass, call *ast.CallExpr) bool {
	sel, ok := call.Fun.(*ast.SelectorExpr)
	if !ok {
		return false
	}

	funcObj, ok := pass.TypesInfo.ObjectOf(sel.Sel).(*types.Func)
	if !ok {
		return false
	}

	// fast path - check using stdlib FQN function
	fqn := funcObj.FullName()
	if _, ok := whitelist[fqn]; ok {
		return true
	}

	// fmt.Fprint{,f,ln} allowed for os.Stderr, os.Stdout, bytes.Buffer, strings.Builder and hash.Hash
	if _, ok := printWhitelist[fqn]; ok && len(call.Args) > 0 {
		if isStdoutOrStderr(pass, call.Args[0]) {
			return true
		}

		// build FQN manually for interface implementations
		writerTyp := pass.TypesInfo.TypeOf(call.Args[0])
		if _, ok := memoryBuffers[writerTyp.String()]; ok {
			return true
		}
	}

	// build FQN manually for interface implementations
	tp := pass.TypesInfo.TypeOf(sel.X)

	var ptrPart string
	if _, ok := tp.(*types.Pointer); ok {
		ptrPart = "*"
	}

	fqn = "(" + ptrPart + tp.String() + ")." + funcObj.Name()
	_, ok = whitelist[fqn]
	return ok
}
