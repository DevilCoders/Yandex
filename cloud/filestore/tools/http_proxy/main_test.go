package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	grpc_metadata "google.golang.org/grpc/metadata"

	filestore_config "a.yandex-team.ru/cloud/filestore/config"
	filestore_grpc "a.yandex-team.ru/cloud/filestore/public/api/grpc"
	filestore_protos "a.yandex-team.ru/cloud/filestore/public/api/protos"
	"a.yandex-team.ru/library/go/test/yatest"
)

////////////////////////////////////////////////////////////////////////////////

const (
	maxRetryCount   = 5
	backoffDuration = time.Second
)

////////////////////////////////////////////////////////////////////////////////

func runRequestWithRetries(
	c *http.Client,
	req *http.Request,
) (*http.Response, error) {

	runRequest := func() (*http.Response, error) {
		resp, err := c.Do(req)
		if err != nil {
			return nil, err
		}

		if resp.StatusCode != 200 {
			err := resp.Body.Close()
			if err != nil {
				log.Fatalf("failed to close response body: %v", err)
			}
			return nil, fmt.Errorf("%v", resp.Status)
		}

		return resp, nil
	}

	for i := 1; i < maxRetryCount; i++ {
		resp, err := runRequest()
		if err == nil {
			return resp, nil
		}

		<-time.After(backoffDuration * time.Duration(i))
	}

	return runRequest()
}

////////////////////////////////////////////////////////////////////////////////

func getCertFilePath() string {
	return yatest.SourcePath("cloud/filestore/tests/certs/server.crt")
}

func getCertPrivateKeyFilePath() string {
	return yatest.SourcePath("cloud/filestore/tests/certs/server.key")
}

////////////////////////////////////////////////////////////////////////////////

func getPort(addr string) (uint32, error) {
	_, portStr, err := net.SplitHostPort(addr)
	if err != nil {
		return 0, fmt.Errorf(
			"failed to parse port out of addr %v: %w",
			addr,
			err,
		)
	}

	port, err := strconv.Atoi(portStr)
	if err != nil {
		return 0, fmt.Errorf(
			"failed to parse port '%v' as an int: %w",
			portStr,
			err,
		)
	}

	return uint32(port), nil
}

////////////////////////////////////////////////////////////////////////////////

type mockFilestoreServer struct {
	mock.Mock
}

func (s *mockFilestoreServer) StartEndpoint(
	ctx context.Context,
	req *filestore_protos.TStartEndpointRequest,
) (*filestore_protos.TStartEndpointResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*filestore_protos.TStartEndpointResponse)
	return res, args.Error(1)
}

func (s *mockFilestoreServer) StopEndpoint(
	ctx context.Context,
	req *filestore_protos.TStopEndpointRequest,
) (*filestore_protos.TStopEndpointResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*filestore_protos.TStopEndpointResponse)
	return res, args.Error(1)
}

func (s *mockFilestoreServer) ListEndpoints(
	ctx context.Context,
	req *filestore_protos.TListEndpointsRequest,
) (*filestore_protos.TListEndpointsResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*filestore_protos.TListEndpointsResponse)
	return res, args.Error(1)
}

func (s *mockFilestoreServer) KickEndpoint(
	ctx context.Context,
	req *filestore_protos.TKickEndpointRequest,
) (*filestore_protos.TKickEndpointResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*filestore_protos.TKickEndpointResponse)
	return res, args.Error(1)
}

