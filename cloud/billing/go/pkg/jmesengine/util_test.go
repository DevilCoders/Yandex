package jmesengine

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type utilsTestSuite struct {
	suite.Suite
}

func TestUtils(t *testing.T) {
	suite.Run(t, new(utilsTestSuite))
}

func (suite *utilsTestSuite) TestIsFalse() {
	ar := fastjson.Arena{}
	cases := []*fastjson.Value{
		nil,
		ar.NewFalse(),
		ar.NewNull(),
		ar.NewArray(),
		ar.NewObject(),
		ar.NewString(""),
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			val := "<nil>"
			if c != nil {
				val = string(c.MarshalTo(nil))
			}
			suite.True(IsFalse(Value(c)), val)
		})
	}
}

func (suite *utilsTestSuite) TestNotIsFalse() {
	ar := fastjson.Arena{}
	arr := ar.NewArray()
	arr.SetArrayItem(0, ar.NewNull())
	obj := ar.NewObject()
	obj.Set("k", ar.NewFalse())

	cases := []*fastjson.Value{
		ar.NewTrue(),
		arr,
		obj,
		ar.NewString("0"),
		ar.NewString("some string"),
		ar.NewNumberInt(0),
		ar.NewNumberInt(999),
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			suite.False(IsFalse(Value(c)), string(c.MarshalTo(nil)))
		})
	}
}

func (suite *utilsTestSuite) TestObjesEqual() {
	ar := fastjson.Arena{}
	obj := ar.NewObject()
	obj.Set("null", ar.NewNull())
	obj.Set("key", ar.NewString("value"))
	arr := ar.NewArray()
	arr.SetArrayItem(0, ar.NewNull())
	arr.SetArrayItem(0, ar.NewString("value"))

	cases := []struct {
		left, right *fastjson.Value
	}{
		{nil, nil},
		{ar.NewNull(), nil},
		{ar.NewNull(), ar.NewNull()},
		{ar.NewTrue(), ar.NewTrue()},
		{ar.NewFalse(), ar.NewFalse()},
		{ar.NewString(""), ar.NewString("")},
		{ar.NewString("string"), ar.NewString("string")},
		{ar.NewString("999"), ar.NewNumberInt(999)},
		{ar.NewNumberInt(999), ar.NewNumberInt(999)},
		{obj, obj},
		{arr, arr},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			lv, rv := "<nil>", "<nil>"
			if c.left != nil {
				lv = string(c.left.MarshalTo(nil))
			}
			if c.right != nil {
				rv = string(c.right.MarshalTo(nil))
			}
			suite.True(objsEqual(c.left, c.right), "%s==%s", lv, rv)
			suite.True(objsEqual(c.right, c.left), "reverse %s==%s", lv, rv)
		})
	}
}

func (suite *utilsTestSuite) TestObjesEqualDiffTypes() {
	ar := fastjson.Arena{}
	obj := ar.NewObject()
	obj.Set("null", ar.NewNull())
	obj.Set("key", ar.NewString("value"))
	arr := ar.NewArray()
	arr.SetArrayItem(0, ar.NewNull())
	arr.SetArrayItem(0, ar.NewString("value"))

	types := []*fastjson.Value{
		ar.NewNull(),
		ar.NewTrue(),
		ar.NewFalse(),
		ar.NewString(""),
		ar.NewString("string"),
		ar.NewString("100"),
		ar.NewNumberInt(999),
		obj,
		arr,
	}

	cases := []struct {
		left, right *fastjson.Value
	}{}
	for i, t1 := range types {
		for j, t2 := range types {
			if i != j {
				cases = append(cases, struct{ left, right *fastjson.Value }{t1, t2})
			}
		}
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			lv, rv := "<nil>", "<nil>"
			if c.left != nil {
				lv = string(c.left.MarshalTo(nil))
			}
			if c.right != nil {
				rv = string(c.right.MarshalTo(nil))
			}
			suite.False(objsEqual(c.left, c.right), "%s==%s", lv, rv)
			suite.False(objsEqual(c.right, c.left), "reverse %s==%s", lv, rv)
		})
	}
}

