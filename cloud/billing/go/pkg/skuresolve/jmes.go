package skuresolve

import (
	"fmt"
	"time"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/jmesengine"
	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
)

type jmesAST struct {
	node         jmesparse.ASTNode
	staticResult jmesengine.ExecValue
	hasStatic    bool
}

var (
	jmesParseCache = ccache.New(
		ccache.Configure().MaxSize(10000),
	)
	jmesEngine = jmesengine.NewInterpreter()
)

func parseJMES(expression JMESPath) (ast *jmesAST, err error) {
	strExp := string(expression)
	item, err := jmesParseCache.Fetch(strExp, time.Hour, func() (interface{}, error) {
		return forceParseJMES(strExp)
	})
	if err != nil {
		return nil, err
	}
	return item.Value().(*jmesAST), nil
}

func forceParseJMES(expr string) (*jmesAST, error) {
	parser := jmesparse.NewParser()
	parsed, err := parser.Parse(expr)
	if err != nil {
		return nil, fmt.Errorf("JMES parse error: %w", err)
	}
	ast := jmesAST{node: parsed}
	if isStaticJmes(parsed) {
		ast.staticResult, err = jmesEngine.Execute(parsed, jmesengine.Value(nil))
		if err != nil {
			return nil, fmt.Errorf("JMES static expression exec error: %w", err)
		}
		ast.hasStatic = true
	}
	return &ast, nil
}

// isStaticJmes checks if jmes formula does not depends on metric value, i.e. "`true`"
func isStaticJmes(ast jmesparse.ASTNode) bool {
	switch ast.NodeType {
	case jmesparse.ASTEmpty,
		jmesparse.ASTExpRef,
		jmesparse.ASTLiteral:
		return true
	case jmesparse.ASTComparator,
		jmesparse.ASTFunctionExpression,
		jmesparse.ASTKeyValPair,
		jmesparse.ASTOrExpression,
		jmesparse.ASTAndExpression,
		jmesparse.ASTNotExpression,
		jmesparse.ASTPipe,
		jmesparse.ASTProjection,
		jmesparse.ASTSubexpression,
		jmesparse.ASTIndexExpression,
		jmesparse.ASTValueProjection:
		for _, c := range ast.Children {
			if !isStaticJmes(c) {
				return false
			}
		}
		return true

	/* jmesparse.ASTField,
	jmesparse.ASTFilterProjection,
	jmesparse.ASTFlatten,
	jmesparse.ASTIndex,
	jmesparse.ASTMultiSelectHash,
	jmesparse.ASTMultiSelectList,
	jmesparse.ASTCurrentNode,
	jmesparse.ASTIdentity,
	jmesparse.ASTSlice
	*/
	default:
		return false
	}
}
