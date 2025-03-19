package parsers

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"
	"github.com/valyala/fastjson"
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
)

func TestResponderExactParser(t *testing.T) {
	inputs := []struct {
		Name     string
		Data     string
		Expected *expectations.ResponderExact
		Error    bool
	}{
		{
			Name:  "Empty",
			Data:  `{}`,
			Error: true,
		},
		{
			Name:     "Response",
			Data:     `{"response": "{\"foo\": \"bar\"}"}`,
			Expected: &expectations.ResponderExact{Response: `{"foo": "bar"}`},
		},
		{
			Name:     "Error",
			Data:     `{"code": "NOT_FOUND", "error_msg": "foobar"}`,
			Expected: &expectations.ResponderExact{Code: codes.NotFound, ErrorMsg: "foobar"},
		},
		{
			Name:  "Both",
			Data:  `{"response": "{\"foo\": \"bar\"}", "code": "NOT_FOUND", "error_msg": "foobar"}`,
			Error: true,
		},
	}

	var parser responderExactParser
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			v, err := fastjson.Parse(input.Data)
			require.NoError(t, err)
			o, err := v.Object()
			require.NoError(t, err)

			r, err := parser.ParseResponder(o)
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

type testCodeResponder struct {
	someHardcodedValue string
}

var _ CodeResponder = &testCodeResponder{}

func (r *testCodeResponder) FuncName() string {
	return "test"
}

func (r *testCodeResponder) Respond(ctx context.Context, inputs expectations.RespondInputs) (interface{}, error) {
	return nil, nil
}

func TestResponderCodeParser(t *testing.T) {
	t.Run("Registered", func(t *testing.T) {
		v, err := fastjson.Parse(`{"func": "test", "somehardcodedvalue": "bar"}`)
		require.NoError(t, err)
		o, err := v.Object()
		require.NoError(t, err)

		p := NewResponderCodeParser()
		expected := &testCodeResponder{someHardcodedValue: "bar"}
		require.NoError(t, p.RegisterCodeResponder(expected))
		r, err := p.ParseResponder(o)
		require.NoError(t, err)
		require.Equal(t, expected, r)
	})

	t.Run("NotRegistered", func(t *testing.T) {
		v, err := fastjson.Parse(`{"func": "test"}`)
		require.NoError(t, err)
		o, err := v.Object()
		require.NoError(t, err)

		p := NewResponderCodeParser()
		r, err := p.ParseResponder(o)
		require.Error(t, err)
		require.Nil(t, r)
	})

	t.Run("DoubleRegister", func(t *testing.T) {
		p := NewResponderCodeParser()
		require.NoError(t, p.RegisterCodeResponder(&testCodeResponder{}))
		require.Error(t, p.RegisterCodeResponder(&testCodeResponder{}))
	})
}
