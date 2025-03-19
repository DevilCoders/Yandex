package main

import (
	"context"
	"fmt"
	"time"

	stdlog "log"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"

	rm "a.yandex-team.ru/cloud/marketplace/pkg/clients/resource-manager"
)

const requestTimeOut = 20 * time.Second

var (
	rmEndpoint string
	caPath     string

	cloudID string

	debugMode bool
)

var rootCmd = &cobra.Command{
	Use:   "dev",
	Short: "Make requests to Resources Manager Private API",
}

var getPermissionsStagesCmd = &cobra.Command{
	Use:   "get-permissions-stages",
	Short: "Get permission stages for specified cloud-id",

	Run: runGetPermissionsStages,
}

func init() {
	rootCmd.PersistentFlags().BoolVarP(
		&debugMode, "debug-mode", "d", false, "enable debug mode",
	)

	rootCmd.PersistentFlags().StringVarP(
		&rmEndpoint, "rm-endpoint", "r", "", "resource manager private api endpoint",
	)

	if err := rootCmd.MarkPersistentFlagRequired("rm-endpoint"); err != nil {
		stdlog.Fatal(err)
	}

	rootCmd.PersistentFlags().StringVarP(
		&caPath, "ca-path", "t", "", "cert file path",
	)

	getPermissionsStagesCmd.PersistentFlags().StringVarP(
		&cloudID, "cloud-id", "i", "", "cloud id to resolve billing account for",
	)

	if err := getPermissionsStagesCmd.MarkPersistentFlagRequired("cloud-id"); err != nil {
		stdlog.Fatal(err)
	}

	rootCmd.AddCommand(
		getPermissionsStagesCmd,
	)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		stdlog.Fatal(err)
	}
}

func runGetPermissionsStages(_ *cobra.Command, _ []string) {
	cli, err := newClient()
	if err != nil {
		stdlog.Fatal(err)
	}

	ctx, cancel := defaultRequestContext()
	defer cancel()

	result, err := cli.GetPermissionStages(ctx, cloudID)
	if err != nil {
		stdlog.Fatal(err)
	}

	fmt.Println("results count", len(result))
	for i := range result {
		fmt.Println("\t", result[i])
	}
}

func newClient() (*rm.Client, error) {
	config := makeClientConfig()

	logger, err := makeDefaultLogger()
	if err != nil {
		return nil, err
	}

	return rm.NewClientWithYCDefaultAuth(context.Background(), config, logger)
}

func makeDefaultLogger() (log.Logger, error) {
	return zap.New(zap.JSONConfig(log.DebugLevel))
}

func makeClientConfig() (out rm.Config) {
	out.Endpoint = rmEndpoint
	out.CAPath = caPath
	out.DebugMode = debugMode

	out.InitTimeout = 20 * time.Second

	return
}

func newRequestContext(timeOut time.Duration) (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), timeOut)
}

func defaultRequestContext() (context.Context, context.CancelFunc) {
	return newRequestContext(requestTimeOut)
}
