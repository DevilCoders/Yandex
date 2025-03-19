package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"log"
	"os"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/cloudmeta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/grpccon"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam"
)

var runCtx, stop = context.WithCancel(context.Background())

func main() {
	defer stop()
	cl := connectIAM()

	if hc, ok := cl.(interface{ HealthCheck(context.Context) error }); ok {
		if err := hc.HealthCheck(runCtx); err != nil {
			log.Fatal(err)
		}
		fmt.Println("healthcheck ok")
	}

	// want CLOUD = aoe47jkb996097rp5rnr
	resp, err := cl.ResolveFolder(runCtx, "aoe401u4tao7jj2b069g", "aoe8qo1krj6vc24sh2dh")
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("%#v\n", resp)
}

func connectIAM() iam.RMClient {
	conn, err := grpccon.Connect(runCtx, "rm.private-api.cloud-preprod.yandex.net:4284",
		grpccon.Config{TLS: getTLS()},
		cloudmeta.New(runCtx).GRPCAuth(),
	)
	if err != nil {
		log.Fatal(err)
	}

	return iam.NewRMClient(runCtx, conn)
}

const caPath = "/etc/ssl/certs/ca-certificates.crt"

func getTLS() *tls.Config {
	p, err := os.ReadFile(caPath)
	if err != nil {
		log.Fatal(err)
	}

	roots, err := x509.SystemCertPool()
	if err != nil {
		log.Fatal(err)
	}

	roots.AppendCertsFromPEM(p)
	return &tls.Config{MinVersion: tls.VersionTLS12, RootCAs: roots}
}
