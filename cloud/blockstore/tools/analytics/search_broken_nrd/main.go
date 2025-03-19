package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"

	"github.com/spf13/cobra"

	protos "a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

////////////////////////////////////////////////////////////////////////////////

type options struct {
	Disks     string
	Timestamp uint64
	Verbose   bool
}

func loadDisks(fileName string) ([]string, error) {
	file, err := os.Open(fileName)
	if err != nil {
		return nil, fmt.Errorf("can't open file: %w", err)
	}

	scanner := bufio.NewScanner(file)

	var disks []string

	for scanner.Scan() {
		disks = append(disks, scanner.Text())
	}

	err = scanner.Err()
	if err != nil {
		return nil, fmt.Errorf("fail to scan: %w", err)
	}

	return disks, nil
}

func run(ctx context.Context, opts *options) error {
	level := nbs_client.LOG_INFO
	if opts.Verbose {
		level = nbs_client.LOG_DEBUG
	}

	nbsLog := nbs_client.NewStderrLog(level)

	info := nbsLog.Logger(nbs_client.LOG_INFO)

	nbs, err := nbs_client.NewClient(
		&nbs_client.GrpcClientOpts{
			Endpoint: "localhost:9766",
		},
		&nbs_client.DurableClientOpts{},
		nbsLog,
	)
	if err != nil {
		return err
	}

	disks, err := loadDisks(opts.Disks)
	if err != nil {
		return err
	}

	var createdAfter []*protos.TVolume
	var maybeResized []*protos.TVolume

	for _, diskID := range disks {
		vol, err := nbs.DescribeVolume(ctx, diskID)
		if err != nil {
			info.Printf(context.TODO(), "Fail DescribeVolume %v: %v", diskID, err)

			continue
		}

		if len(vol.Devices) == 1 {
			info.Printf(context.TODO(), "skip by Devices. %v: %v", diskID, len(vol.Devices))
			continue
		}

		if vol.CreationTs >= opts.Timestamp {
			createdAfter = append(createdAfter, vol)

			continue
		}

		if vol.AlterTs >= opts.Timestamp {
			maybeResized = append(maybeResized, vol)

			continue
		}

		info.Printf(
			context.TODO(),
			"skip by CreationTs|AlterTs. %v: %v %v",
			diskID,
			vol.CreationTs,
			vol.AlterTs,
		)
	}

	dump := func(vols []*protos.TVolume, tag string) {
		for _, vol := range vols {
			fmt.Println(vol.DiskId, tag)
			for _, device := range vol.Devices {
				fmt.Println("    ", device.DeviceUUID, device.BlockCount, vol.BlockSize)
			}
		}
	}

	dump(createdAfter, "[created after]")
	dump(maybeResized, "[maybe resized]")

	return nil
}

func main() {
	var opts options

	rootCmd := cobra.Command{
		Use: "search_broken_nrd",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVar(&opts.Disks, "disks", "", "path to disk list")
	rootCmd.Flags().Uint64Var(&opts.Timestamp, "timestamp", 0, "timestamp")
	rootCmd.Flags().BoolVar(&opts.Verbose, "verbose", false, "verbose mode")

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
