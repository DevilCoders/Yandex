package invertingproxy

import (
	"bytes"
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/agent"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/server"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	testAgentID = "default"
	testTimeout = time.Second
	// When writing to http.ResponseWriter golang's http stack buffers data
	// before sending it on the wire. This is the size of that buffer.
	// We send data in chunks of such size when we need to bypass buffering.
	httpBufferSize = 16384
	logInTests     = false

	testWebsocketShimPath = "/ws"
)

var (
	testTLSCert   []byte
	testTLSConfig *tls.Config
	chunkA        []byte
	chunkB        []byte
	doubleChunkA  []byte
	doubleChunkB  []byte

	testProxyHTTPClient *http.Client
)

func init() {
	cert, tlsConfig, err := common.GenerateSelfSignedCert([]string{})
	if err != nil {
		panic(fmt.Sprintf("failed to generate self signed cert for tests: %s", err))
	}
	testTLSCert = cert
	testTLSConfig = tlsConfig

	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(testTLSCert)
	testProxyHTTPClient = &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{
				RootCAs: caCertPool,
			},
		},
	}

	chunkA = makeChunk('A', httpBufferSize)
	chunkB = makeChunk('B', httpBufferSize)
	doubleChunkA = makeChunk('A', 2*httpBufferSize)
	doubleChunkB = makeChunk('B', 2*httpBufferSize)
}

func makeChunk(char byte, size int) []byte {
	buf := make([]byte, size)
	for i := 0; i < size; i++ {
		buf[i] = char
	}
	return buf
}

type testControl struct {
	t               *testing.T
	ctx             context.Context
	logger          log.Logger
	proxyServerAddr net.Addr
	proxyServer     *server.Server
	proxyAgent      *agent.Agent
	stopAgent       context.CancelFunc
	backendServer   *httptest.Server
}

func (tc *testControl) wait(ch <-chan struct{}) {
	select {
	case <-tc.ctx.Done():
		tc.t.Fatal(tc.ctx.Err().Error())
	case <-ch:
	}
}

func (tc *testControl) waitError(ch chan error) error {
	select {
	case <-tc.ctx.Done():
		tc.t.Fatal(tc.ctx.Err().Error())
	case err := <-ch:
		return err
	}
	return nil
}

func (tc *testControl) waitString(ch chan string) string {
	select {
	case <-tc.ctx.Done():
		tc.t.Fatal(tc.ctx.Err().Error())
	case str := <-ch:
		return str
	}
	return ""
}

func setupTest(t *testing.T, handler http.HandlerFunc) *testControl {
	ctx, cancel := context.WithTimeout(context.Background(), testTimeout)
	t.Cleanup(cancel)

	var logger log.Logger = &nop.Logger{}
	if logInTests {
		zapConfig := zap.KVConfig(log.DebugLevel)
		zapConfig.OutputPaths = []string{"stdout"}
		var err error
		logger, err = zap.New(zapConfig)
		require.NoError(t, err)
	}

	listener, err := net.Listen("tcp", "127.0.0.1:0")
	require.NoError(t, err)
	go func() {
		<-ctx.Done()
		err = listener.Close()
		require.NoError(t, err)
	}()

	ipServer := server.NewProxy(server.DefaultConfig(), logger)
	mux := http.ServeMux{}
	mux.Handle("/", http.HandlerFunc(func(writer http.ResponseWriter, request *http.Request) {
		request = request.WithContext(util.WithClusterID(request.Context(), testAgentID))
		ipServer.AuthenticatedUserRequestHandler().ServeHTTP(writer, request)
	}))
	mux.Handle("/agent/", ipServer.AgentRequestHandler())

	httpServer := &http.Server{
		Handler:   &mux,
		TLSConfig: testTLSConfig,
	}

	go func() {
		_ = httpServer.ServeTLS(listener, "", "")
	}()

	upstream := httptest.NewServer(handler)
	t.Cleanup(upstream.Close)

	config := agent.Config{
		ProxyServerURL:     "https://" + listener.Addr().String() + "/",
		ProxyServerTimeout: 60 * time.Second,
		CAFile:             createTempFile(t, testTLSCert),
		UpstreamURL:        upstream.URL,
		AgentID:            testAgentID,
		ForwardUserID:      false,
		WebsocketShimPath:  testWebsocketShimPath,
	}
	proxyAgent, err := agent.New(config, nil, logger, nil)
	require.NoError(t, err)
	agentCtx, stopAgent := context.WithCancel(ctx)
	t.Cleanup(stopAgent)
	go proxyAgent.Run(agentCtx)

	return &testControl{
		t:               t,
		ctx:             ctx,
		logger:          logger,
		proxyServerAddr: listener.Addr(),
		proxyServer:     ipServer,
		proxyAgent:      proxyAgent,
		stopAgent:       stopAgent,
		backendServer:   upstream,
	}
}

func createTempFile(t *testing.T, content []byte) string {
	tmpfile, err := ioutil.TempFile("", "*.crt")
	require.NoError(t, err)

	t.Cleanup(func() {
		_ = os.Remove(tmpfile.Name())
	})

	_, err = tmpfile.Write(content)
	require.NoError(t, err)
	err = tmpfile.Close()
	require.NoError(t, err)

	return tmpfile.Name()
}

type requestControl struct {
	writer           *io.PipeWriter
	responseBody     chan string
	proxyError       chan error
	cancelRequest    context.CancelFunc
	responseReceived chan struct{}
	response         *http.Response
}

