package structs

import (
	"encoding/json"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

func TestContainsMapper(t *testing.T) {
	suite.Run(t, new(ContainsPathTestSuite))
}

func TestEqualMapper(t *testing.T) {
	suite.Run(t, new(EqualStructMapperTestSuite))
}

func TestRegexpStructMapper(t *testing.T) {
	suite.Run(t, new(RegexpStructMapperTestSuite))
}

func TestBillingStructUnmarshal(t *testing.T) {
	suite.Run(t, new(BillingStructUnmarshalTestSuite))
}

type ContainsPathTestSuite struct {
	suite.Suite
}

func (suite *ContainsPathTestSuite) TestContainsIntPath() {
	var a struct {
		IntField int64 `json:"foo"`
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("foo"), "structure does not contain 'foo' tag")
	suite.False(s.ContainsPath("zoo"), "should not contain 'zoo' tag")
}

func (suite *ContainsPathTestSuite) TestContainsAsSnakePathIntPath() {
	var a struct {
		IntField int64 `json:"fooBar"`
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("foo_bar"), "structure does not contain 'foo' tag")
	suite.False(s.ContainsPath("zoo"), "should not contain 'zoo' tag")
	suite.False(s.ContainsPath("zoo_bar"), "should not contain 'zoo_bar' tag")
	suite.False(s.ContainsPath("fooBar"), "should not contain 'fooBar' tag")
}

func (suite *ContainsPathTestSuite) TestContainsAsSnakePathStringPath() {
	var a struct {
		StringField string `json:"FooBar"`
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("foo_bar"), "structure does not contain 'foo' tag")
	suite.False(s.ContainsPath("zoo"), "should not contain 'zoo' tag")
	suite.False(s.ContainsPath("zoo_bar"), "should not contain 'zoo_bar' tag")
	suite.False(s.ContainsPath("fooBar"), "should not contain 'fooBar' tag")
	suite.False(s.ContainsPath("FooBar"), "should not contain 'FooBar' tag")
}

func (suite *ContainsPathTestSuite) TestContainsStringPath() {
	var a struct {
		StringField string `json:"fooZoo"`
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("foo_zoo"), "structure does not contain 'fooZoo' tag")
	suite.False(s.ContainsPath("fooZoo"), "structure does not contain 'fooZoo' tag")
	suite.False(s.ContainsPath("foo"), "should not contain 'foo' tag")
}

func (suite *ContainsPathTestSuite) TestContainsFloatPathWithOmitEmpty() {
	var a struct {
		StringField string `json:"some_float,omitempty"`
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("some_float"), "structure does not contain 'foo' tag")
	suite.False(s.ContainsPath("zoo"), "should not contain 'zoo' tag")
}

func (suite *ContainsPathTestSuite) TestContainsWithInnerPath() {
	var a struct {
		Float float64 `json:"some_float,omitempty"`
		Inner struct {
			Int    int64  `json:"zoo"`
			String string `json:"bar"`
		} `json:"inner"`

		InnerNotTag struct {
			Int int64 `json:"boo"`
		}
	}

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.ContainsPath("some_float"), "structure does not contain 'foo' tag")
	suite.True(s.ContainsPath("inner.zoo"), "structure does not contain 'inner.zoo' tag")
	suite.True(s.ContainsPath("inner.bar"), "structure does not contain 'inner.bar' tag")

	suite.False(s.ContainsPath("zoo"), "should not contain 'zoo' tag")
	suite.False(s.ContainsPath("boo"), "should not contain 'zoo' tag")
	suite.False(s.ContainsPath("inner.boo"), "structure does not contain 'foo' tag")
}

type EqualStructMapperTestSuite struct {
	suite.Suite
}

