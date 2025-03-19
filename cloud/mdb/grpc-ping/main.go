package main

import (
	"context"
	"crypto/x509"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"
	"google.golang.org/grpc/credentials"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log/nop"
)

var (
	timeout      int
	url          string
	insecure     bool
	certFile     string
	serverName   string
	monrunOutput bool
	slbCloseFile string
)

func init() {
	pflag.IntVar(&timeout, "timeout", 30, "timeout to wait for dial and call ping method")
	pflag.StringVar(&url, "url", "localhost:5051", "URL where service published")
	pflag.BoolVar(&insecure, "insecure", false, "Use insecure connection")
	pflag.StringVar(&certFile, "cert_file", "", "Use certificate chain from PEM file")
	pflag.StringVar(&serverName, "server_name", "", "Server name override")
	pflag.BoolVar(&monrunOutput, "monrun", false, "Format output in monrun-compatible form")
	pflag.StringVar(&slbCloseFile, "slb_close_file", "", "Check file if server is closed from SLB")
}

func systemCreds() credentials.TransportCredentials {
	pool, err := x509.SystemCertPool()
	if err != nil {
		fmt.Printf("failed to get system cert Pool: %v", err)
	}
	return credentials.NewClientTLSFromCert(pool, serverName)
}

func certCreds() credentials.TransportCredentials {
	creds, err := credentials.NewClientTLSFromFile(certFile, serverName)
	if err != nil {
		fmt.Printf("failed to load credentials: %v", err)
		os.Exit(1)
	}
	return creds
}

func report(code int, message string) {
	if monrunOutput {
		fmt.Printf("%d;%s\n", code, message)
		os.Exit(0)
	} else {
		if code != 0 {
			fmt.Println(message)
		}
		os.Exit(code)
	}
}

func main() {
	pflag.Parse()

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*time.Duration(timeout))
	defer cancel()

	if _, err := os.Stat(slbCloseFile); err == nil {
		report(1, "server is closed from SLB")
	}

	conn, err := grpcutil.NewConn(
		ctx,
		url,
		"grpc-ping",
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile:     certFile,
					ServerName: serverName,
				},
				Insecure: insecure,
			},
		},
		&nop.Logger{},
	)
	if err != nil {
		report(2, fmt.Sprintf("failed to dial: %+v", err))
	}

	grpcClient := grpchealth.NewHealthClient(conn)
	response, err := grpcClient.Check(ctx, &grpchealth.HealthCheckRequest{})
	if err != nil {
		report(2, fmt.Sprintf("request error: %+v", err))
	}

	if response.Status != grpchealth.HealthCheckResponse_SERVING {
		report(2, fmt.Sprintf("service status: %s", response.Status))
	}

	report(0, "OK")
}
