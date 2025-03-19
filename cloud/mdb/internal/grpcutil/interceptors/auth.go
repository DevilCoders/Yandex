package interceptors

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/auth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type MethodChecker func(string) bool

// authFromGRPC retrieves auth token from gRPC context and adds it to the same context in a standard format
func authFromGRPC(tokenModel grpcauth.GRPCAuthTokenModel, ctx context.Context) (context.Context, error) {
	token, err := tokenModel.ParseGRPCAuthToken(ctx)
	if err != nil {
		return nil, err
	}
	return auth.WithAuthToken(ctx, token), nil
}

// NewUnaryServerInterceptorAuth returns interceptor that retrieves auth from incoming gRPC request and adds it context
func NewUnaryServerInterceptorAuth(tokenModel grpcauth.GRPCAuthTokenModel, methodChecker MethodChecker) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if !methodChecker(info.FullMethod) {
			return handler(ctx, req)
		}

		need := true
		checker, ok := info.Server.(AuthNeedChecker)
		if ok {
			need = checker.NeedAuth(info.FullMethod)
		}

		if need {
			var err error
			ctx, err = authFromGRPC(tokenModel, ctx)
			if err != nil {
				return nil, semerr.WrapWithAuthentication(err, "invalid authentication header")
			}
		}

		return handler(ctx, req)
	}
}

// NewStreamServerInterceptorAuth returns interceptor that retrieves auth from incoming gRPC request and adds it context
func NewStreamServerInterceptorAuth(tokenModel grpcauth.GRPCAuthTokenModel, methodChecker MethodChecker) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if !methodChecker(info.FullMethod) {
			return handler(srv, ss)
		}

		ctx := ss.Context()
		need := true
		checker, ok := srv.(AuthNeedChecker)
		if ok {
			need = checker.NeedAuth(info.FullMethod)
		}

		if need {
			var err error
			ctx, err = authFromGRPC(tokenModel, ctx)
			if err != nil {
				return semerr.WrapWithAuthentication(err, "invalid authentication header")
			}
		}

		return handler(srv, NewServerStreamWithContext(ss, ctx))
	}
}

// AuthNeedChecker must be implemented by gRPC service that does not want auth checks for all of its methods
type AuthNeedChecker interface {
	// NeedAuth must return  true if method requires auth
	NeedAuth(method string) bool
}

// NeedAuth implements auth policy for select methods
type NeedAuth struct {
	// NoAuth holds methods that do not require auth
	NoAuth map[string]struct{}
}

// NeedAuth returns false if method is blacklisted, otherwise true
func (na *NeedAuth) NeedAuth(method string) bool {
	_, ok := na.NoAuth[method]
	return !ok
}

// NoAuth implements noauth policy (no method requires auth)
type NoAuth struct{}

// NeedAuth always returns false
func (na *NoAuth) NeedAuth(_ string) bool {
	return false
}