func (s *mockFilestoreServer) Ping(
	ctx context.Context,
	req *filestore_protos.TPingRequest,
) (*filestore_protos.TPingResponse, error) {

	args := s.Called(ctx, req)
	res, _ := args.Get(0).(*filestore_protos.TPingResponse)
	return res, args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func runMockFilestoreServer() (*mockFilestoreServer, uint32, func(), error) {

	service := &mockFilestoreServer{}

	var err error

	server := grpc.NewServer()
	filestore_grpc.RegisterTEndpointManagerServiceServer(server, service)

	lis, err := net.Listen("tcp", ":0")
	if err != nil {
		return nil, 0, nil, fmt.Errorf("failed to listen: %w", err)
	}

	port, err := getPort(lis.Addr().String())
	if err != nil {
		return nil, 0, nil, err
	}

	log.Printf("Running mockFilestoreServer on port %v", port)

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		err := server.Serve(lis)
		if err != nil {
			log.Fatalf("Error serving grpc: %v", err)
		}
	}()

	return service, port, func() {
		server.Stop()
		wg.Wait()
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

type httpProxyClient struct {
	client *http.Client
	addr   string
}

func createInsecureHTTPProxyClient(port uint32) (*httpProxyClient, error) {
	return &httpProxyClient{
		client: &http.Client{},
		addr:   fmt.Sprintf("http://localhost:%v", port),
	}, nil
}

func (c *httpProxyClient) run(
	path string,
	body string,
	headers map[string]string,
) (string, error) {

	req, err := http.NewRequest(
		"POST",
		fmt.Sprintf("%v/%v", c.addr, path),
		strings.NewReader(body),
	)
	if err != nil {
		return "", fmt.Errorf("failed to create request: %w", err)
	}

	for k, v := range headers {
		req.Header.Add(k, v)
	}

	resp, err := runRequestWithRetries(c.client, req)
	if err != nil {
		return "", fmt.Errorf("failed to run request: %v", err)
	}
	defer resp.Body.Close()

	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", fmt.Errorf("failed to read response body: %w", err)
	}

	return string(respBody), nil
}

////////////////////////////////////////////////////////////////////////////////

func runHTTPProxyServer(
	nfsPort uint32,
) (*httpProxyClient, func(), error) {

	host := new(string)
	*host = "localhost"
	config := &filestore_config.THttpProxyConfig{
		Port:         new(uint32),
		NfsVhostHost: host,
		NfsVhostPort: &nfsPort,
	}
	mux, err := createProxyMux(context.Background(), config)
	if err != nil {
		return nil, nil, err
	}

	insecureServer, err := runInsecureServer(mux, config)
	if err != nil {
		return nil, nil, err
	}

	insecurePort, err := getPort(insecureServer.addr)
	if err != nil {
		return nil, nil, err
	}

	insecureClient, err := createInsecureHTTPProxyClient(insecurePort)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to create insecure HTTP client: %w", err)
	}

	return insecureClient, func() {
		insecureServer.kill()
		insecureServer.wait()
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func runServers() (*mockFilestoreServer, *httpProxyClient, func(), error) {

	srv, nfsPort, closeFilestoreServer, err := runMockFilestoreServer()
	if err != nil {
		return nil, nil, nil, err
	}

	insecureClient, closeHTTPProxyServer, err := runHTTPProxyServer(nfsPort)
	if err != nil {
		closeFilestoreServer()
		return nil, nil, nil, err
	}

	return srv, insecureClient, func() {
		closeHTTPProxyServer()
		closeFilestoreServer()
	}, nil
}

type runServersType func() (*mockFilestoreServer, *httpProxyClient, func(), error)

////////////////////////////////////////////////////////////////////////////////

func containsHeaders(
	t *testing.T,
	headers map[string]string,
) func(context.Context) bool {

	return func(ctx context.Context) bool {
		md, ok := grpc_metadata.FromIncomingContext(ctx)
		if !ok {
			return assert.ElementsMatch(t, map[string]string{}, headers)
		}
		for k, v := range headers {
			ok = assert.Equal(t, []string{v}, md[k]) && ok
		}

		return ok
	}
}

////////////////////////////////////////////////////////////////////////////////

func testPing(t *testing.T, run runServersType) {
	srv, client, closeServers, err := run()
	require.NoError(t, err)
	defer closeServers()

	srv.On(
		"Ping",
		mock.MatchedBy(containsHeaders(t, map[string]string{})),
		&filestore_protos.TPingRequest{},
	).Times(1).Return(&filestore_protos.TPingResponse{}, nil)

	body, err := client.run("ping", "", map[string]string{})
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, body, "{}")
}

func testListEndpointsWithBody(t *testing.T, run runServersType) {
	srv, client, closeServers, err := run()
	require.NoError(t, err)
	defer closeServers()

	srv.On(
		"ListEndpoints",
		mock.MatchedBy(containsHeaders(t, map[string]string{})),
		&filestore_protos.TListEndpointsRequest{},
	).Times(1).Return(&filestore_protos.TListEndpointsResponse{
		Endpoints: []*filestore_protos.TEndpointConfig{{
			FileSystemId: "file_system_id",
		}},
	}, nil)

	body, err := client.run("list_endpoints", "", map[string]string{})
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, body, "{\"Endpoints\":[{\"FileSystemId\":\"file_system_id\"}]}")
}

func testHeaders(t *testing.T, run runServersType) {
	srv, client, closeServers, err := run()
	require.NoError(t, err)
	defer closeServers()

	srv.On(
		"Ping",
		mock.MatchedBy(containsHeaders(t, map[string]string{
			"authorization":   "Bearer AUTH_TOKEN",
			"x-nfs-client-id": "SOME_CLIENT",
		})),
		&filestore_protos.TPingRequest{},
	).Times(1).Return(&filestore_protos.TPingResponse{}, nil)

	body, err := client.run("ping", "", map[string]string{
		"authorization":   "Bearer AUTH_TOKEN",
		"x-nfs-client-id": "SOME_CLIENT",
	})
	mock.AssertExpectationsForObjects(t, srv)
	assert.NoError(t, err)
	assert.Equal(t, body, "{}")
}

////////////////////////////////////////////////////////////////////////////////

func runInsecureNfsInsecureProxy() (*mockFilestoreServer, *httpProxyClient, func(), error) {
	srv, client, closeServers, err := runServers()
	return srv, client, closeServers, err
}

func TestPingInsecureNfsInsecureProxy(t *testing.T) {
	testPing(t, runInsecureNfsInsecureProxy)
}

func TestListEndpointsWithBodyInsecureNfsInsecureProxy(t *testing.T) {
	testListEndpointsWithBody(t, runInsecureNfsInsecureProxy)
}

func TestHeadersInsecureNfsInsecureProxy(t *testing.T) {
	testHeaders(t, runInsecureNfsInsecureProxy)
}
