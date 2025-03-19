package ctxtools

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestContextTagsTestSuite(t *testing.T) {
	suite.Run(t, new(ContextTagsTestSuite))
}

type ContextTagsTestSuite struct {
	suite.Suite

	ctx context.Context
}

func (suite *ContextTagsTestSuite) SetupTest() {
	suite.ctx = context.Background()
}

func (suite *ContextTagsTestSuite) TestAddTagsOk() {
	ctx := WithTag(suite.ctx, "boo", "bar")
	values := GetTag(ctx, "boo")

	suite.Require().Len(values, 1)
	suite.Require().Contains(values, "bar")
	suite.Require().NotContains(values, "baz")
}

func (suite *ContextTagsTestSuite) TestAddTagWithSeveralValuesOk() {
	ctx := WithTag(suite.ctx, "boo", "bar", "foo", "baz")
	values := GetTag(ctx, "boo")

	suite.Require().Len(values, 3)
	suite.Require().Contains(values, "bar")
	suite.Require().Contains(values, "foo")
	suite.Require().Contains(values, "baz")
	suite.Require().NotContains(values, "none")
}

func (suite *ContextTagsTestSuite) TestAccessChildTagsFromParent() {
	parentCtx := WithTag(suite.ctx, "parent", "bar")
	child1Ctx := WithTag(parentCtx, "child1", "foo", "bar")
	_ = WithTag(child1Ctx, "child2", "zoo")

	values := GetTag(parentCtx, "parent")

	suite.Require().Len(values, 1)

	suite.Require().Contains(values, "bar")
	suite.Require().NotContains(values, "foo")

	values = GetTag(parentCtx, "child1")

	suite.Require().Len(values, 2)

	suite.Require().Contains(values, "foo")
	suite.Require().Contains(values, "bar")
	suite.Require().NotContains(values, "zoo")

	values = GetTag(parentCtx, "child2")

	suite.Require().Len(values, 1)

	suite.Require().Contains(values, "zoo")
	suite.Require().NotContains(values, "foo")
}

func (suite *ContextTagsTestSuite) TestAddEmptyTag() {
	ctx := WithTag(suite.ctx, "boo")
	values := GetTag(ctx, "boo")

	suite.Require().Empty(values)
}

func (suite *ContextTagsTestSuite) TestNotPresentTag() {
	ctx := WithTag(suite.ctx, "boo")
	values := GetTag(ctx, "zoo")

	suite.Require().Empty(values)
}

func (suite *ContextTagsTestSuite) TestGetWithoutAnyAdd() {
	values := GetTag(suite.ctx, "any tag")
	suite.Require().Empty(values)
}
