package main

import (
	"context"
	"fmt"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	url        string
	certFile   string
	serverName string
)

func init() {
	pflag.StringVar(&url, "url", "localhost:443", "URL where service published")
	pflag.StringVar(&certFile, "cert_file", "/opt/yandex/allCAs.pem", "Use certificate chain from PEM file")
	pflag.StringVar(&serverName, "server_name", "", "Server name override")
}

func main() {
	pflag.Parse()

	cfg := grpc.DefaultConfig()
	cfg.Host = url
	cfg.Transport.Security = grpcutil.SecurityConfig{
		TLS: grpcutil.TLSConfig{
			CAFile:     certFile,
			ServerName: serverName,
		},
	}

	handler := func(ctx context.Context, logger log.Logger) monrun.Result {
		c, err := grpc.NewFromConfig(ctx, cfg, "cms-autoduty-monitoring", logger, &grpcutil.PerRPCCredentialsStatic{})
		if err != nil {
			return monrun.Result{
				Code:    monrun.CRIT,
				Message: fmt.Sprintf("can not create cms client: %s", err),
			}
		}

		status, err := c.AlarmOperations(ctx)
		if err != nil {
			return monrun.Result{
				Code:    monrun.CRIT,
				Message: fmt.Sprintf("can not get operations from gRPC API: %s", err),
			}
		}

		return monrun.Result{
			Code:    monrun.ResultCode(status.Status.Number()),
			Message: status.Description,
		}
	}

	runner.RunCheck(handler)
}
