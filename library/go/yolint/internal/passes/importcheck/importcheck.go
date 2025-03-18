package importcheck

import (
	"errors"
	"fmt"
	"go/ast"
	"go/token"
	"sort"
	"strconv"
	"strings"

	"golang.org/x/tools/go/analysis"

	"a.yandex-team.ru/library/go/yolint/internal/passes/nogen"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	Name = "importcheck"

	localPrefix = "a.yandex-team.ru/"
)

const (
	groupStd = iota
	groupVendored
	groupLocal
)

var groupNames = []string{
	groupStd:      "stdlib",
	groupVendored: "vendored",
	groupLocal:    "local",
}

var (
	ErrImportsNotFormatted = errors.New("imports are not formatted")
)

var Analyzer = &analysis.Analyzer{
	Name: Name,
	Doc:  `importcheck checks that your imports are properly formatted`,
	Run:  run,
	Requires: []*analysis.Analyzer{
		nogen.Analyzer,
	},
}

func run(pass *analysis.Pass) (interface{}, error) {
	nogenFiles := lintutils.ResultOf(pass, nogen.Name).(*nogen.Files)

	for _, file := range nogenFiles.List() {
		var importsCount int
		var errorEncountered error

		for _, decl := range file.Decls {
			genDecl, ok := decl.(*ast.GenDecl)
			if !ok || genDecl.Tok != token.IMPORT {
				continue
			}

			importsCount++
			if importsCount > 1 {
				pass.Reportf(genDecl.Pos(), "multiple imports must be merged into one")
				errorEncountered = ErrImportsNotFormatted
				continue
			}

			err := inspectImportDecl(pass, genDecl)
			if err != nil {
				errorEncountered = err
			}
		}

		if errorEncountered != nil {
			pass.Reportf(file.Pos(), "use yoimports to reformat file imports")
		}
	}

	return nil, nil
}

type importsGroup struct {
	start token.Pos
	end   token.Pos
	specs []*ast.ImportSpec
}

func (g importsGroup) Empty() bool {
	return len(g.specs) == 0 &&
		g.start == token.NoPos &&
		g.end == token.NoPos
}

func (g importsGroup) Len() int {
	return len(g.specs)
}

func (g importsGroup) Less(i, j int) bool {
	return g.specs[i].Path.Value < g.specs[j].Path.Value
}

func (g importsGroup) Swap(i, j int) {
	g.specs[i], g.specs[j] = g.specs[j], g.specs[i]
}

func inspectImportDecl(pass *analysis.Pass, decl *ast.GenDecl) (err error) {
	// do not inspect one-line imports
	if len(decl.Specs) == 1 {
		return nil
	}

	// store imports by groups
	imports := []*importsGroup{
		groupStd:      new(importsGroup),
		groupVendored: new(importsGroup),
		groupLocal:    new(importsGroup),
	}

	for _, spec := range decl.Specs {
		importSpec, ok := spec.(*ast.ImportSpec)
		if !ok {
			continue
		}

		groupID := detectImportGroup(importSpec)
		group := imports[groupID]

		group.specs = append(group.specs, importSpec)
		if group.start == token.NoPos || group.start > spec.Pos() {
			group.start = spec.Pos()
		}
		if group.end == token.NoPos || group.end < spec.Pos() {
			group.end = spec.Pos()
		}
	}

	// check import groups are sorted properly
	var groupsUnsorted bool
	for curID, curGroup := range imports {
		if curGroup.Empty() {
			// skip empty group
			continue
		}

		// check imports in single group sorted properly
		if len(curGroup.specs) > 1 && !sort.IsSorted(curGroup) {
			pass.Reportf(curGroup.start, fmt.Sprintf("%s imports should be sorted alpabetically", groupNames[curID]))
			err = ErrImportsNotFormatted
		}

		for nextID, nextGroup := range imports {
			if nextGroup.Empty() || curID == nextID {
				// do not compare to empty group or self
				continue
			}

			// check that groups are not in proper order
			if (curID < nextID && nextGroup.start < curGroup.end) ||
				(curID > nextID && nextGroup.start > curGroup.end) {
				groupsUnsorted = true
			}

			// check that each following group divided by blank line
			if curID < nextID {
				curGroupEnd := pass.Fset.File(curGroup.end).Line(curGroup.end)
				nextGroupStart := pass.Fset.File(nextGroup.start).Line(nextGroup.start)
				if nextGroupStart > curGroupEnd && (nextGroupStart-curGroupEnd) < 2 {
					pass.Reportf(nextGroup.start, "import groups should be divided by blank line")
					err = ErrImportsNotFormatted
				}
			}
		}
	}

	if groupsUnsorted {
		pass.Reportf(decl.Pos(), "import groups must follow order: stdlib group then vendored then local group")
		err = ErrImportsNotFormatted
	}

	return
}

func detectImportGroup(spec *ast.ImportSpec) int {
	importString, _ := strconv.Unquote(spec.Path.Value)

	if strings.HasPrefix(importString, localPrefix) {
		return groupLocal
	}
	if !strings.Contains(importString, ".") {
		return groupStd
	}
	return groupVendored
}
