package parsers

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
)

func TestMatcherFullMethodParser(t *testing.T) {
	inputs := []struct {
		Name     string
		Data     string
		Expected *expectations.MatcherFullMethod
		Error    bool
	}{
		{
			Name:  "Empty",
			Data:  `{}`,
			Error: true,
		},
		{
			Name: "Expect",
			Data: `{"expect": {"foo": {"type":"matcher", "name": "full_method", "expect": {}}, "bar": {"type":"matcher", "name": "full_method", "expect": {}}}}`,
			Expected: &expectations.MatcherFullMethod{
				Matchers: map[string]expectations.Matcher{
					"foo": &expectations.MatcherFullMethod{Matchers: map[string]expectations.Matcher{}},
					"bar": &expectations.MatcherFullMethod{Matchers: map[string]expectations.Matcher{}},
				},
			},
		},
	}

	matcherParser := &matcherFullMethodParser{}
	parser := NewEmptyParser()
	require.NoError(t, parser.RegisterMatcherParser(matcherParser))
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			v, err := fastjson.Parse(input.Data)
			require.NoError(t, err)
			o, err := v.Object()
			require.NoError(t, err)

			r, err := matcherParser.ParseMatcher(parser.parseExpectation, o)
			if input.Error {
				require.Error(t, err)
				require.Nil(t, r)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Expected, r)
		})
	}
}

func TestMatcherArrayParser(t *testing.T) {
	inputs := []struct {
		Name     string
		Data     string
		Expected *expectations.MatcherArray
		Error    bool
	}{
		{
			Name:  "Empty",
			Data:  `{}`,
			Error: true,
		},
		{
			Name: "Expect",
			Data: `{"expect": [{"type":"matcher", "name": "array", "expect": []}, {"type":"matcher", "name": "array", "expect": []}]}`,
			Expected: &expectations.MatcherArray{
				Matchers: []expectations.Matcher{
					&expectations.MatcherArray{Matchers: make([]expectations.Matcher, 0)},
					&expectations.MatcherArray{Matchers: make([]expectations.Matcher, 0)},
				},
			},
		},
	}

	matcherParser := &matcherArrayParser{}
	parser := NewEmptyParser()
	require.NoError(t, parser.RegisterMatcherParser(matcherParser))
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			v, err := fastjson.Parse(input.Data)
			require.NoError(t, err)
			o, err := v.Object()
			require.NoError(t, err)

			r, err := matcherParser.ParseMatcher(parser.parseExpectation, o)
			if input.Error {
				require.Error(t, err)
				require.Nil(t, r)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Expected, r)
		})
	}
}

type testMatcher struct {
	someHardcodedValue string
}

var _ expectations.Matcher = &testMatcher{}

func (m *testMatcher) Match(_ context.Context, _ expectations.MatchInputs) (expectations.Responder, error) {
	return nil, nil
}

func newMockedParseExpectation(m expectations.Matcher, err error) ExpectationParser {
	return func(_ *fastjson.Object) (expectations.Matcher, error) {
		return m, err
	}
}

func TestMatcherContainsParser(t *testing.T) {
	inputs := []struct {
		Name     string
		Data     string
		Expected *expectations.MatcherContains
		Error    bool
	}{
		{
			Name:  "Empty",
			Data:  `{}`,
			Error: true,
		},
		{
			Name: "Expect",
			Data: `{"request": "{\"foo\": \"bar\"}", "expect": {}}`,
			Expected: &expectations.MatcherContains{
				Expected: map[string]interface{}{"foo": "bar"},
				Matcher:  &testMatcher{"foobar"},
			},
		},
	}

	matcherParser := &matcherContainsParser{}
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			v, err := fastjson.Parse(input.Data)
			require.NoError(t, err)
			o, err := v.Object()
			require.NoError(t, err)

			r, err := matcherParser.ParseMatcher(newMockedParseExpectation(&testMatcher{"foobar"}, nil), o)
			if input.Error {
				require.Error(t, err)
				require.Nil(t, r)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Expected, r)
		})
	}
}
