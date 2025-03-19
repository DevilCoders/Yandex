/*
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// initially taken from https://github.com/google/inverting-proxy
package agent

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httputil"
	"strings"
	"sync"
	"time"

	lru "github.com/hashicorp/golang-lru"
	"golang.org/x/net/http2"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/agent/websockets"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	requestCacheLimit = 1000

	// maxPollInterval is maximum backoff interval when polling for new incoming requests
	maxPollInterval = 30 * time.Second

	// PendingPath is the URL subpath for pending requests held by the proxy.
	PendingPath = "agent/pending"

	// RequestPath is the URL subpath for reading a specific request held by the proxy.
	RequestPath = "agent/request"

	// ResponsePath is the URL subpath for posting a request response to the proxy.
	ResponsePath = "agent/response"

	// HeaderUserID is the name of a response header used by the proxy to identify the end user.
	HeaderUserID = "X-Inverting-Proxy-User-ID"
)

type Config struct {
	// URL (including scheme) of the inverting proxy
	ProxyServerURL string `json:"proxy_server_url" yaml:"proxy_server_url"`

	// Client timeout when sending requests to the inverting proxy.
	// Currently this timeout applies to "list pending requests" request only,
	// as receiving user request and sending upstream response might be
	// quite long-running (think of uploading/downloading large file).
	ProxyServerTimeout time.Duration `json:"proxy_server_timeout" yaml:"proxy_server_timeout"`

	// Path to certificates file
	CAFile string `json:"ca_file" yaml:"ca_file"`

	// URL (like http://ip:port/) of the upstream server
	UpstreamURL string `json:"upstream_url" yaml:"upstream_url"`

	// Unique ID for this agent.
	AgentID string `json:"agent_id" yaml:"agent_id"`

	// Whether or not to include the ID (email address) of the end user in requests to the backend
	ForwardUserID bool `json:"forward_user_id" yaml:"forward_user_id"`

	// If this is set to eg. "/ws" then following will be done:
	// * agent will modify every upstream html response and insert special javascript code into it.
	//   This JS code will substitute native browser's websocket API (window.WebSocket) with custom one,
	//   that behaves as described below.
	// * "open websocket connection" api call is transformed to http post call to /ws/open
	// * "send message" api call is transformed to http post call to /ws/data
	// * receiving of messages is implemented using long poll request to /ws/poll
	// * on receiving /ws/open request agent opens websocket connection to upstream server.
	//   Websocket server path is passed within body of /ws/open request
	// * agent handles other "/ws/..." requests by sending and receiving messages from those websocket
	//   connections
	WebsocketShimPath string `json:"websocket_shim_path" yaml:"websocket_shim_path"`
}

type Authenticator = func(http.RoundTripper) http.RoundTripper

type Agent struct {
	config               Config
	httpClient           *http.Client
	upstream             http.Handler
	logger               log.Logger
	requests             *lru.Cache
	upstreamHandlers     map[string]http.Handler
	upstreamHandlersLock sync.RWMutex
	upstreamTransport    http.RoundTripper
}

func New(config Config, authenticator Authenticator, logger log.Logger, upstreamTransport http.RoundTripper) (*Agent, error) {
	if config.ProxyServerURL == "" {
		return nil, xerrors.Errorf("You must specify the address of the proxy")
	}
	if config.AgentID == "" {
		return nil, xerrors.Errorf("You must specify an agent ID")
	}

	transport := &http2.Transport{}
	if config.CAFile != "" {
		cafile, err := ioutil.ReadFile(config.CAFile)
		if err != nil {
			return nil, xerrors.Errorf("failed to read certificates from %q: %w", config.CAFile, err)
		}

		caCertPool := x509.NewCertPool()
		caCertPool.AppendCertsFromPEM(cafile)
		transport.TLSClientConfig = &tls.Config{
			MinVersion: tls.VersionTLS12,
			RootCAs:    caCertPool,
		}
	}
	var roundTripper http.RoundTripper = transport
	if authenticator != nil {
		roundTripper = authenticator(roundTripper)
	}
	client := &http.Client{
		Transport: roundTripper,
	}

	requests, err := lru.New(requestCacheLimit)
	if err != nil {
		return nil, xerrors.Errorf("failed to create requests lru cache: %w", err)
	}

	if upstreamTransport == nil {
		upstreamTransport = http.DefaultTransport
	}

	return &Agent{
		config:            config,
		httpClient:        client,
		logger:            logger,
		requests:          requests,
		upstreamHandlers:  map[string]http.Handler{},
		upstreamTransport: upstreamTransport,
	}, nil
}

func (agent *Agent) Run(ctx context.Context) {
	agent.pollForNewRequests(ctx)
}

type stdLogWrapper struct {
	logger log.Logger
}

func (w *stdLogWrapper) Write(p []byte) (n int, err error) {
	size := len(p)
	if size > 0 && p[size-1] == '\n' {
		p = p[:size-1]
	}
	w.logger.Error(string(p))
	return len(p), nil
}

// pollForNewRequests repeatedly reaches out to the proxy server to ask if any pending are available, and then
// processes any newly-seen ones.
func (agent *Agent) pollForNewRequests(ctx context.Context) {
	var requests []string
	retryConfig := retry.Config{
		MaxInterval: maxPollInterval,
	}
	for {
		err := retry.New(retryConfig).RetryWithLog(
			ctx,
			func() error {
				rr, err := agent.listPendingRequests(ctx)
				if err != nil {
					return err
				}
				requests = rr
				return nil
			},
			"read pending requests",
			agent.logger,
		)

		if err != nil {
			return
		}

		for _, requestID := range requests {
			if _, ok := agent.requests.Get(requestID); !ok {
				agent.requests.Add(requestID, requestID)
				rp := newRequestProcessor(agent, requestID)
				go rp.process(ctx)
			}
		}
	}
}

// listPendingRequests issues a single request to the proxy to ask for the IDs of pending requests.
func (agent *Agent) listPendingRequests(ctx context.Context) ([]string, error) {
	proxyURL := agent.config.ProxyServerURL + PendingPath
	requestCtx, cancel := context.WithTimeout(ctx, agent.config.ProxyServerTimeout)
	defer cancel()
	proxyReq, err := http.NewRequestWithContext(requestCtx, http.MethodGet, proxyURL, nil)
	if err != nil {
		return nil, err
	}
	proxyReq.Header.Add(common.HeaderAgentID, agent.config.AgentID)
	proxyResp, err := agent.httpClient.Do(proxyReq)
	if err != nil {
		return nil, xerrors.Errorf("proxy request failed: %w", err)
	}
	defer func() {
		err = proxyResp.Body.Close()
		if err != nil {
			agent.logger.Errorf("Failed to close response body: %s", err)
		}
	}()
	return parseRequestIDs(proxyResp)
}

// parseRequestIDs takes a response from the proxy and parses any forwarded request IDs out of it.
func parseRequestIDs(response *http.Response) ([]string, error) {
	responseBody := &io.LimitedReader{
		R: response.Body,
		// If a response is larger than 1MB, then truncate it. This will result in an
		// failure to parse the result, but that is better than a potential OOM.
		//
		// Note that this shouldn't happen anyway, since a reasonable proxy server
		// should limit the size of a response to less than this. For instance, the
		// initial version of our proxy will never return a list of more than 100
		// request IDs.
		N: 1024 * 1024,
	}
	responseBytes, err := ioutil.ReadAll(responseBody)
	if err != nil {
		return nil, xerrors.Errorf("failed to read response body: %w", err)
	}
	if response.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("unexpected response: %d, %q", response.StatusCode, responseBytes)
	}
	if len(responseBytes) <= 0 {
		return []string{}, nil
	}

	var requests []string
	if err := json.Unmarshal(responseBytes, &requests); err != nil {
		return nil, xerrors.Errorf("failed to parse response body: %w", err)
	}
	return requests, nil
}

func (agent *Agent) getUpstreamHandler(ctx context.Context, upstreamHost string) (http.Handler, error) {
	if upstreamHost == "" {
		knoxHost := strings.ReplaceAll(agent.config.UpstreamURL, "http://", "")
		if knoxHost != "" {
			return agent.getUpstreamHandler(ctx, knoxHost)
		}
		return nil, xerrors.Errorf("failed to proxy request to Knox because UpstreamURL is not configured")
	}

	agent.upstreamHandlersLock.RLock()
	handler, ok := agent.upstreamHandlers[upstreamHost]
	agent.upstreamHandlersLock.RUnlock()
	if ok {
		return handler, nil
	}

	agent.upstreamHandlersLock.Lock()
	defer agent.upstreamHandlersLock.Unlock()

	if handler, ok := agent.upstreamHandlers[upstreamHost]; ok {
		return handler, nil
	}

	director := func(req *http.Request) {
		req.URL.Scheme = "http"
		req.URL.Host = upstreamHost
		if _, ok := req.Header["User-Agent"]; !ok {
			// explicitly disable User-Agent so it's not set to default value
			req.Header.Set("User-Agent", "")
		}

		ctxlog.Infof(req.Context(), agent.logger, "Sending %s %s request to %s", req.Method, req.URL.Path, upstreamHost)
	}
	errorHandler := func(rw http.ResponseWriter, request *http.Request, err error) {
		ctxlog.Errorf(request.Context(), agent.logger, "Error while sending request to upstream: %s", err)
		rw.WriteHeader(http.StatusBadGateway)
		_, _ = rw.Write([]byte(err.Error()))
	}
	upstream := &httputil.ReverseProxy{
		Director:     director,
		Transport:    agent.upstreamTransport,
		ErrorHandler: errorHandler,
	}
	handler = upstream

	if agent.config.WebsocketShimPath != "" {
		adapter, err := websockets.NewAdapter(upstream, upstreamHost, agent.config.WebsocketShimPath, agent.logger)
		if err != nil {
			return nil, err
		}
		adapter.Run(ctx)
		handler = adapter.Handler()
	}

	agent.upstreamHandlers[upstreamHost] = handler
	return handler, nil
}
