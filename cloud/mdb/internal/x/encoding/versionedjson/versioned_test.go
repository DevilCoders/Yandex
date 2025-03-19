package versionedjson

import (
	"encoding/json"
	"fmt"
	"strconv"
	"testing"

	"github.com/stretchr/testify/require"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Version struct {
	Major int `json:"major"`
	Minor int `json:"minor"`
}

type VersionedData interface {
	GetFoo() string
	GetBar() string
}

type ValidStruct struct {
	Version         Version       `json:"version" version:"version,key"`
	Data            VersionedData `json:"data" version:"versioned,key"`
	OtherData       int           `json:"other_data"`
	SecondOtherData int           `json:"second_other_data"`
}

type VersionOne string

func (v *VersionOne) GetFoo() string {
	return string(*v)
}

const (
	nonExistentBar = "BAR DOES NOT EXIST IN VERSION ONE"
)

func (v *VersionOne) GetBar() string {
	return nonExistentBar
}

func versionedParser(version interface{}, data *fastjson.Value, dst interface{}, parsers Parsers, subParser SubParserFunc) (interface{}, error) {
	v := version.(Version)
	switch v {
	case Version{Major: 1}:
		strBytes, err := data.StringBytes()
		if err != nil {
			return nil, err
		}

		v1 := VersionOne(strBytes)
		return &v1, nil
	case Version{Major: 2}:
		var v2 VersionTwo
		return subParser(data, &v2, parsers)
	case Version{Major: 2, Minor: 1}:
		var v2p1 VersionTwoPointOne
		return subParser(data, &v2p1, parsers)
	case Version{Major: 3}:
		var v3 VersionThree
		return subParser(data, &v3, parsers)
	default:
		return nil, xerrors.Errorf("unknown version %+v", v)
	}
}

type VersionTwo struct {
	Foo int64  `json:"foo"`
	Bar string `json:"bar"`
}

func (v *VersionTwo) GetFoo() string {
	return strconv.FormatInt(v.Foo, 10)
}

func (v *VersionTwo) GetBar() string {
	return v.Bar
}

type VersionTwoPointOne struct {
	Foo string `json:"foo"`
	Bar int64  `json:"bar"`
}

func (v *VersionTwoPointOne) GetFoo() string {
	return v.Foo
}

func (v *VersionTwoPointOne) GetBar() string {
	return strconv.FormatInt(v.Bar, 10)
}

type VersionThree struct {
	Foo        string        `json:"foo"`
	BarVersion Version       `json:"bar_version" version:"version,key"`
	Bar        VersionedData `json:"bar" version:"versioned,key"`
}

func (v *VersionThree) GetFoo() string {
	return v.Foo
}

func (v *VersionThree) GetBar() string {
	return fmt.Sprintf("Foo: %q Bar: %q", v.Bar.GetFoo(), v.Bar.GetBar())
}

type customUnmarshalRoot struct {
	Version     string `version:"version,key"`
	Versioned   string `version:"versioned,key"`
	Unversioned customUnmarshalField
}

type customUnmarshalField struct {
	Foo string
}

func (c *customUnmarshalField) UnmarshalJSON(data []byte) error {
	var bar struct {
		Bar string `json:"bar"`
	}
	if err := json.Unmarshal(data, &bar); err != nil {
		return err
	}

	c.Foo = bar.Bar
	return nil
}

func customUnmarshalVersionedParser(version interface{}, data *fastjson.Value, dst interface{}, parsers Parsers, subParser SubParserFunc) (interface{}, error) {
	_ = version.(string)
	strBytes, err := data.StringBytes()
	if err != nil {
		return nil, err
	}

	return string(strBytes), nil
}

func TestParse(t *testing.T) {
	inputs := []struct {
		Name                    string
		JSON                    string
		ExpectedVersion         Version
		ExpectedOtherData       int
		ExpectedSecondOtherData int
		ExpectedFoo             string
		ExpectedBar             string
	}{
		{
			Name:              "One",
			JSON:              `{"version": {"major": 1, "minor": 0}, "data": "146", "other_data": 42}`,
			ExpectedVersion:   Version{Major: 1},
			ExpectedOtherData: 42,
			ExpectedFoo:       "146",
			ExpectedBar:       nonExistentBar,
		},
		{
			Name:              "Two",
			JSON:              `{"version": {"major": 2, "minor": 0}, "data": {"foo": 52, "bar": "146"}, "other_data": 42}`,
			ExpectedVersion:   Version{Major: 2},
			ExpectedOtherData: 42,
			ExpectedFoo:       "52",
			ExpectedBar:       "146",
		},
		{
			Name:              "TwoPointOne",
			JSON:              `{"version": {"major": 2, "minor": 1}, "data": {"foo": "146", "bar": 52}, "other_data": 42}`,
			ExpectedVersion:   Version{Major: 2, Minor: 1},
			ExpectedOtherData: 42,
			ExpectedFoo:       "146",
			ExpectedBar:       "52",
		},
		{
			Name:              "Three",
			JSON:              `{"version": {"major": 3, "minor": 0}, "data": {"foo": "146", "bar_version": {"major": 2, "minor": 0}, "bar": {"foo": 3, "bar": "6"}}, "other_data": 42}`,
			ExpectedVersion:   Version{Major: 3},
			ExpectedOtherData: 42,
			ExpectedFoo:       "146",
			ExpectedBar:       `Foo: "3" Bar: "6"`,
		},
		{
			Name:                    "NonVersionedData",
			JSON:                    `{"version": {"major": 1, "minor": 0}, "data": "146", "other_data": 42, "second_other_data": 24}`,
			ExpectedVersion:         Version{Major: 1},
			ExpectedOtherData:       42,
			ExpectedSecondOtherData: 24,
			ExpectedFoo:             "146",
			ExpectedBar:             nonExistentBar,
		},
		{
			Name:                    "NonVersionedPartialData",
			JSON:                    `{"version": {"major": 1, "minor": 0}, "data": "146", "second_other_data": 24}`,
			ExpectedVersion:         Version{Major: 1},
			ExpectedSecondOtherData: 24,
			ExpectedFoo:             "146",
			ExpectedBar:             nonExistentBar,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var dst ValidStruct
			require.NoError(t, Parse([]byte(input.JSON), &dst, map[string]ParserFunc{"key": versionedParser}))
			require.Equal(t, input.ExpectedVersion, dst.Version)
			require.Equal(t, input.ExpectedOtherData, dst.OtherData)
			require.Equal(t, input.ExpectedSecondOtherData, dst.SecondOtherData)
			require.NotNil(t, dst.Data)
			require.Equal(t, input.ExpectedFoo, dst.Data.GetFoo())
			require.Equal(t, input.ExpectedBar, dst.Data.GetBar())
		})
	}

	t.Run("UnmarshalJSON", func(t *testing.T) {
		var dst customUnmarshalRoot
		require.NoError(t, Parse([]byte(`{"version": "1", "versioned": "2", "unversioned": {"bar": "stuff"}}`), &dst, map[string]ParserFunc{"key": customUnmarshalVersionedParser}))
		require.Equal(t, "1", dst.Version)
		require.Equal(t, "2", dst.Versioned)
		require.Equal(t, "stuff", dst.Unversioned.Foo)
	})

	t.Run("Embed pointer to a struct", func(t *testing.T) {
		type WithEmbedPointer struct {
			Version Version       `json:"version" version:"version,key"`
			Data    VersionedData `json:"data" version:"versioned,key"`
			Flag    *struct {
				Disabled *bool `json:"disabled"`
			} `json:"flag,omitempty"`
		}

		var dst WithEmbedPointer
		require.NoError(t, Parse([]byte(`{"version": {"major": 1, "minor": 0}, "data": "146", "other_data": 42, "flag": {"disabled": true}}`), &dst, map[string]ParserFunc{"key": versionedParser}))
		require.NotNil(t, dst.Flag)
		require.NotNil(t, dst.Flag.Disabled)
		require.True(t, *dst.Flag.Disabled)
	})

	t.Run("Embed pointer to a struct and initialized dst", func(t *testing.T) {
		type Flag struct {
			Disabled *bool `json:"disabled,omitempty"`
			Colored  bool  `json:"colored,omitempty"`
		}
		type WithEmbedPointer struct {
			Version Version       `json:"version" version:"version,key"`
			Data    VersionedData `json:"data" version:"versioned,key"`
			Flag    *Flag         `json:"flag,omitempty"`
		}

		var dst WithEmbedPointer
		dst.Flag = &Flag{Colored: true}
		require.NoError(t, Parse([]byte(`{"version": {"major": 1, "minor": 0}, "data": "146", "other_data": 42, "flag": {"disabled": true}}`), &dst, map[string]ParserFunc{"key": versionedParser}))
		require.NotNil(t, dst.Flag)
		require.NotNil(t, dst.Flag.Disabled)
		require.True(t, *dst.Flag.Disabled)
		require.True(t, dst.Flag.Colored)
	})

	t.Run("Embed pointer to a primitive values", func(t *testing.T) {
		type WithPointerToPrimitive struct {
			Version Version       `json:"version" version:"version,key"`
			Data    VersionedData `json:"data" version:"versioned,key"`
			String  *string       `json:"string"`
			Int     *int          `json:"int"`
		}

		var dst WithPointerToPrimitive
		require.NoError(t, Parse([]byte(`{"version": {"major": 1, "minor": 0}, "data": "146", "other_data": 42, "string": "Yo", "int": 42}`), &dst, map[string]ParserFunc{"key": versionedParser}))
		require.NotNil(t, dst.String)
		require.Equal(t, "Yo", *dst.String)
		require.NotNil(t, dst.Int)
		require.Equal(t, 42, *dst.Int)
	})
}

