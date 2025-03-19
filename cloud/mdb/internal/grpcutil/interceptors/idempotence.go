package interceptors

import (
	"context"
	"crypto/sha256"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	headerIdempotenceID = "idempotency-key"
)

func withIdempotenceID(ctx context.Context, id string) context.Context {
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		md = metadata.MD{}
	} else {
		md = md.Copy()
	}

	md.Set(headerIdempotenceID, id)
	return metadata.NewOutgoingContext(ctx, md)
}

func idempotenceIDFromGRPC(ctx context.Context) (string, bool) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", false
	}

	ik := md.Get(headerIdempotenceID)
	if len(ik) != 1 {
		return "", false
	}

	return ik[0], true
}

// newUnaryServerInterceptorIdempotence returns interceptor that retrieves idempotence id from incoming gRPC request, hashes request and adds results to the context
func newUnaryServerInterceptorIdempotence(l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		// Retrieve idempotence id header
		id, ok := idempotenceIDFromGRPC(ctx)
		if ok {
			if err := idempotence.Validate(id); err != nil {
				return nil, semerr.InvalidInputf("idempotence id in header %q is invalid: %s", headerIdempotenceID, err)
			}

			// Add transport-agnostic idempotence to context
			idemp := idempotence.Incoming{ID: id}
			hash, err := hashRequest(req)
			if err != nil {
				ctxlog.Warn(ctx, l, "failed to hash gRPC request for idempotence", log.Error(err))
			} else {
				idemp.Hash = hash
			}

			ctx = idempotence.WithIncoming(ctx, idemp)
			ctx = idempotence.WithLogField(ctx, idemp.ID)
		}

		return handler(ctx, req)
	}
}

func hashRequest(req interface{}) ([]byte, error) {
	pb, ok := req.(proto.Message)
	if !ok {
		return nil, xerrors.New("request is not proto.Message")
	}

	m := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}
	marshaled, err := m.MarshalToString(pb)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal request: %w", err)
	}

	hash := sha256.Sum256([]byte(marshaled))
	return hash[:], nil
}

// newUnaryClientInterceptorIdempotence returns interceptor that retrieves idempotence id from context and adds it to outgoing gRPC request
func newUnaryClientInterceptorIdempotence() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, conn *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		// Retrieve idempotence id from context
		id, ok := idempotence.OutgoingFromContext(ctx)
		if ok {
			// Add idempotence id to headers
			ctx = withIdempotenceID(ctx, id)
		}

		return invoker(ctx, method, req, reply, conn, opts...)
	}
}

// TODO: stream interceptors
