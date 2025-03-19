package expectations

import (
	"context"
	"reflect"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// ErrNoMatch is used when no expectations were found for a given request.
var ErrNoMatch = xerrors.NewSentinel("no match found")

type MatchInputs struct {
	FullMethod        string
	Service           string
	Method            string
	ProtoRequest      proto.Message
	StructuredRequest map[string]interface{}
}

// Matcher interface must be implemented by any expectations matcher.
type Matcher interface {
	Match(ctx context.Context, inputs MatchInputs) (Responder, error)
}

// MatcherResponder is a special type of matcher that always matches and always returns a responder.
type MatcherResponder struct {
	Responder Responder
}

func (m *MatcherResponder) Match(_ context.Context, _ MatchInputs) (Responder, error) {
	return m.Responder, nil
}

// MatcherArray holds an array of matchers. Matchers are iterated over in specified order until one of them matches or
// throws an unrecoverable error.
type MatcherArray struct {
	Matchers []Matcher
}

func (m *MatcherArray) Match(ctx context.Context, inputs MatchInputs) (Responder, error) {
	for _, matcher := range m.Matchers {
		r, err := matcher.Match(ctx, inputs)
		if err == nil {
			return r, nil
		}

		if !xerrors.Is(err, ErrNoMatch) {
			return nil, err
		}
	}

	return nil, ErrNoMatch.WithStackTrace()
}

// MatcherFullMethod matches fully-qualified gRPC method address.
type MatcherFullMethod struct {
	Matchers map[string]Matcher
}

func (m *MatcherFullMethod) Match(ctx context.Context, inputs MatchInputs) (Responder, error) {
	matcher, ok := m.Matchers[inputs.FullMethod]
	if !ok {
		return nil, ErrNoMatch.WithStackTrace()
	}

	return matcher.Match(ctx, inputs)
}

// MatcherContains matches if request contains expected values. Ignores values not listed as expected.
//
// Important: ignores only top-level unexpected values, second-level values must match exactly.
type MatcherContains struct {
	Expected map[string]interface{}
	Matcher  Matcher
}

func (m *MatcherContains) Match(ctx context.Context, inputs MatchInputs) (Responder, error) {
	for k, v := range m.Expected {
		actual, ok := inputs.StructuredRequest[k]
		if !ok {
			return nil, ErrNoMatch.Wrap(xerrors.Errorf("missing key %q", k))
		}

		if !reflect.DeepEqual(v, actual) {
			return nil, ErrNoMatch.Wrap(xerrors.Errorf("invalid value of key %q", k))
		}
	}

	return m.Matcher.Match(ctx, inputs)
}
