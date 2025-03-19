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
package server

import (
	"fmt"
	"io"
	"net/http"
	"regexp"
	"strings"
	"sync"
	"time"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/server/rewriters"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/library/go/core/log"
)

var serviceByPortDefaults = map[string]string{
	"9870":  "hdfs",
	"9864":  "webhdfs",
	"10002": "hive",
}

type pendingRequest struct {
	req            *http.Request
	respChan       chan *http.Response
	errStatusChan  chan int
	serviceHeaders map[string]string
}

func newPendingRequest(r *http.Request, serviceHeaders map[string]string) *pendingRequest {
	return &pendingRequest{
		req:            r,
		respChan:       make(chan *http.Response),
		errStatusChan:  make(chan int, 1),
		serviceHeaders: serviceHeaders,
	}
}

// CSPConfig contains Content Security Policy settings
type CSPConfig struct {
	Enabled   bool   `json:"enabled" yaml:"enabled"`
	ReportURI string `json:"report_uri" yaml:"report_uri"`
	Block     bool   `json:"block" yaml:"block"`
}

type Config struct {
	CSP             CSPConfig         `json:"csp" yaml:"csp"`
	DataplaneDomain string            `json:"dataplane_domain" yaml:"dataplane_domain"`
	UIProxyDomain   string            `json:"ui_proxy_domain" yaml:"ui_proxy_domain"`
	ServiceByPort   map[string]string `json:"service_by_port" yaml:"service_by_port"`
}

func DefaultConfig() Config {
	return Config{}
}

type Server struct {
	config Config
	logger log.Logger

	agentHandlers     map[string]*agentHandler
	agentHandlersLock sync.RWMutex
}

func NewProxy(config Config, logger log.Logger) *Server {
	return &Server{
		agentHandlers: make(map[string]*agentHandler),
		config:        config,
		logger:        logger,
	}
}

func (s *Server) getAgentHandler(agentID string) *agentHandler {
	s.agentHandlersLock.RLock()
	agent, ok := s.agentHandlers[agentID]
	s.agentHandlersLock.RUnlock()
	if ok {
		return agent
	}

	s.agentHandlersLock.Lock()
	defer s.agentHandlersLock.Unlock()
	if agent, ok := s.agentHandlers[agentID]; ok {
		return agent
	}
	agent = &agentHandler{
		agentID:    agentID,
		requestIDs: make(chan string),
		logger:     s.logger,
	}
	s.agentHandlers[agentID] = agent
	return agent
}

func (s *Server) handleAgentRequest(w http.ResponseWriter, r *http.Request) {
	agentID := r.Header.Get(common.HeaderAgentID)
	if agentID == "" {
		http.Error(w, "AgentID header is missing", http.StatusBadRequest)
		return
	}
	s.getAgentHandler(agentID).handleRequest(w, r)
}

func (s *Server) AgentRequestHandler() http.Handler {
	return http.HandlerFunc(s.handleAgentRequest)
}

func (s *Server) AuthenticatedUserRequestHandler() http.Handler {
	return http.HandlerFunc(s.handleUserRequest)
}

// The reasons we handle OPTIONS requests in UI Proxy and no sending them upstream are as follows:
// * we need to handle such requests without authentication because cookies are not sent
// * we want to apply some additional security checks for Host and Origin (that they belong to the same cluster)
// * we need to return some custom headers:
//   Access-Control-Allow-Credentials=true - this allows to send cookies with cross domain ajax requests
//   Access-Control-Allow-Origin=value-of-origin - because value "*" is not allowed
//   when Access-Control-Allow-Credentials==true
func (s *Server) handleOptionsRequest(w http.ResponseWriter, r *http.Request) bool {
	if r.Method != "OPTIONS" {
		return false
	}

	host := r.Header.Get("X-Forwarded-Host")
	if host == "" {
		host = r.Host
	}
	parts := strings.Split(host, ".")
	hostname := parts[0]
	domain := strings.Join(parts[1:], ".")
	withoutKnox := strings.HasPrefix(hostname, "ui-")

	if !withoutKnox {
		return false
	}

	origin := r.Header.Get("Origin")
	submatch := regexp.MustCompile(`(?:https?://)?(.+?)\.(.+)(:\d+)?`).FindStringSubmatch(origin)
	if len(submatch) < 3 {
		s.logger.Warnf("Blocking CORS due to invalid Origin format: %s", origin)
		w.WriteHeader(http.StatusOK)
		return true
	}

	parts = strings.Split(hostname, "-")
	clusterID := ""
	if len(parts) > 1 {
		clusterID = parts[1]
	}
	originHostname := submatch[1]
	if !strings.HasPrefix(originHostname, fmt.Sprintf("ui-%s-", clusterID)) {
		s.logger.Warnf("Blocking CORS: Origin (%s) belongs to the cluster distinct"+
			" from request Host's one (%s)", origin, host)
		w.WriteHeader(http.StatusOK)
		return true
	}

	if submatch[2] != domain {
		s.logger.Warnf("Blocking CORS: Origin's (%s) domain differs from request Host's (%s) one",
			origin, host)
		w.WriteHeader(http.StatusOK)
		return true
	}

	w.Header().Set("Access-Control-Allow-Origin", origin)
	w.Header().Set("Access-Control-Allow-Credentials", "true")
	w.Header().Set("Access-Control-Allow-Headers", "Accept")
	w.Header().Set("Access-Control-Allow-Methods", r.Header.Get("Access-Control-Request-Method"))
	w.Header().Set("Access-Control-Max-Age", "1728000")
	w.WriteHeader(http.StatusOK)
	return true
}