func (suite *utilsTestSuite) TestObjesEqualDiffValues() {
	ar := fastjson.Arena{}

	obj := func(kv map[string]*fastjson.Value) *fastjson.Value {
		o := ar.NewObject()
		for k, v := range kv {
			o.Set(k, v)
		}
		return o
	}
	arr := func(items ...*fastjson.Value) *fastjson.Value {
		a := ar.NewArray()
		for idx, it := range items {
			a.SetArrayItem(idx, it)
		}
		return a
	}

	cases := []struct {
		left, right *fastjson.Value
	}{
		{ar.NewString("string one"), ar.NewString("string two")},
		{ar.NewString("999"), ar.NewNumberInt(1000)},
		{ar.NewNumberInt(999), ar.NewNumberInt(1000)},
		{obj(nil), obj(map[string]*fastjson.Value{"k": ar.NewString("v")})},
		{
			obj(map[string]*fastjson.Value{"k1": ar.NewString("v1")}),
			obj(map[string]*fastjson.Value{"k2": ar.NewString("v2")}),
		},
		{
			obj(map[string]*fastjson.Value{"k": ar.NewString("v1")}),
			obj(map[string]*fastjson.Value{"k": ar.NewString("v2")}),
		},
		{
			obj(map[string]*fastjson.Value{"k": ar.NewString("v1"), "ke": ar.NewNull()}),
			obj(map[string]*fastjson.Value{"k": ar.NewString("v2"), "ke": ar.NewNull()}),
		},
		{arr(), arr(ar.NewString("it"))},
		{
			arr(ar.NewString("it1"), ar.NewString("it2")),
			arr(ar.NewString("it1")),
		},
		{
			arr(ar.NewString("it1"), ar.NewString("it2")),
			arr(ar.NewString("it1"), ar.NewString("it3")),
		},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			lv := string(c.left.MarshalTo(nil))
			rv := string(c.right.MarshalTo(nil))
			suite.False(objsEqual(c.left, c.right), "%s==%s", lv, rv)
			suite.False(objsEqual(c.right, c.left), "reverse %s==%s", lv, rv)
		})
	}
}

func (suite *utilsTestSuite) TestDecimalFromAnyTypes() {
	ar := fastjson.Arena{}

	cases := []struct {
		in      *fastjson.Value
		wantNan bool
	}{
		{nil, true},
		{ar.NewNull(), true},
		{ar.NewTrue(), true},
		{ar.NewFalse(), true},
		{ar.NewString(""), true},
		{ar.NewString("string"), true},
		{ar.NewString("999"), false},
		{ar.NewNumberInt(999), false},
		{ar.NewObject(), true},
		{ar.NewArray(), true},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			d := decimalFromAny(c.in)
			suite.Equal(c.wantNan, d.IsNan(), d.String())
		})
	}
}

func (suite *utilsTestSuite) TestDecimalFromAnyValues() {
	ar := fastjson.Arena{}

	cases := []struct {
		in   *fastjson.Value
		want decimal.Decimal128
	}{
		{ar.NewString("999"), decimal.Must(decimal.FromString("999"))},
		{ar.NewString("-1"), decimal.Must(decimal.FromString("-1"))},
		{ar.NewString("999.99"), decimal.Must(decimal.FromString("999.99"))},
		{ar.NewString("-0.5"), decimal.Must(decimal.FromString("-0.5"))},
		{ar.NewNumberString("999"), decimal.Must(decimal.FromString("999"))},
		{ar.NewNumberString("-1"), decimal.Must(decimal.FromString("-1"))},
		{ar.NewNumberString("999.99"), decimal.Must(decimal.FromString("999.99"))},
		{ar.NewNumberString("-0.5"), decimal.Must(decimal.FromString("-0.5"))},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			d := decimalFromAny(c.in)
			suite.Zero(c.want.Cmp(d), d.String())
		})
	}
}

func (suite *utilsTestSuite) TestBoolToValue() {
	suite.Equal(fastjson.TypeTrue, boolToValue(true).valueType())
	suite.Equal(fastjson.TypeFalse, boolToValue(false).valueType())
}