func newRequestControl(t *testing.T, control *testControl) *requestControl {
	reader, writer := io.Pipe()
	endpoint := fmt.Sprintf("https://%s/some/path", control.proxyServerAddr.String())
	req, err := http.NewRequest("POST", endpoint, reader)
	require.NoError(t, err)
	ctx, cancelRequest := context.WithCancel(control.ctx)
	req = req.WithContext(ctx)

	rc := &requestControl{
		writer:           writer,
		responseBody:     make(chan string, 1),
		proxyError:       make(chan error, 1),
		cancelRequest:    cancelRequest,
		responseReceived: make(chan struct{}, 1),
	}
	go rc.run(control, req)

	return rc
}

func (rc *requestControl) run(control *testControl, req *http.Request) {
	response, err := testProxyHTTPClient.Do(req)
	if err != nil {
		rc.proxyError <- err
		return
	}

	rc.response = response
	rc.responseReceived <- struct{}{}

	body, err := ioutil.ReadAll(response.Body)
	if err != nil {
		control.logger.Errorf("Error while reading response body: %s", err)
		return
	}
	rc.responseBody <- string(body)
}

func simpleEchoServer(t *testing.T) (http.HandlerFunc, chan struct{}) {
	requestReceived := make(chan struct{}, 10)
	handler := func(w http.ResponseWriter, r *http.Request) {
		requestReceived <- struct{}{}
		buffer := bytes.Buffer{}
		_, err := buffer.ReadFrom(r.Body)
		require.NoError(t, err)
		// echo full request body back
		_, err = io.WriteString(w, buffer.String())
		require.NoError(t, err)
	}
	return handler, requestReceived
}

type upstreamControl struct {
	writer           *io.PipeWriter
	handler          http.HandlerFunc
	requestReceived  chan struct{}
	requestRead      chan struct{}
	requestReadError chan error
}

func newUpstreamControl(t *testing.T) *upstreamControl {
	reader, writer := io.Pipe()

	uc := &upstreamControl{
		writer:           writer,
		requestReceived:  make(chan struct{}, 1),
		requestRead:      make(chan struct{}, 1),
		requestReadError: make(chan error, 1),
	}

	uc.handler = func(w http.ResponseWriter, r *http.Request) {
		uc.requestReceived <- struct{}{}
		buffer := bytes.Buffer{}
		_, err := buffer.ReadFrom(r.Body)
		if err != nil {
			uc.requestReadError <- err
			return
		}
		uc.requestRead <- struct{}{}

		// Once request is cancelled finish writing to http.ResponseWriter.
		// This is required to stop test http server.
		go func() {
			<-r.Context().Done()
			_ = writer.Close()
		}()

		_, err = io.Copy(w, reader)
		require.NoError(t, err)
	}

	return uc
}

func TestBasicScenario(t *testing.T) {
	handler := func(w http.ResponseWriter, r *http.Request) {
		_, err := io.WriteString(w, "ok")
		require.NoError(t, err)
	}

	control := setupTest(t, handler)

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)
	req = req.WithContext(util.WithClusterID(req.Context(), testAgentID))
	rr := httptest.NewRecorder()
	control.proxyServer.ServeHTTP(rr, req)
	require.Equal(t, "ok", rr.Body.String())
}

func TestMultipleRequestsSupport(t *testing.T) {
	// In this test we are sending 2 concurrent requests and
	// check that their associated request and response data is not intermixed.
	handler, requestReceived := simpleEchoServer(t)
	control := setupTest(t, handler)

	req1 := newRequestControl(t, control)
	req2 := newRequestControl(t, control)

	_, err := req1.writer.Write(chunkA)
	require.NoError(t, err)

	_, err = req2.writer.Write(chunkB)
	require.NoError(t, err)

	control.wait(requestReceived)
	control.wait(requestReceived)

	_, err = req1.writer.Write(chunkA)
	require.NoError(t, err)
	err = req1.writer.Close() // finish sending request
	require.NoError(t, err)

	require.Equal(t, string(doubleChunkA), control.waitString(req1.responseBody))

	_, err = req2.writer.Write(chunkB)
	require.NoError(t, err)
	err = req2.writer.Close() // finish sending request
	require.NoError(t, err)

	require.Equal(t, string(doubleChunkB), control.waitString(req2.responseBody))
}

func TestClientCancelsRequestDuringProxyingResponse(t *testing.T) {
	upstream := newUpstreamControl(t)
	control := setupTest(t, upstream.handler)
	req := newRequestControl(t, control)

	_, err := req.writer.Write(chunkA)
	require.NoError(t, err)
	err = req.writer.Close()
	require.NoError(t, err)

	control.wait(upstream.requestRead)
	_, err = upstream.writer.Write(chunkB)
	require.NoError(t, err)
	control.wait(req.responseReceived)
	require.Equal(t, 200, req.response.StatusCode)
	req.cancelRequest()
}

func TestStopAgent(t *testing.T) {
	upstream := newUpstreamControl(t)
	control := setupTest(t, upstream.handler)
	req := newRequestControl(t, control)

	_, err := req.writer.Write(chunkA)
	require.NoError(t, err)

	control.wait(upstream.requestReceived)
	control.stopAgent()

	// send some more data in order to detect cancelled agent request
	for i := 0; i < 5; i++ {
		_, err = req.writer.Write(chunkA)
		require.NoError(t, err)
	}
	// it is not possible to receive response until we finish sending request body
	err = req.writer.Close()
	require.NoError(t, err)

	control.wait(req.responseReceived)
	require.Equal(t, http.StatusServiceUnavailable, req.response.StatusCode)
}
