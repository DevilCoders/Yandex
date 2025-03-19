package interceptors

import (
	"context"
	"strings"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

var DefaultReadOnlyMethods = []string{"get", "list", "check"}

type ReadOnlyChecker interface {
	ReadOnlyEnabled() bool
	Applicable(method string) bool
	MethodReadOnly(method string) bool
}

type readOnlyChecker struct {
	FileWatcher fs.FileWatcher
	Prefix      string
	Methods     []string
}

var _ ReadOnlyChecker = &readOnlyChecker{}

func NewReadOnlyChecker(fw fs.FileWatcher, prefix string, methods []string) ReadOnlyChecker {
	return &readOnlyChecker{FileWatcher: fw, Prefix: prefix, Methods: methods}
}

func (c readOnlyChecker) Applicable(method string) bool {
	return strings.HasPrefix(method, c.Prefix)
}

func (c readOnlyChecker) ReadOnlyEnabled() bool {
	return c.FileWatcher.Exists()
}

func (c readOnlyChecker) MethodReadOnly(method string) bool {
	return MethodReadOnly(method, c.Methods)
}

func MethodReadOnly(method string, readOnlyMethods []string) bool {
	method = strings.ToLower(method)
	for _, s := range readOnlyMethods {
		if strings.HasPrefix(method, s) {
			return true
		}
	}

	return false
}

func isReadOnly(method string, checker ReadOnlyChecker) error {
	// TODO: make read-only optional
	if checker == nil {
		return nil
	}

	if !checker.Applicable(method) {
		return nil
	}

	i := strings.LastIndex(method, "/") + 1
	method = method[i:]

	if !checker.MethodReadOnly(method) && checker.ReadOnlyEnabled() {
		return semerr.FailedPrecondition("read-only mode")
	}

	return nil
}

// newUnaryServerInterceptorReadOnly returns interceptor that checks method name and blocks writes in read-only mode
func newUnaryServerInterceptorReadOnly(checker ReadOnlyChecker) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		if err := isReadOnly(info.FullMethod, checker); err != nil {
			return nil, err
		}

		return handler(ctx, req)
	}
}

// newStreamServerInterceptorReadOnly returns interceptor that checks method name and blocks writes in read-only mode
func newStreamServerInterceptorReadOnly(checker ReadOnlyChecker) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if err := isReadOnly(info.FullMethod, checker); err != nil {
			return err
		}

		return handler(srv, ss)
	}
}

type nopReadOnlyChecker struct{}

func NewNopReadOnlyChecker() ReadOnlyChecker {
	return &nopReadOnlyChecker{}
}

func (c nopReadOnlyChecker) Applicable(string) bool {
	return false
}

func (c nopReadOnlyChecker) ReadOnlyEnabled() bool {
	return false
}

func (c nopReadOnlyChecker) MethodReadOnly(string) bool {
	return false
}
