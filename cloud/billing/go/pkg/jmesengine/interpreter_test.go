package jmesengine

import (
	"testing"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
)

type commonInterpreterTestSuite struct {
	intr *TreeInterpreter
}

func (suite *commonInterpreterTestSuite) SetupSuite() {
	suite.intr = NewInterpreter()
}

type interpreterOriginalTestSuite struct {
	suite.Suite
	commonInterpreterTestSuite
}

func TestInterpreterOriginal(t *testing.T) {
	suite.Run(t, new(interpreterOriginalTestSuite))
}

func (suite *interpreterOriginalTestSuite) TestGetKey() {
	ast, _ := jmesparse.NewParser().Parse("foo")
	json := fastjson.MustParse(`{"foo":"bar"}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Equal("bar", string(got.Value().GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestSliceKey() {
	ast, _ := jmesparse.NewParser().Parse("b[].foo")
	json := fastjson.MustParse(`{"a":"foo", "b":[{"foo":"b1", "bar":"f1"},{"foo":"b2", "bar":"f2"}]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 2)
	suite.Equal("b1", string(arr[0].GetStringBytes()))
	suite.Equal("b2", string(arr[1].GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestWithProjection() {
	ast, _ := jmesparse.NewParser().Parse("b[? `true` ].foo")
	json := fastjson.MustParse(`{"a":"foo", "b":[{"foo":"b1", "bar":"f1"},{"foo":"b2", "bar":"f2"}]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 2)
	suite.Equal("b1", string(arr[0].GetStringBytes()))
	suite.Equal("b2", string(arr[1].GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestWithSlice() {
	ast, _ := jmesparse.NewParser().Parse("b[-1].foo")
	json := fastjson.MustParse(`{"a":"foo", "b":[{"foo":"b1", "bar":"f1"},{"foo":"correct", "bar":"f2"}]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Equal("correct", string(got.Value().GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestWithOr() {
	ast, _ := jmesparse.NewParser().Parse("c || a")
	json := fastjson.MustParse(`{"a":"foo", "c": null}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Equal("foo", string(got.Value().GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestWithNested() {
	ast, _ := jmesparse.NewParser().Parse("a.b")
	json := fastjson.MustParse(`{"a":{ "b": "foo"} }`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Equal("foo", string(got.Value().GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestFlatten() {
	ast, _ := jmesparse.NewParser().Parse("a[].b[].foo")
	json := fastjson.MustParse(`
		{"a":[
			{ "b": [
				{"foo": "f1a"},
				{"foo": "f1b"}
			]},
			{ "b": [
				{"foo": "f2a"},
				{"foo": "f2b"}
			]}
		]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 4)
	suite.Equal("f1a", string(arr[0].GetStringBytes()))
	suite.Equal("f1b", string(arr[1].GetStringBytes()))
	suite.Equal("f2a", string(arr[2].GetStringBytes()))
	suite.Equal("f2b", string(arr[3].GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestFlattenEmptySlice() {
	ast, _ := jmesparse.NewParser().Parse("a[].b[].foo")
	json := fastjson.MustParse(`
		{"a":[
			{ "b": []},
			{ "b": [
				{"foo": "f2a"},
				{"foo": "f2b"}
			]}
		]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 2)
	suite.Equal("f2a", string(arr[0].GetStringBytes()))
	suite.Equal("f2b", string(arr[1].GetStringBytes()))
}

func (suite *interpreterOriginalTestSuite) TestProjections() {
	ast, _ := jmesparse.NewParser().Parse("a[*].foo")
	json := fastjson.MustParse(`
		{"a":[
			{"foo": 1},
			{"foo": 2},
			{"foo": 3}
		]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 3)
	suite.Equal(1, arr[0].GetInt())
	suite.Equal(2, arr[1].GetInt())
	suite.Equal(3, arr[2].GetInt())
}

func (suite *interpreterOriginalTestSuite) TestFunc() {
	ast, _ := jmesparse.NewParser().Parse("length(@)")
	json := fastjson.MustParse(`[
		{"foo": "bar"},
		{"foo": "bar"},
		{"foo": "bar"}
	]`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Equal(3, got.Value().GetInt())
}

type interpreterTestSuite struct {
	// Some additional tests written while compliance tests debug
	suite.Suite
	commonInterpreterTestSuite
}

func TestInterpreter(t *testing.T) {
	suite.Run(t, new(interpreterTestSuite))
}

func (suite *interpreterTestSuite) TestFilterLiteral() {
	ast, _ := jmesparse.NewParser().Parse("*[?[0] == `0`]")
	json := fastjson.MustParse(`{"foo": [0, 1], "bar": [2, 3]}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 2)
	suite.Equal(fastjson.TypeArray, arr[0].Type())
	suite.Equal(fastjson.TypeArray, arr[1].Type())
}

func (suite *interpreterTestSuite) TestToNumberProjection() {
	ast, _ := jmesparse.NewParser().Parse("[].to_number(@)")
	json := fastjson.MustParse(`[-1, 3, 4, 5, "a", "100"]`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	arr := got.Value().GetArray()
	suite.Require().Len(arr, 5)
	suite.Equal(-1, arr[0].GetInt())
	suite.Equal(3, arr[1].GetInt())
	suite.Equal(4, arr[2].GetInt())
	suite.Equal(5, arr[3].GetInt())
	suite.Equal(100, arr[4].GetInt())
}

func (suite *interpreterTestSuite) TestWildcard() {
	ast, _ := jmesparse.NewParser().Parse("*.*")
	json := fastjson.MustParse(`{"type": "object"}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Require().Equal(fastjson.TypeArray, got.Value().Type())
	arr := got.Value().GetArray()
	suite.Require().Empty(arr)
}

func (suite *interpreterTestSuite) TestTagsStorage() {
	ast, err := jmesparse.NewParser().Parse("tags.storage_class == `STANDARD`")
	suite.Require().NoError(err)
	json := fastjson.MustParse(`{"tags":{"storage_class":"STANDARD"}}`)

	got, err := suite.intr.Execute(ast, Value(json))
	suite.Require().NoError(err)
	suite.Require().NotNil(got.Value())
	suite.Require().Equal(fastjson.TypeTrue, got.Value().Type(), got.Value().Type().String())
}

func BenchmarkInterpretSingleFieldObject(b *testing.B) {
	intr := NewInterpreter()
	parser := jmesparse.NewParser()
	ast, _ := parser.Parse("fooasdfasdfasdfasdf")
	data := fastjson.MustParse(`{"fooasdfasdfasdfasdf": "foobarbazqux"}`)
	ev := Value(data)
	for i := 0; i < b.N; i++ {
		_, _ = intr.Execute(ast, ev)
	}
}

func BenchmarkInterpretNestedObjects(b *testing.B) {
	intr := NewInterpreter()
	parser := jmesparse.NewParser()
	ast, _ := parser.Parse("fooasdfasdfasdfasdf.fooasdfasdfasdfasdf.fooasdfasdfasdfasdf.fooasdfasdfasdfasdf")
	data := fastjson.MustParse(`{
		"fooasdfasdfasdfasdf": {
			"fooasdfasdfasdfasdf": {
				"fooasdfasdfasdfasdf": {
					"fooasdfasdfasdfasdf": "foobarbazqux"
				}
			}
		}
	}`)
	ev := Value(data)
	for i := 0; i < b.N; i++ {
		_, _ = intr.Execute(ast, ev)
	}
}