func (suite *EqualStructMapperTestSuite) TestEqualFloat() {
	var a struct {
		Float      float64 `json:"some_float,omitempty"`
		OtherFloat float64 `json:"other_float,omitempty"`
	}

	a.Float = 1.0

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.Equals("some_float", a.Float), "mismatch value %f", a.Float)
	suite.False(s.Equals("other_float", a.Float), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualInt() {
	var a struct {
		Int      int64 `json:"some_int,omitempty"`
		OtherInt int64 `json:"other_int,omitempty"`
	}

	a.Int = 100500

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.True(s.Equals("some_int", a.Int), "mismatch value %f", a.Int)
	suite.False(s.Equals("other_int", a.Int), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualString() {
	var a struct {
		String      string `json:"some_string,omitempty"`
		OtherString string `json:"other_string,omitempty"`
	}

	a.String = "abc"

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("some_string", a.String), "mismatch value %s", a.String)
	suite.Require().False(s.Equals("other_string", a.String), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualSnakeCaseString() {
	var a struct {
		StringSnake string `json:"some_string,omitempty"`
		OtherString string `json:"other_string,omitempty"`
	}

	a.StringSnake = "abc"

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("some_string", a.StringSnake), "mismatch value %s", a.StringSnake)
	suite.Require().False(s.Equals("other_string", a.StringSnake), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualInnerStructString() {
	var a struct {
		String      string `json:"some_string,omitempty"`
		InnerStruct struct {
			String      string `json:"inner_string,omitempty"`
			OtherString string `json:"other_string,omitempty"`
		} `json:"bar"`
	}

	a.InnerStruct.String = "abc"

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("bar.inner_string", a.InnerStruct.String), "mismatch value %+v, but expect %s", s.Value("bar.inner_string"), a.InnerStruct.String)
	suite.Require().False(s.Equals("bar.other_string", a.InnerStruct.String), "values are equal")
	suite.Require().False(s.Equals("bar.none_string", a.InnerStruct.String), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualInnerStructWithSnakeCaseStringOk() {
	var a struct {
		String      string `json:"some_string,omitempty"`
		InnerStruct struct {
			StringSnake string `json:"inner_string,omitempty"`
			OtherString string `json:"other_string,omitempty"`
		} `json:"bar"`
	}

	a.InnerStruct.StringSnake = "abc"

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("bar.inner_string", a.InnerStruct.StringSnake), "mismatch value %+v, but expect %s", s.Value("bar.inner_string"), a.InnerStruct.StringSnake)
	suite.Require().False(s.Equals("bar.other_string", a.InnerStruct.StringSnake), "values are equal")
	suite.Require().False(s.Equals("bar.other_string", a.InnerStruct.StringSnake), "values are equal")
	suite.Require().False(s.Equals("bar.none_string", a.InnerStruct.StringSnake), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestEqualInnerStructWithSnakeCaseAbsent() {
	var a struct {
		String      string `json:"some_string,omitempty"`
		InnerStruct struct {
			StringCamel string `json:"innerString,omitempty"`
			StringSnake string `json:"inner_string,omitempty"`
			OtherString string `json:"other_string,omitempty"`
		} `json:"bar"`
	}

	a.InnerStruct.StringCamel = "abc"

	s, err := NewMapping(&a)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("bar.inner_string", a.InnerStruct.StringCamel), "mismatch value %+v, but expect %s", s.Value("bar.inner_string"), a.InnerStruct.StringCamel)
	suite.Require().False(s.Equals("bar.other_string", a.InnerStruct.StringCamel), "values are equal")
	suite.Require().False(s.Equals("bar.none_string", a.InnerStruct.StringCamel), "values are equal")
}
func (suite *EqualStructMapperTestSuite) TestEqualIntSlice() {
	var a struct {
		IntSlice      []int64 `json:"some_int_slice,omitempty"`
		OtherIntSlice []int64 `json:"other_int_slice,omitempty"`
	}

	a.IntSlice = []int64{1, 2, 3}

	s, err := NewMapping(a)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("some_int_slice", []int64{1, 2, 3}), "mismatch value %+v", s.Value("some_int_slice"))
	suite.Require().False(s.Equals("other_int_slice", []int64{1, 2, 3}), "values are equal")
}

func (suite *EqualStructMapperTestSuite) TestJSONNumber() {
	var source struct {
		IntValue json.Number `json:"int"`
	}

	source.IntValue = json.Number("100500")

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("int", "100500"), "mismatch value %+v", s.Value("int"))
	suite.Require().False(s.Equals("int", "42"), "values are equal %+v", s.Value("int"))
}

func (suite *EqualStructMapperTestSuite) TestBillingSchemelessInt() {
	var source struct {
		IntValue billing.SchemelessInt `json:"int"`
	}

	source.IntValue.Value = "042"

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("int", "042"), "mismatch value %+v", s.Value("int"))
	suite.Require().False(s.Equals("int", "42"), "values are equal %+v", s.Value("int"))
}

func (suite *EqualStructMapperTestSuite) TestJSONNumberNull() {
	var source struct {
		IntValue json.Number `json:"int"`
	}

	s, err := NewMapping(&source)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Equals("int", ""), "mismatch value %+v", s.Value("int"))
	suite.Require().True(s.ContainsPath("int"), "path 'int' doesn't exist")
	suite.Require().False(s.ContainsPathWithValue("int"), "mismatch value %+v", s.Value("int"))
}

type RegexpStructMapperTestSuite struct {
	suite.Suite
}

func (suite *EqualStructMapperTestSuite) TestMatchOk() {
	var source struct {
		IntValue billing.SchemelessInt `json:"someInt"`
	}

	source.IntValue.Value = "042"

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Match("some_int", ".42"), "mismatch value %+v", s.Value("some_int"))
	suite.Require().False(s.Match("some_int", "100500-42"), "mismatch value %+v", s.Value("some_int"))
}

func (suite *EqualStructMapperTestSuite) TestMatchSnakeCaseOk() {
	var source struct {
		IntValueSnake billing.SchemelessInt `json:"someInt"`
	}

	source.IntValueSnake.Value = "042"

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.Match("some_int", ".42"), "mismatch value %+v", s.Value("some_int"))
	suite.Require().False(s.Match("some_int", "100500-42"), "mismatch value %+v", s.Value("some_int"))
}

