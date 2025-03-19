package main

import (
	"context"
	"fmt"
	"log"

	"github.com/spf13/cobra"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

type options struct {
	Host    string
	Port    uint32
	DiskId  string
	Batch   uint32
	Verbose bool
}

func run(ctx context.Context, opts *options) error {
	level := nbs.LOG_INFO
	if opts.Verbose {
		level = nbs.LOG_DEBUG
	}

	nbsLog := nbs.NewStderrLog(level)

	grpcOpts := &nbs.GrpcClientOpts{
		Endpoint: fmt.Sprintf("%v:%v", opts.Host, opts.Port),
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

	return RepairVolume(
		ctx,
		client,
		opts.DiskId,
		uint32(volume.BlocksCount),
		opts.Batch)
}

func main() {
	var opts options

	rootCmd := cobra.Command{
		Use: "blockstore-volume-repairer",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVar(&opts.Host, "host", "localhost", "nsb server")
	rootCmd.Flags().Uint32Var(&opts.Port, "port", 9766, "nbs port")
	rootCmd.Flags().StringVar(&opts.DiskId, "disk-id", "", "disk to repair")
	rootCmd.Flags().Uint32Var(&opts.Batch, "batch", 100000, "describe blocks request blocks count")
	rootCmd.Flags().BoolVar(&opts.Verbose, "verbose", false, "verbose mode")

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
