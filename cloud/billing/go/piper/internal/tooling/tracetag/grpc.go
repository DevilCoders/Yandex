package tracetag

import (
	"github.com/opentracing/opentracing-go"
	"google.golang.org/grpc/codes"
)

func GRPCError(code codes.Code) opentracing.Tag {
	return opentracing.Tag{Key: "grpc.response.code", Value: int(code)}
}