func (suite *EqualStructMapperTestSuite) TestMatchFailure() {
	var source struct {
		IntValue billing.SchemelessInt `json:"someInt"`
	}

	source.IntValue.Value = "some string with 042"

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().False(s.Match("some_int", "^100500.*42$"), "mismatch value %+v", s.Value("some_int"))
}

type BillingStructUnmarshalTestSuite struct {
	suite.Suite
}

func (suite *BillingStructUnmarshalTestSuite) TestCamelCase() {
	const sourceJSON = `
    {
		"person": {
        	"company": {
				"email": "gimli@yandex.ru",
				"inn": "0100500",
				"kpp": "42",
				"legalAddress": "Mordor",
				"longname": "Thorin's company",
				"name": "Fellowship of the Ring",
				"phone": "+100200300",
				"postAddress":"Middle Earth",
				"postCode":"43"
        	}
		}
	}
	`
	var source billing.ExtendedBillingAccountCamelCaseView

	err := json.Unmarshal([]byte(sourceJSON), &source)
	suite.Require().NoError(err)

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	suite.Require().True(s.ContainsPath("person.company"))
	suite.Require().False(s.ContainsPath("person.individual"))

	suite.Require().True(s.Equals("person.company.email", "gimli@yandex.ru"), "unexpected email value %s", s.Value("person.company.email"))
	suite.Require().True(s.Equals("person.company.post_address", "Middle Earth"), "unexpected post address value %s", s.Value("person.company.post_address"))
	suite.Require().True(s.Equals("person.company.post_code", "43"), "unexpected post code value %s", s.Value("person.company.post_code"))
}

func (suite *BillingStructUnmarshalTestSuite) TestSnakeCase() {
	// suite.T().Skip()

	const sourceJSON = `
    {
		"person": {
        	"company": {
				"email": "gimli@yandex.ru",
				"inn": "0100500",
				"kpp": "42",
				"legal_address": "Mordor",
				"longname": "Thorin's company",
				"name": "Fellowship of the Ring",
				"phone": "+100200300",
				"post_address":"Middle Earth",
				"post_code":"43"
        	}
		}
	}
	`
	var source billing.ExtendedBillingAccountCamelCaseView

	err := json.Unmarshal([]byte(sourceJSON), &source)
	suite.Require().NoError(err)

	s, err := NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(s)

	fmt.Printf("STRUCT:%+v\n", s)

	suite.Require().True(s.ContainsPath("person.company"), "subpath should be present")
	suite.Require().False(s.ContainsPath("person.individual"), "subpath should be empty")

	suite.Require().True(s.Equals("person.company.email", "gimli@yandex.ru"), "unexpected email value %s", s.Value("person.company.email"))
	suite.Require().True(s.Equals("person.company.post_address", "Middle Earth"), "unexpected post address value %s", s.Value("person.company.post_address"))
	suite.Require().True(s.Equals("person.company.post_code", "43"), "unexpected post code value %s", s.Value("person.company.post_code"))
	suite.Require().True(s.Equals("person.company.legal_address", "Mordor"), "unexpected post code value %s", s.Value("person.company.legal_address"))

	suite.Require().False(s.Equals("person.company.email", "some.mail@yandex.ru"), "unexpected email value %s", s.Value("person.company.email"))
	suite.Require().False(s.Equals("person.company.post_address", "Mordor"), "unexpected post address value %s", s.Value("person.company.post_address"))
	suite.Require().False(s.Equals("person.company.post_code", "0043"), "unexpected post code value %s", s.Value("person.company.post_code"))
	suite.Require().False(s.Equals("person.company.legal_address", "Shire"), "unexpected post code value %s", s.Value("person.company.legal_address"))

}
