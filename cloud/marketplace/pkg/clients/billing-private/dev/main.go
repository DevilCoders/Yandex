package main

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	stdlog "log"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

const requestTimeOut = 20 * time.Second

var (
	billingEndpoint string

	cloudID string
)

var rootCmd = &cobra.Command{
	Use:   "dev",
	Short: "Run requested billing API commands with default token",
}

var resolveBillingAccountCmd = &cobra.Command{
	Use:   "resolve",
	Short: "Resolve billing account(s)",
}

var resolveAllBillingAccountsCmd = &cobra.Command{
	Use:   "all",
	Short: "Post billingAccounts:resolve all",

	Run: runResolveAllBillingAccounts,
}

var resolveBillingAccountByCloudIDCmd = &cobra.Command{
	Use:   "by-cloud-id",
	Short: "Resolve billing account by cloud id and effective time",

	Run: runResolveAllBillingAccountsByCloudID,
}

func init() {
	rootCmd.PersistentFlags().StringVarP(
		&billingEndpoint, "billing-endpoint", "b", "", "billing API endpoint",
	)

	if err := rootCmd.MarkPersistentFlagRequired("billing-endpoint"); err != nil {
		stdlog.Fatal(err)
	}

	resolveBillingAccountCmd.PersistentFlags().StringVarP(
		&cloudID, "cloud-id", "i", "", "cloud id to resolve billing account for",
	)

	if err := resolveBillingAccountCmd.MarkPersistentFlagRequired("cloud-id"); err != nil {
		stdlog.Fatal(err)
	}

	rootCmd.AddCommand(
		resolveBillingAccountCmd,
	)

	resolveBillingAccountCmd.AddCommand(
		resolveAllBillingAccountsCmd,
		resolveBillingAccountByCloudIDCmd,
	)
}

func main() {
	if err := rootCmd.Execute(); err != nil {
		stdlog.Fatal(err)
	}
}

func runResolveAllBillingAccounts(_ *cobra.Command, _ []string) {
	cli, err := newClient()
	if err != nil {
		stdlog.Fatal(err)
	}

	ctx, cancel := defaultRequestContext()
	defer cancel()

	authenticator := auth.NewYCDefaultTokenAuthenticator(ctx)

	iamToken, err := authenticator.Token(ctx)
	if err != nil {
		stdlog.Fatal(err)
	}

	s := cli.SessionWithYCSubjectToken(ctx, iamToken)

	result, err := s.ResolveBillingAccounts(billing.ResolveBillingAccountsParams{
		CloudID: cloudID,
	})

	if err != nil {
		stdlog.Fatal(err)
	}

	printJSONResult(result)
}

func runResolveAllBillingAccountsByCloudID(_ *cobra.Command, _ []string) {
	cli, err := newClient()
	if err != nil {
		stdlog.Fatal(err)
	}

	ctx, cancel := defaultRequestContext()
	defer cancel()

	authenticator := auth.NewYCDefaultTokenAuthenticator(ctx)

	iamToken, err := authenticator.Token(ctx)
	if err != nil {
		stdlog.Fatal(err)
	}

	result, err := cli.SessionWithYCSubjectToken(ctx, iamToken).ResolveBillingAccountByCloudIDFull(cloudID, time.Now().Unix())

	if err != nil {
		stdlog.Fatal(err)
	}

	printJSONResult(result)
}

func newClient() (*billing.Client, error) {
	config := makeClientConfig()

	logger, err := makeDefaultLogger()
	if err != nil {
		return nil, err
	}

	return billing.NewClient(config, logger), nil
}

func makeDefaultLogger() (log.Logger, error) {
	return zap.New(zap.JSONConfig(log.DebugLevel))
}

func makeClientConfig() (out billing.Config) {
	out.Endpoint = billingEndpoint
	return
}

func newRequestContext(timeOut time.Duration) (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), timeOut)
}

func defaultRequestContext() (context.Context, context.CancelFunc) {
	return newRequestContext(requestTimeOut)
}

func printJSONResult(result interface{}) {
	bJs, err := json.MarshalIndent(result, "", "  ")
	if err != nil {
		stdlog.Fatal(err)
	}

	fmt.Println(string(bJs))
}
