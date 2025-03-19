package asmock

import (
	"context"
	"fmt"
	"net"
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"

	grpc_metadata "google.golang.org/grpc/metadata"

	"github.com/stretchr/testify/mock"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

type MockAccessService struct {
	mock.Mock
}

type Options struct {
	LogFunc func(format string, args ...interface{}) // Optional function for logging
}

func (s *MockAccessService) Authenticate(
	ctx context.Context,
	req *servicecontrol.AuthenticateRequest,
) (*servicecontrol.AuthenticateResponse, error) {
	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.AuthenticateResponse)
	return res, args.Error(1)
}

func (s *MockAccessService) Authorize(
	ctx context.Context,
	req *servicecontrol.AuthorizeRequest,
) (*servicecontrol.AuthorizeResponse, error) {
	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.AuthorizeResponse)
	return res, args.Error(1)
}

func (s *MockAccessService) BulkAuthorize(
	ctx context.Context,
	req *servicecontrol.BulkAuthorizeRequest,
) (*servicecontrol.BulkAuthorizeResponse, error) {
	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*servicecontrol.BulkAuthorizeResponse)
	return res, args.Error(1)
}

// NewMockAccessService create mock of grpc Access Service
// mock - testify mock object for create your behaviour
// address - return connection address of server (host:port) - for use in grpc connection
// cancel - cancel func. It must be called after end of usage mock - for stop server listener
func NewMockAccessService(opts *Options) (mock *MockAccessService, address string, cancel func(), err error) {
	if opts == nil {
		opts = &Options{}
	}

	fail := func(innerErr error) (mock *MockAccessService, address string, cancel func(), err error) {
		messErr := fmt.Sprintf("NewMockAccessService create error: %v", innerErr)
		opts.Logf(messErr)
		return nil, "", nil, fmt.Errorf(messErr)
	}

	service := &MockAccessService{}
	server := grpc.NewServer()
	servicecontrol.RegisterAccessServiceServer(server, service)
	listener, err := net.Listen("tcp", "localhost:0")
	if err != nil {
		return fail(err)
	}
	port, err := getPort(listener)
	if err != nil {
		return fail(err)
	}
	address = "localhost:" + port
	opts.Logf("Start access service mock listener on: '%v'", address)

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		err := server.Serve(listener)
		if err != nil && err != grpc.ErrServerStopped {
			opts.Logf("Error serving grpc: %v", err)
		}
	}()
	cancel = func() {
		server.Stop()
		wg.Wait()
	}

	return service, address, cancel, nil
}

func (o *Options) Logf(format string, args ...interface{}) {
	if o.LogFunc != nil {
		o.LogFunc(format, args...)
	}
}

func ContainsGRPCMetadataStrings(
	t *testing.T,
	headersKV ...string, // pairs of key, value
) func(context.Context) bool {
	headers := make(map[string]string)
	for i := 0; i < len(headersKV); i += 2 {
		headers[headersKV[i]] = headersKV[i+1]
	}

	return func(ctx context.Context) bool {
		md, ok := grpc_metadata.FromIncomingContext(ctx)
		if !ok {
			return assert.ElementsMatch(t, map[string]string{}, headers)
		}
		for k, v := range headers {
			mdValue := md[k]
			ok = assert.Equal(t, []string{v}, mdValue) && ok
		}
		return ok
	}
}

func getPort(l net.Listener) (string, error) {
	_, portStr, err := net.SplitHostPort(l.Addr().String())
	if err != nil {
		return "", fmt.Errorf(
			"failed to parse port out of addr %v: %w",
			l.Addr().String(),
			err,
		)
	}
	return portStr, nil
}
