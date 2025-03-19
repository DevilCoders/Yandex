package parsers

import (
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ResponderParser interface {
	// Name of responder used in configuration.
	Name() string
	// ParseResponder from configuration.
	ParseResponder(o *fastjson.Object) (expectations.Responder, error)
}

type responderExactParser struct{}

func (p *responderExactParser) Name() string {
	return "exact"
}

func (p *responderExactParser) ParseResponder(o *fastjson.Object) (expectations.Responder, error) {
	respValue := o.Get("response")
	if respValue != nil {
		respBytes, err := respValue.StringBytes()
		if err != nil {
			return nil, xerrors.Errorf("response: %w", err)
		}

		if o.Get("code") != nil || o.Get("error_msg") != nil {
			return nil, xerrors.New("exact responder: can't have both response and code/error message")
		}

		return &expectations.ResponderExact{Response: string(respBytes)}, nil
	}

	codeValue := o.Get("code")
	if codeValue == nil {
		return nil, xerrors.New("exact responder: must have either response or code")
	}

	codeBytes, err := codeValue.StringBytes()
	if err != nil {
		return nil, xerrors.Errorf("code bytes: %w", err)
	}

	code, err := grpcutil.ParseStatusCode(string(codeBytes))
	if err != nil {
		return nil, err
	}

	resp := expectations.ResponderExact{Code: code}
	if errorMsgValue := o.Get("error_msg"); errorMsgValue != nil {
		errorMsgBytes, err := errorMsgValue.StringBytes()
		if err != nil {
			return nil, xerrors.Errorf("error_msg bytes: %w", err)
		}

		resp.ErrorMsg = string(errorMsgBytes)
	}

	return &resp, nil
}

type ResponderCodeParser struct {
	codeResponders map[string]expectations.Responder
}

func NewResponderCodeParser() *ResponderCodeParser {
	return &ResponderCodeParser{codeResponders: map[string]expectations.Responder{}}
}

func (p *ResponderCodeParser) Name() string {
	return "code"
}

type CodeResponder interface {
	expectations.Responder

	FuncName() string
}

func (p *ResponderCodeParser) RegisterCodeResponder(cr CodeResponder) error {
	if _, ok := p.codeResponders[cr.FuncName()]; ok {
		return xerrors.Errorf("code responder %q is already registered", cr.FuncName())
	}

	p.codeResponders[cr.FuncName()] = cr
	return nil
}

func (p *ResponderCodeParser) ParseResponder(o *fastjson.Object) (expectations.Responder, error) {
	fn, err := o.Get("func").StringBytes()
	if err != nil {
		return nil, xerrors.Errorf("func: %w", err)
	}

	r, ok := p.codeResponders[string(fn)]
	if !ok {
		return nil, xerrors.Errorf("code responder %q is not registered", string(fn))
	}

	return r, nil
}
