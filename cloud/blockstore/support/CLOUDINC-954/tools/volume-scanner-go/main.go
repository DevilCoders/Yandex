package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"strings"

	"github.com/spf13/cobra"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

type options struct {
	Host         string
	IamTokenFile string
	Port         uint32
	DiskId       string
	IndexOnly    bool
	Batch        uint32
	Verbose      bool
}

////////////////////////////////////////////////////////////////////////////////

func getIAMToken(path string) (string, error) {
	bytes, err := ioutil.ReadFile(path)
	if err != nil {
		return "", err
	}

	token := string(bytes)
	return strings.Trim(token, "\n\t "), nil
}

type iamTokenProvider struct {
	token string
}

func (p *iamTokenProvider) Token(ctx context.Context) (string, error) {
	return p.token, nil
}

////////////////////////////////////////////////////////////////////////////////

func run(ctx context.Context, opts *options) error {
	level := nbs.LOG_INFO
	if opts.Verbose {
		level = nbs.LOG_DEBUG
	}

	nbsLog := nbs.NewStderrLog(level)

	iamToken, err := getIAMToken(opts.IamTokenFile)
	if err != nil {
		return err
	}

	clientCreds := &nbs.ClientCredentials{
		IAMClient: &iamTokenProvider{token:iamToken},
	}

	grpcOpts := &nbs.GrpcClientOpts{
		Endpoint: fmt.Sprintf("%v:%v", opts.Host, opts.Port),
		Credentials: clientCreds,
	}

	durableOpts := &nbs.DurableClientOpts{}

	client, err := nbs.NewClient(grpcOpts, durableOpts, nbsLog)
	if err != nil {
		return err
	}

	volume, err := client.DescribeVolume(ctx, opts.DiskId)
	if err != nil {
		return err
	}

	badRanges, err := ScanRange(
		ctx,
		client,
		opts.DiskId,
		uint32(volume.BlocksCount),
		opts.Batch)

	if err != nil {
		return err
	}

	for _, r := range badRanges {
		fmt.Printf("Bad range: %v %v\n", r.StartIndex, r.BlocksCount)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var opts options

	rootCmd := cobra.Command{
		Use: "blockstore-volume-scanner",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVar(&opts.Host, "host", "localhost", "nsb server")
	rootCmd.Flags().StringVar(&opts.IamTokenFile, "iam-token-file", "", "path to iam token")
	rootCmd.Flags().Uint32Var(&opts.Port, "port", 9766, "nbs port")
	rootCmd.Flags().StringVar(&opts.DiskId, "disk-id", "", "disk to scan")
	rootCmd.Flags().BoolVar(&opts.IndexOnly, "index-only", true, "do not actually read data")
	rootCmd.Flags().Uint32Var(&opts.Batch, "batch", 100000, "describe blocks request blocks count")
	rootCmd.Flags().BoolVar(&opts.Verbose, "verbose", false, "verbose mode")

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