func (suite *utilsTestSuite) TestArrayToValue() {
	ar := fastjson.Arena{}
	arr := arrayToValue([]*fastjson.Value{ar.NewTrue(), ar.NewFalse()})

	gotArr := arr.Value().GetArray()
	suite.Require().Len(gotArr, 2)
	suite.Equal(fastjson.TypeTrue, gotArr[0].Type())
	suite.Equal(fastjson.TypeFalse, gotArr[1].Type())
}

func (suite *utilsTestSuite) TestObjectValue() {
	obj := objectValue()

	suite.Equal(fastjson.TypeObject, obj.Type())
	suite.Zero(obj.GetObject().Len())
}

func (suite *utilsTestSuite) TestAnyToValue() {
	d := decimal.Must(decimal.FromInt64(42))

	got := anyToValue(d)
	suite.Equal(fastjson.TypeNumber, got.Type())
	suite.EqualValues(42, got.GetInt())
}

func (suite *utilsTestSuite) TestCopyValue() {
	ar := fastjson.Arena{}
	obj := ar.NewObject()
	obj.Set("k", ar.NewString("v"))

	got := copyValue(obj)
	obj.Set("k", ar.NewNull())

	suite.Equal(fastjson.TypeObject, got.Type())
	suite.Equal(fastjson.TypeString, got.Get("k").Type())
	suite.EqualValues("v", got.GetStringBytes("k"))

	suite.Require().Nil(copyValue(nil))
}

func (suite *utilsTestSuite) TestSlicePositiveStep() {
	ar := fastjson.Arena{}

	input := []*fastjson.Value{
		ar.NewNumberInt(0),
		ar.NewNumberInt(1),
		ar.NewNumberInt(2),
		ar.NewNumberInt(3),
		ar.NewNumberInt(4),
	}
	{
		result, err := slice(input, []sliceParam{{0, true}, {3, true}, {1, true}})
		suite.Require().NoError(err)
		suite.Equal(input[:3], result)
	}
	{
		result, err := slice(input, []sliceParam{{3, true}, {0, false}, {1, true}})
		suite.Require().NoError(err)
		suite.Equal(input[3:], result)
	}
	{
		result, err := slice(input, []sliceParam{{1, true}, {4, true}, {1, true}})
		suite.Require().NoError(err)
		suite.Equal(input[1:4], result)
	}
}

func (suite *utilsTestSuite) TestToArrayNum() {
	ar := fastjson.Arena{}
	arr := ar.NewArray()
	arr.SetArrayItem(0, ar.NewNumberInt(0))
	arr.SetArrayItem(1, ar.NewNumberInt(1))
	arr.SetArrayItem(2, ar.NewString("2"))

	dd, ok := toArrayNum(arr)
	suite.Require().True(ok)
	suite.Require().Len(dd, 3)
	suite.Equal([]decimal.Decimal128{
		decimal.Must(decimal.FromInt64(0)),
		decimal.Must(decimal.FromInt64(1)),
		decimal.Must(decimal.FromInt64(2)),
	}, dd)

	arr.SetArrayItem(3, ar.NewString("x"))
	_, ok = toArrayNum(arr)
	suite.Require().False(ok)

	_, ok = toArrayNum(nil)
	suite.Require().False(ok)
}

func (suite *utilsTestSuite) TestToArrayStr() {
	ar := fastjson.Arena{}
	arr := ar.NewArray()
	arr.SetArrayItem(0, ar.NewString("a"))
	arr.SetArrayItem(1, ar.NewString("b"))
	arr.SetArrayItem(2, ar.NewString("c"))

	ss, ok := toArrayStr(arr)
	suite.Require().True(ok)
	suite.Require().Len(ss, 3)
	suite.Equal([]string{"a", "b", "c"}, ss)

	arr.SetArrayItem(3, ar.NewNull())
	_, ok = toArrayStr(arr)
	suite.Require().False(ok)

	_, ok = toArrayStr(nil)
	suite.Require().False(ok)
}
