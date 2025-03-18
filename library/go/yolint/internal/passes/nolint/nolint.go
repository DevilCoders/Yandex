package nolint

import (
	"go/ast"
	"reflect"
	"sort"
	"strings"

	"golang.org/x/tools/go/analysis"

	"a.yandex-team.ru/library/go/yolint/internal/passes/nogen"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	Name = "nolint"

	CommentPrefix = "//nolint:"
)

var Analyzer = &analysis.Analyzer{
	Name:             Name,
	Doc:              `removes Nodes under nolint directives for later passes`,
	Run:              run,
	RunDespiteErrors: true,
	ResultType:       reflect.TypeOf(new(Index)),
	Requires: []*analysis.Analyzer{
		nogen.Analyzer,
	},
}

type Index struct {
	idx map[string][]ast.Node
}

// ForLinter returns subset of excluded nodes specifically for given linter
func (i Index) ForLinter(linter string) *LinterIndex {
	li := &LinterIndex{linter: linter}

	if nodes, ok := i.idx[linter]; ok {
		li.idx = nodes
		sort.Slice(li.idx, func(i, j int) bool {
			return li.idx[i].Pos() < li.idx[j].Pos()
		})
	}

	return li
}

type LinterIndex struct {
	linter string
	idx    []ast.Node
}

func (l LinterIndex) Excluded(node ast.Node) bool {
	// TODO: binary search here
	for _, n := range l.idx {
		if node.Pos() >= n.Pos() && node.End() <= n.End() {
			return true
		}
	}
	return false
}

func run(pass *analysis.Pass) (interface{}, error) {
	files := lintutils.ResultOf(pass, nogen.Name).(*nogen.Files).List()

	// gather nolint index
	index := make(map[string][]ast.Node)

	for _, file := range files {
		for _, cg := range file.Comments {
			linters := getNolintNames(cg)
			if len(linters) == 0 {
				continue
			}

			if node, ok := lintutils.CommentNode(cg, file); ok {
				for _, linter := range linters {
					index[linter] = append(index[linter], node)
				}
			}
		}
	}

	return &Index{idx: index}, nil
}

// getNolintNames returns names of linters from `nolint` comment
func getNolintNames(cg *ast.CommentGroup) []string {
	if cg == nil {
		return nil
	}

	var res []string
	for _, cm := range cg.List {
		if strings.HasPrefix(cm.Text, CommentPrefix) && len(cm.Text) > len(CommentPrefix) {
			res = append(res, cm.Text[len(CommentPrefix):])
		}
	}

	return res
}
