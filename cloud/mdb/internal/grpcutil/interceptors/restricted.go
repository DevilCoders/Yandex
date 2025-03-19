package interceptors

import (
	"context"
	"strings"
	"time"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type Rule struct {
	Package string    `json:"package" yaml:"package"`
	Service string    `json:"service" yaml:"service"`
	Method  string    `json:"method" yaml:"method"`
	Since   time.Time `json:"since" yaml:"since"`
	Until   time.Time `json:"until" yaml:"until"`
}

type RestrictedCheckerConfig struct {
	Rules []Rule `json:"rules" yaml:"rules"`
}

func parseGRPCFullMethod(fullMethod string) (pkg, svc, method string) {
	i := strings.LastIndex(fullMethod, "/")
	pkgservice := fullMethod[1:i]
	method = fullMethod[i+1:]
	j := strings.LastIndex(pkgservice, ".")
	pkg = pkgservice[:j]
	svc = pkgservice[j+1:]
	return
}

func checkMethodRestricted(fullMethod string, now time.Time, config RestrictedCheckerConfig) bool {
	if len(config.Rules) == 0 {
		return false
	}
	pkg, svc, method := parseGRPCFullMethod(fullMethod)
	for _, r := range config.Rules {
		if (r.Package == "" || r.Package == pkg) && (r.Service == "" || r.Service == svc) && (r.Method == "" || r.Method == method) {
			if (!r.Since.IsZero() && now.Before(r.Since)) || (!r.Until.IsZero() && now.After(r.Until)) {
				return true
			}
		}
	}
	return false
}

// NewUnaryServerInterceptorRestricted returns interceptor that checks method is available
func NewUnaryServerInterceptorRestricted(config RestrictedCheckerConfig) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if checkMethodRestricted(info.FullMethod, time.Now(), config) {
			return nil, semerr.NotImplementedf("Method %v is restricted", info.FullMethod)
		}
		return handler(ctx, req)
	}
}

// NewStreamServerInterceptorRestricted returns interceptor that checks method is available
func NewStreamServerInterceptorRestricted(config RestrictedCheckerConfig) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if checkMethodRestricted(info.FullMethod, time.Now(), config) {
			return semerr.NotImplementedf("Method %v is restricted", info.FullMethod)
		}
		return handler(srv, ss)
	}
}
