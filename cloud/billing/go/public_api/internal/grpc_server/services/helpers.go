package services

import (
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"
)

func convertToAny(v interface{}) (*anypb.Any, error) {
	message, ok := v.(proto.Message)
	if !ok {
		return nil, errInternal
	}
	return anypb.New(message)
}
