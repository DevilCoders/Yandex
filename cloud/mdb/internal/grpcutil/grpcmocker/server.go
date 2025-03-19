package grpcmocker

import (
	"crypto/tls"
	"io/ioutil"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/mdb/internal/app/signals"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/parsers"
	"a.yandex-team.ru/cloud/mdb/internal/x/net"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RunOption func(o options) options

type options struct {
	ServerOptions          []grpc.ServerOption
	ConfigFile             string
	Matcher                expectations.Matcher
	Parser                 *parsers.Parser
	DynamicControlAddr     string
	DynamicControlRegister RegisterDynamicControlFunc
	L                      log.Logger
}

// WithExpectationsFromFile enables reading expectations from file
func WithExpectationsFromFile(path string) RunOption {
	return func(o options) options {
		o.ConfigFile = path
		return o
	}
}

// WithExpectations enables custom-build expectations
func WithExpectations(exp expectations.Matcher) RunOption {
	return func(o options) options {
		o.Matcher = exp
		return o
	}
}

// WithParser enables custom parser
func WithParser(parser *parsers.Parser) RunOption {
	return func(o options) options {
		o.Parser = parser
		return o
	}
}

// WithLogger enables logging
func WithLogger(l log.Logger) RunOption {
	return func(o options) options {
		o.L = l
		return o
	}
}

// WithTLS enables TLS on gRPC server
func WithTLS(cfg *tls.Config) RunOption {
	return func(o options) options {
		o.ServerOptions = append(o.ServerOptions, grpc.Creds(credentials.NewTLS(cfg)))
		return o
	}
}

// WithDynamicControl enables HTTP server to dynamically control expectations. User can write custom HTTP handlers
// that change expectations behavior. For example, user can write function responder that takes response data from
// data structures that are modified via custom HTTP calls to this mock.
// HTTP server will listen on gRPC server's host:port+1.
func WithDynamicControl(reg RegisterDynamicControlFunc) RunOption {
	return func(o options) options {
		o.DynamicControlAddr = ""
		o.DynamicControlRegister = reg
		return o
	}
}

// WithDynamicControlAddr is exactly the same as WithDynamicControl but also allows using custom listening address.
func WithDynamicControlAddr(addr string, reg RegisterDynamicControlFunc) RunOption {
	return func(o options) options {
		o.DynamicControlAddr = addr
		o.DynamicControlRegister = reg
		return o
	}
}

// Server instance of gRPC mock server.
type Server struct {
	s    *grpc.Server
	addr string
}

func (m *Server) Addr() string {
	return m.addr
}

// Stop this instance.
func (m *Server) Stop(d time.Duration) error {
	if err := grpcutil.Shutdown(m.s, d); err != nil {
		return xerrors.Errorf("shutdown: %w", err)
	}

	return nil
}

type RegisterServicesFunc func(s *grpc.Server) error

// Run instance of a mock server. Returns server instance or error. Server is already accepting connections when it is returned.
func Run(addr string, reg RegisterServicesFunc, opts ...RunOption) (*Server, error) {
	o := options{
		L: &nop.Logger{},
	}
	for _, opt := range opts {
		o = opt(o)
	}

	if o.ConfigFile == "" && o.Matcher == nil {
		return nil, xerrors.New("must provide either path to config file or predefined matcher, provided none")
	}

	if o.ConfigFile != "" && o.Matcher != nil {
		return nil, xerrors.New("must provide either path to config file or predefined matcher, not both")
	}

	if o.Parser != nil && o.ConfigFile == "" {
		return nil, xerrors.New("provided parser but not path to config file")
	}

	if o.ConfigFile != "" {
		cfg, err := ioutil.ReadFile(o.ConfigFile)
		if err != nil {
			return nil, xerrors.Errorf("read config file %q: %w", o.ConfigFile, err)
		}

		if o.Parser == nil {
			o.Parser = parsers.NewDefaultParser()
		}

		o.Matcher, err = o.Parser.Parse(cfg)
		if err != nil {
			return nil, xerrors.Errorf("parse config %q: %w", o.ConfigFile, err)
		}
	}

	if o.DynamicControlRegister != nil {
		dynControlAddr := o.DynamicControlAddr
		if dynControlAddr == "" {
			var err error
			dynControlAddr, err = net.IncrementPort(addr, 1)
			if err != nil {
				return nil, err
			}
		}
		if err := serveDynamicControl(dynControlAddr, o.DynamicControlRegister, o.L); err != nil {
			return nil, err
		}
	}

	mock := &Mock{}

	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			mock.NewUnaryServerInterceptor(o.L),
		),
	}
	options = append(options, o.ServerOptions...)
	server := grpc.NewServer(options...)
	if err := reg(server); err != nil {
		return nil, xerrors.Errorf("register services: %w", err)
	}

	reflection.Register(server)

	if err := mock.Register(server, o.Matcher); err != nil {
		return nil, xerrors.Errorf("register mock: %w", err)
	}

	l, err := grpcutil.Serve(server, addr, o.L)
	if err != nil {
		return nil, xerrors.Errorf("serve: %w", err)
	}

	return &Server{s: server, addr: l.Addr().String()}, nil
}

// RunAndWait mock server. Waits for SIGINT or SIGTERM, then stops.
//
// This is the method that should be used in most cases.
func RunAndWait(addr string, reg RegisterServicesFunc, opts ...RunOption) error {
	server, err := Run(addr, reg, opts...)
	if err != nil {
		return err
	}

	signals.WaitForStop()
	return server.Stop(time.Second)
}
