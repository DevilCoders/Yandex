package lintutils

import (
	"go/ast"
	"go/token"

	"golang.org/x/tools/go/analysis"
)

// FileOfReport returns file in which report occurs
func FileOfReport(pass *analysis.Pass, d analysis.Diagnostic) (file *ast.File, found bool) {
	return FileOfPos(pass, d.Pos)
}

func FileOfPos(pass *analysis.Pass, pos token.Pos) (file *ast.File, found bool) {
	for _, f := range pass.Files {
		if f.Pos() <= pos && f.End() >= pos {
			return f, true
		}
	}
	return
}

// NodeOfReport finds node of report
func NodeOfReport(pass *analysis.Pass, d analysis.Diagnostic) (node ast.Node, found bool) {
	file, ok := FileOfReport(pass, d)
	if !ok {
		return nil, false
	}

	ast.Inspect(file, func(n ast.Node) bool {
		if n != nil && n.Pos() == d.Pos {
			node = n
			found = true
		}
		return !found
	})

	return
}
