package protob

import (
	"encoding/json"

	"github.com/golang/protobuf/ptypes/any"
	"github.com/golang/protobuf/ptypes/empty"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var EmptyPB = &empty.Empty{}

var EmptyAny = func() *anypb.Any {
	return MustBeAny(EmptyPB)
}()

func JSONFromAnyMessage(any *anypb.Any) (json.RawMessage, error) {
	return json.Marshal(any)
}

func JSONFromProtoMessage(m proto.Message) (json.RawMessage, error) {
	return json.Marshal(
		MustBeAny(m),
	)
}

func AnyMessageFromJSON(raw json.RawMessage) (*anypb.Any, error) {
	any := &anypb.Any{}
	if err := json.Unmarshal(raw, any); err != nil {
		return nil, err
	}

	return any, nil
}

func MustBeAny(m proto.Message) *anypb.Any {
	any, err := anypb.New(m)
	if err != nil {
		panic(xerrors.Errorf("failed to convert to any type: %s", err))
	}

	return any
}

func MustBeInt64Any(i int64) *any.Any {
	return MustBeAny(&wrapperspb.Int64Value{Value: i})
}
