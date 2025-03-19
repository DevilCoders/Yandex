package skuresolve

import (
	"testing"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
)

type jmesTestSuite struct {
	suite.Suite
}

func TestJMES(t *testing.T) {
	suite.Run(t, new(jmesTestSuite))
}

func (suite *jmesTestSuite) TestParse() {
	ast, err := forceParseJMES("key")
	suite.Require().NoError(err)
	suite.Require().NotNil(ast)

	suite.NotEqual(jmesparse.ASTEmpty, ast.node.NodeType)
	suite.False(ast.hasStatic)
	suite.Equal(fastjson.TypeNull, ast.staticResult.Value().Type())
}

func (suite *jmesTestSuite) TestParseStatic() {
	ast, err := forceParseJMES("'key'")
	suite.Require().NoError(err)
	suite.Require().NotNil(ast)

	suite.NotEqual(jmesparse.ASTEmpty, ast.node.NodeType)
	suite.True(ast.hasStatic)
	suite.Equal(fastjson.TypeString, ast.staticResult.Value().Type())
	suite.EqualValues("key", ast.staticResult.Value().GetStringBytes())
}

func (suite *jmesTestSuite) TestParseSyntaxError() {
	ast, err := forceParseJMES("'key")
	suite.Require().Error(err)
	suite.Require().Nil(ast)
}

func (suite *jmesTestSuite) TestParseStaticExecError() {
	ast, err := forceParseJMES("add('this is', 'type error')")
	suite.Require().Error(err)
	suite.Require().Nil(ast)
}
