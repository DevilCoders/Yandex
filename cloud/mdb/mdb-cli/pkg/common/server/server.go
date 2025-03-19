package server

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"sync"
	"time"

	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/timestamp"

	"a.yandex-team.ru/library/go/core/log"
)

type URLRequestFunc func(serverAddr string) error

func GetToken(ctx context.Context, f URLRequestFunc, consoleURL string, logger log.Logger) (*Token, error) {
	logger.Debug("starting token server on localhost")
	srv, err := serve(logger, consoleURL)
	if err != nil {
		logger.Error("token server start failed", log.Error(err))
		return nil, err
	}

	logger.Debug(fmt.Sprintf("opening browser, server address: %s", srv.Addr().String()))
	err = f(srv.Addr().String())
	if err != nil {
		logger.Error("failed to open browser", log.Error(err))
		_ = srv.Shutdown(ctx)
		return nil, err
	}

	logger.Debug("waiting for token")
	select {
	case <-srv.Served():
	case <-ctx.Done():
	}

	logger.Debug("waiting for token done")

	// ensure server shutdown before reading Token from the struct
	logger.Debug("token server shutdown")
	err = srv.Shutdown(ctx)

	if err != nil {
		logger.Error("error during waiting for token or server shutdown", log.Error(err))
	}
	return srv.Token(), err
}

func (s *server) Token() *Token {
	return s.token
}

func (s *server) Served() <-chan struct{} {
	return s.served
}

type Token struct {
	IamToken  string
	ExpiresAt *timestamp.Timestamp
	Err       error
}

type server struct {
	srv        http.Server
	lsn        net.Listener
	err        chan error
	served     chan struct{}
	token      *Token
	mux        sync.Mutex
	logger     log.Logger
	consoleURL string
}

func (s *server) Addr() net.Addr {
	return s.lsn.Addr()
}

func (s *server) Serve() error {
	var err error
	s.lsn, err = listener(s.logger)
	if err != nil {
		s.logger.Error("failed to get listener", log.Error(err))
		return err
	}

	s.err = make(chan error)
	s.served = make(chan struct{}, 1)
	mux := http.NewServeMux()
	mux.Handle("/", s)
	s.srv.Handler = mux
	go func() {
		s.err <- s.srv.Serve(s.lsn)
	}()

	return nil
}

func (s *server) Shutdown(ctx context.Context) error {
	err := s.srv.Shutdown(ctx)
	if err != nil {
		s.logger.Error("failed to shutdown server", log.Error(err))
		return err
	}
	select {
	case err = <-s.err:
		if err != http.ErrServerClosed {
			s.logger.Error("failed to close server", log.Error(err))
			return err
		}
	case <-ctx.Done():
		s.logger.Error("context is done", log.Error(ctx.Err()))
		return ctx.Err()
	}

	return nil
}

func (s *server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	s.mux.Lock()
	defer s.mux.Unlock()
	if s.token != nil {
		s.logger.Debug("token server handler is called twice")
		return
	}

	s.logger.Debug("token server handler called")
	defer redirectToConsole(w, r, s.consoleURL)

	s.token = &Token{}
	defer func() {
		close(s.served)
	}()

	flags := r.URL.Query()

	tokens, ok := flags["token"]
	if !ok || len(tokens) != 1 {
		s.logger.Error(fmt.Sprintf("failed to fetch token from http request: %s", r.RequestURI))
		s.token.Err = fmt.Errorf("failed to fetch token from http request")
		return
	}

	s.token.IamToken = tokens[0]

	times, ok := flags["expiresAt"]
	if !ok || len(times) != 1 {
		s.logger.Error(fmt.Sprintf("failed to fetch token expiration timestamp from http request: %s", r.RequestURI))
		s.token.Err = fmt.Errorf("failed to fetch token expiration timestamp from http request")
		return
	}

	t, err := time.Parse(time.RFC3339, times[0])
	if err != nil {
		s.logger.Error(fmt.Sprintf("failed to parse token expiration timestamp from: %s", times[0]), log.Error(err))
		s.token.Err = fmt.Errorf("failed to parse token expiration timestamp from: %s, %v", times[0], err)
		return
	}

	s.token.ExpiresAt, err = ptypes.TimestampProto(t)
	if err != nil {
		s.logger.Error(fmt.Sprintf("failed to parse token expiration timestamp from: %s", times[0]), log.Error(err))
		s.token.Err = fmt.Errorf("failed to parse token expiration timestamp from: %s, %v", times[0], err)
		return
	}
}

func redirectToConsole(w http.ResponseWriter, r *http.Request, consoleURL string) {
	if consoleURL == "" {
		return
	}

	http.Redirect(w, r, consoleURL, http.StatusSeeOther)
}

func serve(logger log.Logger, consoleURL string) (*server, error) {
	srv := &server{
		logger:     logger,
		consoleURL: consoleURL,
	}
	err := srv.Serve()
	if err != nil {
		return nil, err
	}

	return srv, nil
}

func listener(logger log.Logger) (net.Listener, error) {
	l, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		logger.Error("listening on IPv4 failed, try IPv6", log.Error(err))
		if l, err = net.Listen("tcp6", "[::1]:0"); err != nil {
			return nil, err
		}
	}
	return l, nil
}
