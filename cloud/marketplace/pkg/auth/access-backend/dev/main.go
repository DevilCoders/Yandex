package main

import (
	"context"
	"time"

	stdlog "log"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/permissions"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"

	as "a.yandex-team.ru/cloud/marketplace/pkg/auth/access-backend"
)

const requestTimeOut = 20 * time.Second

var (
	asEndpoint string
	caPath     string

	iamToken string
)

var rootCmd = &cobra.Command{
	Use:   "dev",
	Short: "Make requests to Access Service Private API",
}

var authorizeCmd = &cobra.Command{
	Use:   "authorize",
	Short: "Authorize as billing admin with provided token",

	Run: runAuthorizeCmd,
}

func init() {
	rootCmd.PersistentFlags().StringVarP(
		&asEndpoint, "as-endpoint", "r", "", "access service api endpoint",
	)

	if err := rootCmd.MarkPersistentFlagRequired("as-endpoint"); err != nil {
		stdlog.Fatal(err)
	}

	rootCmd.PersistentFlags().StringVarP(
		&caPath, "ca-path", "c", "", "cert file path",
	)

	authorizeCmd.PersistentFlags().StringVarP(
		&iamToken, "iam-token", "t", "", "your iam token (se yc cli for details)",
	)

	if err := authorizeCmd.MarkPersistentFlagRequired("iam-token"); err != nil {
		stdlog.Fatal(err)
	}

	rootCmd.AddCommand(
		authorizeCmd,
	)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		stdlog.Fatal(err)
	}
}

func runAuthorizeCmd(_ *cobra.Command, _ []string) {
	cli, err := newClient()
	if err != nil {
		stdlog.Fatal(err)
	}

	defer cli.Close()

	ctx, cancel := defaultRequestContext()
	defer cancel()

	if err = cli.AuthorizeBillingAdminWithToken(ctx, iamToken, permissions.LicenseCheckPermission); err != nil {
		stdlog.Fatal(err)
	}
}

func newClient() (*as.Client, error) {
	config := makeClientConfig()

	logger, err := makeDefaultLogger()
	if err != nil {
		return nil, err
	}

	logging.SetLogger(logger.Structured())

	return as.NewClient(context.Background(), config)
}

func makeDefaultLogger() (log.Logger, error) {
	return zap.New(zap.JSONConfig(log.DebugLevel))
}

func makeClientConfig() (out as.Config) {
	out.Endpoint = asEndpoint
	out.CAPath = caPath

	return
}

func newRequestContext(timeOut time.Duration) (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), timeOut)
}

func defaultRequestContext() (context.Context, context.CancelFunc) {
	return newRequestContext(requestTimeOut)
}