func TestParseErrors(t *testing.T) {
	inputs := []struct {
		Name          string
		JSON          string
		ExpectedError string
	}{
		{
			Name:          "InvalidVersion",
			JSON:          `{"version": {"foo": "bar"}, "data": "146", "other_data": 42}`,
			ExpectedError: "versioned parser for key \"key\": unknown version {Major:0 Minor:0}",
		},
		{
			Name:          "UnknownVersion",
			JSON:          `{"version": {"major": 0, "minor": 0}, "data": "146", "other_data": 42}`,
			ExpectedError: "versioned parser for key \"key\": unknown version {Major:0 Minor:0}",
		},
		{
			Name:          "VersionButNoVersioned",
			JSON:          `{"version": {"major": 0, "minor": 0}, "other_data": 42}`,
			ExpectedError: "has version field \"Version\" but not versioned field \"Data\"",
		},
		{
			Name:          "VersionedButNoVersion",
			JSON:          `{"data": "146", "other_data": 42}`,
			ExpectedError: "has versioned field \"Data\" but not version field \"Version\"",
		},
		{
			Name:          "InvalidObjectType",
			JSON:          `{"version": true, "data": "146", "other_data": 42}`,
			ExpectedError: "non-versioned field \"Version\": invalid field type: value doesn't contain object; it contains true",
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var dst ValidStruct
			err := Parse([]byte(input.JSON), &dst, map[string]ParserFunc{"key": versionedParser})
			require.EqualError(t, err, input.ExpectedError)
		})
	}

	t.Run("NoParser", func(t *testing.T) {
		var dst struct {
			Version   string `version:"version,key"`
			Versioned string `version:"versioned,key"`
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, nil)
		require.EqualError(t, err, "no parser for key \"key\"")
	})

	t.Run("MissingVersion", func(t *testing.T) {
		var dst struct {
			Version   string
			Versioned string `version:"versioned,key"`
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, nil)
		require.EqualError(t, err, "key \"key\" missing version field")
	})

	t.Run("MissingVersioned", func(t *testing.T) {
		var dst struct {
			Version   string `version:"version,key"`
			Versioned string
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, nil)
		require.EqualError(t, err, "key \"key\" missing versioned field")
	})

	t.Run("DoubleVersion", func(t *testing.T) {
		var dst struct {
			Version   string `version:"version,key"`
			Versioned string `version:"version,key"`
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, nil)
		require.EqualError(t, err, "double version tag for key \"key\"")
	})

	t.Run("DoubleVersioned", func(t *testing.T) {
		var dst struct {
			Version   string `version:"versioned,key"`
			Versioned string `version:"versioned,key"`
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, nil)
		require.EqualError(t, err, "double versioned tag for key \"key\"")
	})

	t.Run("VersionedParserError", func(t *testing.T) {
		parser := func(version interface{}, data *fastjson.Value, dst interface{}, parsers Parsers, subParser SubParserFunc) (interface{}, error) {
			return nil, xerrors.New("versionedParserError")
		}
		var dst struct {
			Version   string `version:"version,key"`
			Versioned string `version:"versioned,key"`
		}
		err := Parse([]byte(`{"version": "foo", "versioned": "bar"}`), &dst, map[string]ParserFunc{"key": parser})
		require.EqualError(t, err, "versioned parser for key \"key\": versionedParserError")
	})
}
