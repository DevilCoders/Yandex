package invertingproxy

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
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

func TestBypassKnox(t *testing.T) {
	// Step 0. Prepare
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

	testClusterID := "xxx"
	uiProxyConfig := server.Config{
		DataplaneDomain: "dataplane",
		UIProxyDomain:   "dataproc-ui",
	}
	node1 := "rc1a-dataproc-m-yyy"
	node2 := "rc1a-dataproc-d-zzz"

	// Step 1. Setup demo upstream, that responds with a link to another Data Proc node
	yarnUIHandler := func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html")
		_, err := io.WriteString(w, fmt.Sprintf("Go to http://%s.dataplane:8888/some/path", node2))
		require.NoError(t, err)
	}
	yarnUI := httptest.NewServer(http.HandlerFunc(yarnUIHandler))
	t.Cleanup(yarnUI.Close)
	parts := strings.Split(yarnUI.URL, ":")
	yarnUIPort := parts[len(parts)-1]

	// Step 2. Setup UI Proxy
	// 2.1. setup handler to work without user authentication
	ipServer := server.NewProxy(uiProxyConfig, logger)
	mux := http.ServeMux{}
	mux.Handle("/", http.HandlerFunc(func(writer http.ResponseWriter, request *http.Request) {
		request = request.WithContext(util.WithClusterID(request.Context(), testClusterID))
		ipServer.AuthenticatedUserRequestHandler().ServeHTTP(writer, request)
	}))
	mux.Handle("/agent/", ipServer.AgentRequestHandler())

	// 2.2. create listener
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	require.NoError(t, err)
	go func() {
		<-ctx.Done()
		err = listener.Close()
		require.NoError(t, err)
	}()
	parts = strings.Split(listener.Addr().String(), ":")
	uiProxyServerPort := parts[len(parts)-1]

	// 2.3. create tls certificate and start http server
	uiProxyClusterDomain := fmt.Sprintf("ui-%s-%s-%s.%s", testClusterID, node1, yarnUIPort, uiProxyConfig.DataplaneDomain)
	uiProxyTLSCert, uiProxyTLSConfig, err := common.GenerateSelfSignedCert([]string{uiProxyClusterDomain})
	httpServer := &http.Server{
		Handler:   &mux,
		TLSConfig: uiProxyTLSConfig,
	}
	go func() {
		_ = httpServer.ServeTLS(listener, "", "")
	}()

	// Step 3. Configure and start ui proxy agent
	config := agent.Config{
		ProxyServerURL:     "https://" + listener.Addr().String() + "/",
		ProxyServerTimeout: 60 * time.Second,
		CAFile:             createTempFile(t, uiProxyTLSCert),
		UpstreamURL:        "",
		AgentID:            testClusterID,
		ForwardUserID:      false,
		WebsocketShimPath:  testWebsocketShimPath,
	}
	// setup transport that resolves our custom DNS names to 127.0.0.1
	defaultTransport := http.DefaultTransport.(*http.Transport)
	upstreamTransport := defaultTransport.Clone()
	upstreamTransport.DialContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
		parts := strings.Split(addr, ":")
		parts[0] = "127.0.0.1"
		addr = strings.Join(parts, ":")
		return defaultTransport.DialContext(ctx, network, addr)
	}
	proxyAgent, err := agent.New(config, nil, logger, upstreamTransport)
	require.NoError(t, err)
	agentCtx, stopAgent := context.WithCancel(ctx)
	t.Cleanup(stopAgent)
	go proxyAgent.Run(agentCtx)

	// Step 4. Setup client (eg. browser)
	myTransport := defaultTransport.Clone()
	// setup transport that resolves our custom DNS names to 127.0.0.1
	myTransport.DialContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
		parts := strings.Split(addr, ":")
		parts[0] = "127.0.0.1"
		addr = strings.Join(parts, ":")
		return defaultTransport.DialContext(ctx, network, addr)
	}
	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(uiProxyTLSCert)
	myTransport.TLSClientConfig = &tls.Config{
		RootCAs: caCertPool,
	}
	client := &http.Client{Transport: myTransport}
	uiProxyClusterURL := fmt.Sprintf("https://%s:%s", uiProxyClusterDomain, uiProxyServerPort)

	// Step 5. Test: send request and check response.
	req, err := http.NewRequest(http.MethodGet, uiProxyClusterURL+"/some/path", nil)
	require.NoError(t, err)
	resp, err := client.Do(req)
	require.NoError(t, err)
	require.Equal(t, http.StatusOK, resp.StatusCode)
	body, err := ioutil.ReadAll(resp.Body)
	require.NoError(t, err)
	expected := fmt.Sprintf("Go to https://ui-%s-%s-8888.dataproc-ui/some/path", testClusterID, node2)
	require.Equal(t, expected, string(body))
}
