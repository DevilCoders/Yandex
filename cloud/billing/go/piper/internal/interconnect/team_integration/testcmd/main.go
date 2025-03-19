package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"log"
	"os"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/cloudjwt"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/grpccon"
	teamintegration "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration"
)

var (
	runCtx, stop    = context.WithCancel(context.Background())
	userAgentOption = grpc.WithUserAgent("Yandex Object Storage")
)

func main() {
	defer stop()
	cl := connectTI()

	// if hc, ok := cl.(interface{ HealthCheck(context.Context) error }); ok {
	// 	if err := hc.HealthCheck(runCtx); err != nil {
	// 		log.Fatal(err)
	// 	}
	// 	fmt.Println("healthcheck ok")
	// }

	resp, err := cl.ResolveABC(runCtx, 2224)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("%#v\n", resp)
}

func connectTI() teamintegration.TIClient {
	tlsc := getTLS()
	tickConn, err := grpccon.Connect(runCtx, "ts.cloud.yandex-team.ru:4282",
		grpccon.Config{TLS: tlsc}, nil,
	)
	if err != nil {
		log.Fatal(err)
	}

	auth, err := cloudjwt.New(runCtx, tickConn, loadKey())
	if err != nil {
		log.Fatal(err)
	}

	if err := auth.HealthCheck(runCtx); err != nil {
		log.Fatal(err)
	}

	conn, err := grpccon.Connect(runCtx, "ti.cloud.yandex-team.ru:443",
		grpccon.Config{TLS: tlsc},
		auth.GRPCAuth(),
	)
	if err != nil {
		log.Fatal(err)
	}

	return teamintegration.NewTIClient(runCtx, conn)
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

func loadKey() cloudjwt.Config {
	fd, err := os.ReadFile("/var/lib/yc/billing/system_account.json")
	if err != nil {
		log.Fatal(err)
	}

	var data struct {
		PrivateKey       string `json:"private_key"`
		KeyID            string `json:"key_id"`
		ServiceAccountID string `json:"service_account_id"`
	}
	err = json.Unmarshal(fd, &data)
	if err != nil {
		log.Fatal(err)
	}
	return cloudjwt.Config{
		Audience:   "https://iam.api.cloud.yandex.net/iam/v1/tokens",
		AccountID:  data.ServiceAccountID,
		KeyID:      data.KeyID,
		PrivateKey: []byte(data.PrivateKey),
	}
}
