package server

import (
	"crypto/tls"
	"math/rand"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/agentauth"
	proxyserver "a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/server"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/userauth"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/library/go/core/log"
)

type HTTPServer struct {
	Config      Config
	agentAuth   *agentauth.AgentAuth
	userAuth    *userauth.UserAuth
	proxyServer *proxyserver.Server
	logger      log.Logger
}

type TLSConfig struct {
	Cert string `json:"cert" yaml:"cert"`
	Key  string `json:"key" yaml:"key"`
}

type Config struct {
	ListenAddr string    `json:"listen_addr" yaml:"listen_addr"`
	TLS        TLSConfig `json:"tls" yaml:"tls"`
}

func New(
	config Config,
	agentAuth *agentauth.AgentAuth,
	userAuth *userauth.UserAuth,
	proxyServer *proxyserver.Server,
	logger log.Logger,
) *HTTPServer {
	return &HTTPServer{
		Config:      config,
		agentAuth:   agentAuth,
		userAuth:    userAuth,
		proxyServer: proxyServer,
		logger:      logger,
	}
}

func healthHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write([]byte("{\"status\": \"OK\"}"))
}

func (server *HTTPServer) Run() {
	rand.Seed(time.Now().UnixNano())
	srv := http.ServeMux{}

	srv.Handle("/ping", http.HandlerFunc(healthHandler))
	srv.Handle("/health", http.HandlerFunc(healthHandler))

	var authMiddleware util.HTTPMiddleware = nil
	if server.userAuth != nil {
		server.userAuth.Install(&srv)
		authMiddleware = server.userAuth.AuthMiddleware
	}
	handler := server.proxyServer.UserRequestHandler(authMiddleware)
	srv.Handle("/", handler)

	handler = server.proxyServer.AgentRequestHandler()
	if server.agentAuth != nil {
		handler = server.agentAuth.Middleware(handler)
	}
	srv.Handle("/agent/", handler)

	tlsConfig := tls.Config{}
	tlsConfig.MinVersion = tls.VersionTLS12
	tlsConfig.Certificates = make([]tls.Certificate, 1)
	cert, err := tls.X509KeyPair([]byte(server.Config.TLS.Cert), []byte(server.Config.TLS.Key))
	if err != nil {
		server.logger.Fatalf("X509KeyPair: %s", err)
		return
	}
	tlsConfig.Certificates[0] = cert

	httpServer := &http.Server{
		Addr:      server.Config.ListenAddr,
		Handler:   &srv,
		TLSConfig: &tlsConfig,
	}
	err = httpServer.ListenAndServeTLS("", "")
	if err != nil {
		server.logger.Fatalf("ListenAndServe: %s", err)
	}
}
