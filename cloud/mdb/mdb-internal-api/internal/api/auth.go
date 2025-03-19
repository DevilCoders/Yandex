package api

import (
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
)

var tokenModel grpcauth.GRPCAuthTokenModel = iamauth.NewIAMAuthTokenModel()

// NewAuthHeader returns authentication data in a format suitable for gRPC metadata
func NewAuthHeader(token string) map[string]string {
	return tokenModel.NewAuthHeader(token)
}
func NewUnaryServerInterceptorAuth(methodChecker interceptors.MethodChecker) grpc.UnaryServerInterceptor {
	return interceptors.NewUnaryServerInterceptorAuth(tokenModel, methodChecker)
}

func NewStreamServerInterceptorAuth(methodChecker interceptors.MethodChecker) grpc.StreamServerInterceptor {
	return interceptors.NewStreamServerInterceptorAuth(tokenModel, methodChecker)
}
