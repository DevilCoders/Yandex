package expectations

import (
	"context"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type RespondInputs struct {
	FullMethod        string
	Service           string
	Method            string
	ProtoRequest      proto.Message
	StructuredRequest map[string]interface{}
	NewResponse       func(service, method string) (proto.Message, error)
}

// Responder interface must be implemented by any expectations responder.
type Responder interface {
	Respond(ctx context.Context, inputs RespondInputs) (interface{}, error)
}

// ResponderExact returns the exact response without any modifications.
type ResponderExact struct {
	Response string
	Code     codes.Code
	ErrorMsg string
}

func (r *ResponderExact) Respond(_ context.Context, inputs RespondInputs) (interface{}, error) {
	var resp proto.Message
	if r.Response != "" {
		var err error
		resp, err = inputs.NewResponse(inputs.Service, inputs.Method)
		if err != nil {
			return nil, err
		}

		if err := jsonpb.UnmarshalString(r.Response, resp); err != nil {
			return nil, xerrors.Errorf("unmarshal response: %w", err)
		}

		return resp, nil
	}

	return nil, status.New(r.Code, r.ErrorMsg).Err()
}
