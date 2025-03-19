package parsers

import (
	"encoding/json"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MatcherParser interface {
	// Name of matcher used in configuration.
	Name() string
	// ParseMatcher from configuration.
	ParseMatcher(ep ExpectationParser, o *fastjson.Object) (expectations.Matcher, error)
}

type matcherFullMethodParser struct{}

func (p *matcherFullMethodParser) Name() string {
	return "full_method"
}

func (p *matcherFullMethodParser) ParseMatcher(ep ExpectationParser, o *fastjson.Object) (expectations.Matcher, error) {
	expectValue := o.Get("expect")
	if expectValue == nil {
		return nil, xerrors.New("missing expect")
	}

	expectObject, err := expectValue.Object()
	if err != nil {
		return nil, xerrors.Errorf("expect: %w", err)
	}

	matcher := &expectations.MatcherFullMethod{Matchers: make(map[string]expectations.Matcher, expectObject.Len())}
	visitor := func(key []byte, v *fastjson.Value) {
		expectationObject, expectationErr := v.Object()
		if expectationErr != nil {
			err = xerrors.Errorf("expectation object: %w", expectationErr)
			return
		}

		m, expectationErr := ep(expectationObject)
		if expectationErr != nil {
			err = expectationErr
			return
		}

		if _, ok := matcher.Matchers[string(key)]; ok {
			err = xerrors.Errorf("expectation for full method %q already exists", string(key))
			return
		}

		if m != nil {
			matcher.Matchers[string(key)] = m
		}
	}

	expectObject.Visit(visitor)
	if err != nil {
		return nil, err
	}

	return matcher, nil
}

type matcherArrayParser struct{}

func (p *matcherArrayParser) Name() string {
	return "array"
}

func (p *matcherArrayParser) ParseMatcher(ep ExpectationParser, o *fastjson.Object) (expectations.Matcher, error) {
	expectValue := o.Get("expect")
	if expectValue == nil {
		return nil, xerrors.New("missing expect")
	}

	expectArray, err := expectValue.Array()
	if err != nil {
		return nil, xerrors.Errorf("expect: %w", err)
	}

	matcher := &expectations.MatcherArray{Matchers: make([]expectations.Matcher, 0, len(expectArray))}
	for _, v := range expectArray {
		o, err = v.Object()
		if err != nil {
			return nil, xerrors.Errorf("expect: %w", err)
		}

		m, err := ep(o)
		if err != nil {
			return nil, err
		}

		matcher.Matchers = append(matcher.Matchers, m)
	}

	return matcher, nil
}

type matcherContainsParser struct{}

func (p *matcherContainsParser) Name() string {
	return "contains"
}

func (p *matcherContainsParser) ParseMatcher(ep ExpectationParser, o *fastjson.Object) (expectations.Matcher, error) {
	requestValue := o.Get("request")
	if requestValue == nil {
		return nil, xerrors.New("missing request")
	}

	requestString, err := requestValue.StringBytes()
	if err != nil {
		return nil, xerrors.Errorf("request: %w", err)
	}

	var expected map[string]interface{}
	if err = json.Unmarshal(requestString, &expected); err != nil {
		return nil, xerrors.Errorf("unmarshal expected %s: %w", requestString, err)
	}

	expectValue := o.Get("expect")
	if expectValue == nil {
		return nil, xerrors.New("missing expect")
	}

	expectObject, err := expectValue.Object()
	if err != nil {
		return nil, xerrors.Errorf("expect: %w", err)
	}

	m, err := ep(expectObject)
	if err != nil {
		return nil, err
	}

	matcher := &expectations.MatcherContains{Expected: expected, Matcher: m}
	return matcher, nil
}
