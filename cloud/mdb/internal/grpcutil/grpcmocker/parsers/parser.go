package parsers

import (
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// NewEmptyParser returns empty parser.
func NewEmptyParser() *Parser {
	return &Parser{
		matchers:   map[string]MatcherParser{},
		responders: map[string]ResponderParser{},
	}
}

// NewDefaultParser returns parser with default matchers and responders.
func NewDefaultParser() *Parser {
	p := NewEmptyParser()

	if err := p.RegisterMatcherParser(&matcherFullMethodParser{}); err != nil {
		panic(err)
	}

	if err := p.RegisterMatcherParser(&matcherArrayParser{}); err != nil {
		panic(err)
	}

	if err := p.RegisterMatcherParser(&matcherContainsParser{}); err != nil {
		panic(err)
	}

	if err := p.RegisterResponderParser(&responderExactParser{}); err != nil {
		panic(err)
	}

	return p
}

// Parser produces expectations from configuration based on registered parsers.
type Parser struct {
	matchers   map[string]MatcherParser
	responders map[string]ResponderParser
}

// ExpectationParser is passed to matcher parsers for handling subexpectations.
type ExpectationParser func(o *fastjson.Object) (expectations.Matcher, error)

// RegisterMatcherParser adds new matcher parser.
func (p *Parser) RegisterMatcherParser(mp MatcherParser) error {
	if _, ok := p.matchers[mp.Name()]; ok {
		return xerrors.Errorf("matcher parser %q is already registered", mp.Name())
	}

	p.matchers[mp.Name()] = mp
	return nil
}

// RegisterResponderParser adds new responder parser.
func (p *Parser) RegisterResponderParser(rp ResponderParser) error {
	if _, ok := p.responders[rp.Name()]; ok {
		return xerrors.Errorf("responder parser %q is already registered", rp.Name())
	}

	p.responders[rp.Name()] = rp
	return nil
}

// Parse configuration and produce expectations.
func (p *Parser) Parse(input []byte) (expectations.Matcher, error) {
	v, err := fastjson.ParseBytes(input)
	if err != nil {
		return nil, xerrors.Errorf("parse: %w", err)
	}

	o, err := v.Object()
	if err != nil {
		return nil, xerrors.Errorf("root: %w", err)
	}

	o, err = o.Get("expectations").Object()
	if err != nil {
		return nil, xerrors.Errorf("root expectations: %w", err)
	}

	return p.parseExpectation(o)
}

func (p *Parser) parseExpectation(o *fastjson.Object) (expectations.Matcher, error) {
	typeValue := o.Get("type")
	if typeValue == nil {
		return nil, xerrors.Errorf("missing type")
	}

	typeBytes, err := typeValue.StringBytes()
	if err != nil {
		return nil, xerrors.Errorf("expectation type: %w", err)
	}

	typeString := string(typeBytes)

	nameValue := o.Get("name")
	if nameValue == nil {
		return nil, xerrors.Errorf("missing name")
	}

	nameBytes, err := nameValue.StringBytes()
	if err != nil {
		return nil, xerrors.Errorf("expectation name: %w", err)
	}

	nameString := string(nameBytes)

	switch typeString {
	case "matcher":
		parser, ok := p.matchers[nameString]
		if !ok {
			return nil, xerrors.Errorf("no parser for %q matcher", nameString)
		}

		return parser.ParseMatcher(p.parseExpectation, o)
	case "responder":
		parser, ok := p.responders[nameString]
		if !ok {
			return nil, xerrors.Errorf("no parser for %q responder", nameString)
		}

		responder, err := parser.ParseResponder(o)
		if err != nil {
			return nil, err
		}

		return &expectations.MatcherResponder{Responder: responder}, nil
	default:
		return nil, xerrors.Errorf("unknown expectation type %q", string(typeBytes))
	}
}
