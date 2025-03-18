package exhaustivestruct

import (
	"go/ast"
	"go/token"
	"go/types"
	"path/filepath"
	"strings"

	"golang.org/x/tools/go/analysis"
	"golang.org/x/tools/go/analysis/passes/inspect"
	"golang.org/x/tools/go/ast/inspector"

	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

// Analyzer that checks if all struct's fields are initialized
var Analyzer = &analysis.Analyzer{
	Name:      "exhaustivestruct",
	Doc:       "Checks if all struct's fields are initialized",
	Run:       run,
	Requires:  []*analysis.Analyzer{inspect.Analyzer},
	FactTypes: []analysis.Fact{new(toBeChecked), new(toBeIgnored)},
}

type toBeChecked struct{}

func (*toBeChecked) AFact()         {}
func (*toBeChecked) String() string { return "exhaustivestruct-to-be-checked" }

type toBeIgnored struct{}

func (*toBeIgnored) AFact()         {}
func (*toBeIgnored) String() string { return "exhaustivestruct-to-be-ignored" }

const (
	magicStringIgnore = "exhaustivestruct:ignore"
	magicStringSkip   = "exhaustivestruct:skip"
)

func run(pass *analysis.Pass) (interface{}, error) {
	markPackageToBeChecked(pass)
	markIgnoredStruct(pass)
	if shouldSkipPackage(pass) {
		return nil, nil
	}
	checkPackage(pass)
	return nil, nil
}

func checkPackage(pass *analysis.Pass) {
	ins := pass.ResultOf[inspect.Analyzer].(*inspector.Inspector)

	nodeFilter := []ast.Node{
		(*ast.CompositeLit)(nil),
		(*ast.ReturnStmt)(nil),
	}

	var lastReturnStmt *ast.ReturnStmt

	ins.Preorder(nodeFilter, func(node ast.Node) {
		// skip _test.go files
		if lintutils.IsTest(pass, node) {
			return
		}

		compositeLit, ok := node.(*ast.CompositeLit)
		if !ok {
			node := node.(*ast.ReturnStmt)
			// Keep track of the last return statement while iterating
			lastReturnStmt = node
			return
		}

		if compositeLit.Type == nil {
			return
		}
		t := pass.TypesInfo.TypeOf(compositeLit.Type)
		if t == nil {
			return
		}
		name, structDef, ok := getStructDef(pass, compositeLit.Type)
		if !ok {
			return
		}
		if !pass.ImportPackageFact(structDef.Pkg(), new(toBeChecked)) {
			return
		}
		if pass.ImportObjectFact(structDef, new(toBeIgnored)) {
			return
		}
		str, ok := t.Underlying().(*types.Struct)
		if !ok {
			return
		}
		if ignoreUninitializedStructInReturnStatementWithInitializedError(compositeLit, lastReturnStmt, pass) {
			return
		}

		samePackage := strings.HasPrefix(t.String(), pass.Pkg.Path()+".")

		missing := collectUninitializedFieldNames(str, samePackage, compositeLit)

		if len(missing) == 0 {
			return
		}
		structDefFilePath := pass.Fset.File(structDef.Pos()).Name()
		structDefFilename := filepath.Base(structDefFilePath)
		structDefDir := filepath.Base(filepath.Dir(structDefFilePath))
		structDefLine := pass.Fset.Position(structDef.Pos()).Line
		pass.Reportf(node.Pos(), "%s %s missing in `%s` defined at %s/%s:%d",
			strings.Join(missing, ", "), verbFor(len(missing)), name,
			structDefDir, structDefFilename, structDefLine)
	})
}

func shouldSkipPackage(pass *analysis.Pass) bool {
	for _, f := range pass.Files {
		if f.Doc == nil {
			continue
		}
		for _, comment := range f.Doc.List {
			if strings.Contains(comment.Text, magicStringSkip) {
				return true
			}
		}
	}
	return false
}

func collectUninitializedFieldNames(structTy *types.Struct, samePackage bool, compositeLit *ast.CompositeLit) []string {

	uninitialized := make([]string, 0)

	for i := 0; i < structTy.NumFields(); i++ {

		fieldName := structTy.Field(i).Name()

		if !samePackage && !structTy.Field(i).Exported() {
			continue
		}
		if fieldIsInitialized(compositeLit, i, fieldName) {
			continue
		}
		uninitialized = append(uninitialized, fieldName)
	}
	return uninitialized
}

func fieldIsInitialized(compositeLit *ast.CompositeLit, fieldIndex int, fieldName string) bool {

	for eltIndex, elt := range compositeLit.Elts {

		kv, ok := elt.(*ast.KeyValueExpr)
		if !ok {
			if eltIndex == fieldIndex {
				return true
			}
			continue
		}
		ident, ok := kv.Key.(*ast.Ident)
		if !ok {
			continue
		}
		if ident.Name == fieldName {
			return true
		}
	}
	return false
}

func verbFor(count int) string {
	if count == 1 {
		return "is"
	}
	return "are"
}

// Don't report an error if:
// 1. This composite literal contains no fields and
// 2. It's in a return statement and
// 3. The return statement contains a non-nil error
func ignoreUninitializedStructInReturnStatementWithInitializedError(compositeLit *ast.CompositeLit, returnStmt *ast.ReturnStmt, pass *analysis.Pass) bool {
	if len(compositeLit.Elts) > 0 {
		return false
	}

	// Check if this composite is one of the results the last return statement
	if returnStmt == nil {
		return false
	}
	isInResults := false
	for _, result := range returnStmt.Results {
		compareComposite, ok := result.(*ast.CompositeLit)
		if ok && compareComposite == compositeLit {
			isInResults = true
			break
		}
	}
	if !isInResults {
		return false
	}

	// Check if any of the results has an error type and if that error
	// is set to non-nil (if it's set to nil, the type would be "untyped nil")
	for _, result := range returnStmt.Results {
		if pass.TypesInfo.TypeOf(result).String() == "error" {
			return true
		}
	}
	return false
}

func markPackageToBeChecked(pass *analysis.Pass) {
	if len(pass.Files) == 0 {
		return
	}
	if strings.HasSuffix(pass.Pkg.Path(), ".test") {
		return
	}
	for _, f := range pass.Files {
		if f.Doc == nil {
			continue
		}
		for _, comment := range f.Doc.List {
			if strings.Contains(comment.Text, magicStringIgnore) {
				return
			}
		}
	}
	pass.ExportPackageFact(new(toBeChecked))
}

func markIgnoredStruct(pass *analysis.Pass) {
	ins := pass.ResultOf[inspect.Analyzer].(*inspector.Inspector)
	nodeFilter := []ast.Node{
		(*ast.GenDecl)(nil),
		(*ast.TypeSpec)(nil),
	}
	ins.Preorder(nodeFilter, func(node ast.Node) {
		genDecl, ok := node.(*ast.GenDecl)
		if !ok {
			typeSpec := node.(*ast.TypeSpec)
			file, ok := lintutils.FileOfPos(pass, typeSpec.Pos())
			if !ok {
				return
			}
			if !lintutils.IsGenerated(file) {
				return
			}
			obj := pass.TypesInfo.ObjectOf(typeSpec.Name)
			pass.ExportObjectFact(obj, new(toBeIgnored))
			return
		}
		if genDecl.Tok != token.TYPE {
			return
		}
		if genDecl.Doc == nil {
			return
		}
		if !doIgnoreObjects(pass, genDecl) {
			return
		}
		for _, spec := range genDecl.Specs {
			typeSpec := spec.(*ast.TypeSpec)
			obj := pass.TypesInfo.ObjectOf(typeSpec.Name)
			pass.ExportObjectFact(obj, new(toBeIgnored))
		}
	})
}

func doIgnoreObjects(pass *analysis.Pass, genDecl *ast.GenDecl) bool {
	ignoreObjects := false
	for _, comment := range genDecl.Doc.List {
		if strings.Contains(comment.Text, magicStringIgnore) {
			ignoreObjects = true
			break
		}
	}
	return ignoreObjects
}

func getStructDef(pass *analysis.Pass, ty ast.Expr) (string, types.Object, bool) {
	if i, ok := ty.(*ast.Ident); ok {
		return i.Name, pass.TypesInfo.ObjectOf(i), true
	}
	if s, ok := ty.(*ast.SelectorExpr); ok {
		return s.Sel.Name, pass.TypesInfo.ObjectOf(s.Sel), true
	}
	return "", nil, false
}
