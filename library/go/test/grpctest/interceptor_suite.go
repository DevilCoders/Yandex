// Copyright 2016 Michal Witkowski. All Rights Reserved.
// See LICENSE for licensing terms.

package grpctest

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"flag"
	"fmt"
	"net"
	"time"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/library/go/core/resource"
	"a.yandex-team.ru/library/go/test/grpctest/testproto"
)

var (
	flagTLS = flag.Bool("use_tls", true, "whether all gRPC middleware tests should use tls")
)

func getRawCertificatePrivateKey() ([]byte, []byte) {
	rawCertificate := resource.MustGet("certs/localhost.crt")
	rawPrivateKey := resource.MustGet("certs/localhost.key")
	return rawCertificate, rawPrivateKey
}

func getServerCertificate() (*tls.Certificate, error) {
	rawCertificate, rawPrivateKey := getRawCertificatePrivateKey()
	cert, err := tls.X509KeyPair(rawCertificate, rawPrivateKey)
	if err != nil {
		return nil, err
	}
	return &cert, nil
}

func getClientCertPool() (*x509.CertPool, error) {
	rawCertificate, _ := getRawCertificatePrivateKey()

	cp := x509.NewCertPool()
	if !cp.AppendCertsFromPEM(rawCertificate) {
		return nil, fmt.Errorf("credentials: failed to append certificates")
	}
	return cp, nil
}

// InterceptorTestSuite is a testify/Suite that starts a gRPC PingService server and a client.
type InterceptorTestSuite struct {
	suite.Suite

	TestService testproto.TestServiceServer
	ServerOpts  []grpc.ServerOption
	ClientOpts  []grpc.DialOption

	ServerListener net.Listener
	Server         *grpc.Server
	clientConn     *grpc.ClientConn
	Client         testproto.TestServiceClient
}

func (s *InterceptorTestSuite) SetupSuite() {
	var err error
	s.ServerListener, err = net.Listen("tcp", "127.0.0.1:0")
	require.NoError(s.T(), err, "must be able to allocate a port for serverListener")
	if *flagTLS {
		certificate, err := getServerCertificate()
		s.Require().NoError(err)
		creds := credentials.NewServerTLSFromCert(certificate)
		s.ServerOpts = append(s.ServerOpts, grpc.Creds(creds))
	}
	// This is the point where we hook up the interceptor
	s.Server = grpc.NewServer(s.ServerOpts...)
	// Crete a service of the instantiator hasn't provided one.
	if s.TestService == nil {
		s.TestService = &TestPingService{T: s.T()}
	}
	testproto.RegisterTestServiceServer(s.Server, s.TestService)

	go func() {
		_ = s.Server.Serve(s.ServerListener)
	}()
	s.Client = s.NewClient(s.ClientOpts...)
}

func (s *InterceptorTestSuite) NewClient(dialOpts ...grpc.DialOption) testproto.TestServiceClient {
	newDialOpts := append(dialOpts, grpc.WithBlock())
	if *flagTLS {
		certPool, err := getClientCertPool()
		s.Require().NoError(err)
		creds := credentials.NewClientTLSFromCert(certPool, "localhost")
		newDialOpts = append(newDialOpts, grpc.WithTransportCredentials(creds))
	} else {
		newDialOpts = append(newDialOpts, grpc.WithInsecure())
	}

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	clientConn, err := grpc.DialContext(ctx, s.ServerAddr(), newDialOpts...)
	require.NoError(s.T(), err, "must not error on client Dial")
	return testproto.NewTestServiceClient(clientConn)
}

func (s *InterceptorTestSuite) ServerAddr() string {
	return s.ServerListener.Addr().String()
}

func (s *InterceptorTestSuite) TearDownSuite() {
	time.Sleep(10 * time.Millisecond)
	if s.ServerListener != nil {
		s.Server.GracefulStop()
		s.T().Logf("stopped grpc.Server at: %v", s.ServerAddr())
		_ = s.ServerListener.Close()

	}
	if s.clientConn != nil {
		_ = s.clientConn.Close()
	}
}