func (s *Server) UserRequestHandler(authMiddleware util.HTTPMiddleware) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if s.handleOptionsRequest(w, r) {
			return
		}

		if authMiddleware != nil {
			authMiddleware(s.AuthenticatedUserRequestHandler()).ServeHTTP(w, r)
		} else {
			s.AuthenticatedUserRequestHandler().ServeHTTP(w, r)
		}
	})
}

func (s *Server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if agentID := r.Header.Get(common.HeaderAgentID); agentID != "" {
		s.handleAgentRequest(w, r)
		return
	}
	s.handleUserRequest(w, r)
}

func (s *Server) handleUserRequest(w http.ResponseWriter, r *http.Request) {
	uuidObject, err := uuid.NewV4()
	if err != nil {
		s.logger.Errorf("Failed to generate request id: %s", err)
		http.Error(w, "Internal Server Error", http.StatusInternalServerError)
		return
	}

	id := uuidObject.String()
	logger := log.With(s.logger, log.String("RequestID", id))
	for _, h := range common.HopHeaders {
		r.Header.Del(h)
	}
	r.Header.Set("X-Forwarded-Proto", "https")
	host := r.Header.Get("X-Forwarded-Host")
	if host == "" {
		host = r.Host
	} else {
		parts := strings.Split(host, ":")
		if len(parts) == 1 {
			r.Header.Set("X-Forwarded-Port", "443")
		}
	}

	hostname := strings.Split(host, ".")[0]
	withoutKnox := strings.HasPrefix(hostname, "ui-")

	agentID := util.ClusterIDFromContext(r.Context())
	if agentID == "" {
		logger.Error("No agent id is set within request context")
		http.Error(w, "Internal Server Error", http.StatusInternalServerError)
		return
	}

	upstreamHost := ""
	var rewriter rewriters.Rewriter = nil
	if withoutKnox {
		// when Knox is not used format of hostname is ui-${CLUSTER_ID}-${DATAPROC_NODE_NAME}-${PORT},
		// eg. ui-c9qrg9vdufi196kilf73-rc1a-dataproc-m-r1r2y2hs233j2tl1-8088
		parts := strings.Split(hostname, "-")
		cnt := len(parts)
		if cnt < 4 {
			logger.Warnf("Failed to extract dataproc node hostname and target port from request host %q", host)
			http.Error(w, "Unexpected host name", http.StatusBadRequest)
			return
		}
		port := parts[cnt-1]
		dataprocNodeName := strings.Join(parts[2:cnt-1], "-")
		upstreamHost = fmt.Sprintf("%s.%s:%s", dataprocNodeName, s.config.DataplaneDomain, port)
		rewriter = rewriters.Build(r, agentID, s.serviceByPort(port), s.config.DataplaneDomain, s.config.UIProxyDomain,
			s.logger)
	}

	logger.Infof("Received %s %s%s request", r.Method, upstreamHost, r.URL.Path)

	agent := s.getAgentHandler(agentID)
	startTime := time.Now()
	serviceHeaders := map[string]string{
		common.HeaderRequestStartTime: startTime.Format(time.RFC3339Nano),
		common.HeaderUpstreamHost:     upstreamHost,
	}
	pending := newPendingRequest(r, serviceHeaders)
	agent.pendingRequests.Store(id, pending)
	defer agent.pendingRequests.Delete(id)

	// Enqueue the request
	select {
	case <-r.Context().Done():
		// The client request was cancelled
		logger.Warn("Timeout waiting to enqueue the request ID")
		return
	case agent.requestIDs <- id:
	}
	logger.Debugf("Request enqueued after %s", time.Since(startTime))

	// Pull out and copy the response
	select {
	case <-r.Context().Done():
		// The client request was cancelled
		logger.Warn("Timeout waiting for the response")
	case status := <-pending.errStatusChan:
		http.Error(w, http.StatusText(status), status)
	case resp := <-pending.respChan:
		defer func() {
			err := resp.Body.Close()
			if err != nil {
				logger.Errorf("Failed to close response body: %s", err)
			}
		}()
		logger.Debugf("Response %s received after %s", resp.Status, time.Since(startTime))
		// Copy all of the non-hop-by-hop headers to the proxied response
		for _, h := range common.HopHeaders {
			resp.Header.Del(h)
		}
		for name, vals := range resp.Header {
			w.Header()[name] = vals
		}

		body := io.Reader(resp.Body)
		if rewriter != nil {
			body, err = rewriter.Rewrite(w.Header(), resp.Body)
			if err != nil {
				logger.Errorf("Failed to rewrite response: %s", err)
				http.Error(w, "Internal Server Error", http.StatusInternalServerError)
				return
			}
		}
		if rewriter == nil || rewriter.ApplyCSP() {
			s.applyCSP(w)
		}
		w.WriteHeader(resp.StatusCode)
		_, err := io.Copy(w, body)
		if err != nil {
			logger.Errorf("Failed to copy response body: %s", err)
		}
	}
	logger.Debug("Done processing request")
}

func (s *Server) serviceByPort(port string) string {
	if service, ok := s.config.ServiceByPort[port]; ok {
		return service
	}
	return serviceByPortDefaults[port]
}

func (s *Server) applyCSP(w http.ResponseWriter) {
	if s.config.CSP.Enabled {
		policy := "default-src 'self'; " +
			"script-src 'self' 'unsafe-inline' 'unsafe-eval'; " +
			"style-src 'self' 'unsafe-inline'; " +
			"img-src 'self' data:; " +
			"font-src 'self' data:"
		if s.config.CSP.ReportURI != "" {
			policy = policy + "; report-uri " + s.config.CSP.ReportURI
		}
		headerName := "Content-Security-Policy-Report-Only"
		if s.config.CSP.Block {
			headerName = "Content-Security-Policy"
		}
		w.Header().Set(headerName, policy)
	}
}
