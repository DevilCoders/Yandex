package cfgtypes

import (
	"fmt"
	"testing"
	"time"

	"github.com/imdario/mergo"
	"github.com/stretchr/testify/suite"
)

type secondsTestSuite struct {
	suite.Suite
}

func TestSeconds(t *testing.T) {
	suite.Run(t, new(secondsTestSuite))
}

func (suite *secondsTestSuite) TestMarshal() {
	cases := []struct {
		arg  Seconds
		want string
	}{
		{Seconds(time.Second), "1"},
		{Seconds(time.Millisecond * 999), "0.999"},
	}

	for _, c := range cases {
		suite.Run(c.want, func() {
			got, _ := c.arg.MarshalText()
			suite.Equal(c.want, string(got))
		})
	}
}

func (suite *secondsTestSuite) TestUnmarshal() {
	cases := []struct {
		arg  string
		want Seconds
	}{
		{"1", Seconds(time.Second)},
		{"999ms", Seconds(time.Millisecond * 999)},
	}

	for _, c := range cases {
		suite.Run(c.arg, func() {
			var got Seconds
			_ = got.UnmarshalText([]byte(c.arg))
			suite.EqualValues(c.want, got, got.String())
		})
	}
}

type dataSizeTestSuite struct {
	suite.Suite
}

func TestDataSize(t *testing.T) {
	suite.Run(t, new(dataSizeTestSuite))
}

func (suite *dataSizeTestSuite) TestMarshal() {
	cases := []struct {
		arg  DataSize
		want string
	}{
		{1, "1"},
		{1024, "1KiB"},
		{1024 * 1024, "1MiB"},
		{1024 * 1024 * 1024, "1GiB"},
		{1024 * 1024 * 1024 * 1024, "1TiB"},
		{1024 * 1024 * 1024 * 1024 * 1024, "1PiB"},
	}

	for _, c := range cases {
		suite.Run(c.want, func() {
			got, _ := c.arg.MarshalText()
			suite.Equal(c.want, string(got))
		})
	}
}

func (suite *dataSizeTestSuite) TestUnmarshal() {
	cases := []struct {
		arg  string
		want DataSize
	}{
		{"1", 1},
		{"1KiB", 1024},
		{"1MiB", 1024 * 1024},
		{"1GiB", 1024 * 1024 * 1024},
		{"1TiB", 1024 * 1024 * 1024 * 1024},
		{"1PiB", 1024 * 1024 * 1024 * 1024 * 1024},
	}

	for _, c := range cases {
		suite.Run(c.arg, func() {
			var got DataSize
			_ = got.UnmarshalText([]byte(c.arg))
			suite.EqualValues(c.want, got, got.String())
		})
	}
}

type overridableBoolTestSuite struct {
	suite.Suite
}

func TestOverridableBool(t *testing.T) {
	suite.Run(t, new(overridableBoolTestSuite))
}

func (suite *overridableBoolTestSuite) TestMarshal() {
	cases := []struct {
		arg  OverridableBool
		want string
	}{
		{BoolDefault, "false"},
		{BoolFalse, "false"},
		{BoolTrue, "true"},
	}

	for _, c := range cases {
		suite.Run(c.want, func() {
			got, _ := c.arg.MarshalText()
			suite.Equal(c.want, string(got))
		})
	}
}

func (suite *overridableBoolTestSuite) TestUnmarshal() {
	cases := []struct {
		arg  string
		want OverridableBool
	}{
		{"1", BoolTrue},
		{"t", BoolTrue},
		{"T", BoolTrue},
		{"true", BoolTrue},
		{"TRUE", BoolTrue},
		{"True", BoolTrue},
		{"0", BoolFalse},
		{"f", BoolFalse},
		{"F", BoolFalse},
		{"false", BoolFalse},
		{"FALSE", BoolFalse},
		{"False", BoolFalse},
	}

	for _, c := range cases {
		suite.Run(c.arg, func() {
			var got OverridableBool
			_ = got.UnmarshalText([]byte(c.arg))
			suite.EqualValues(c.want, got)
		})
	}
}

func (suite *overridableBoolTestSuite) TestMerge() {
	cases := []struct {
		given    OverridableBool
		override OverridableBool
		want     OverridableBool
	}{
		{BoolDefault, BoolDefault, BoolDefault},
		{BoolFalse, BoolDefault, BoolFalse},
		{BoolTrue, BoolDefault, BoolTrue},
		{BoolDefault, BoolFalse, BoolFalse},
		{BoolFalse, BoolFalse, BoolFalse},
		{BoolTrue, BoolFalse, BoolFalse},
		{BoolDefault, BoolTrue, BoolTrue},
		{BoolFalse, BoolTrue, BoolTrue},
		{BoolTrue, BoolTrue, BoolTrue},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			got := boolOverrideMerge{Value: c.given}
			_ = mergo.MergeWithOverwrite(&got, boolOverrideMerge{Value: c.override})
			suite.Equal(c.want, got.Value)
		})
	}
}

type boolOverrideMerge struct {
	Value OverridableBool
}
