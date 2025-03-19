package main

import (
	"context"
	stdlog "log"

	"github.com/spf13/cobra"

	private_protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

////////////////////////////////////////////////////////////////////////////////

func isDiskRemoved(err *nbs.ClientError) bool {
	return err.Code == nbs.E_NOT_FOUND ||
		(err.Facility() == nbs.FACILITY_SCHEMESHARD && err.Status() == 2)
}

////////////////////////////////////////////////////////////////////////////////

func run(ctx context.Context, log *rebuildMetadataLog, opts *options) error {
	defer opts.completedDisks.close()

	volumes, err := readFile(opts.inputFile)
	if err != nil {
		return err
	}

	doneCh := make(chan opResponse)
	numWorkers := 0
	tasksCompleted := 0
	volumeIdx := 0

	for tasksCompleted < len(volumes) {
		if volumeIdx < len(volumes) && numWorkers < opts.numWorkers {
			if !opts.completedDisks.isDiskCompleted(volumes[volumeIdx]) {
				numWorkers++
				go worker(ctx, log, volumes[volumeIdx], opts, doneCh)
			} else {
				log.logInfo(ctx, "Skip disk %v as it is already processed", volumes[volumeIdx])
				tasksCompleted++
			}
			volumeIdx++
			continue
		}
		response := <-doneCh
		numWorkers--
		tasksCompleted++

		if response.result != nil {
			if clientError, ok := response.result.(*nbs.ClientError); ok {
				if clientError.Code == nbs.E_NOT_IMPLEMENTED || isDiskRemoved(clientError) {
					response.result = nil
				}
			}
		}

		if response.result != nil {
			log.logError(
				ctx,
				"rebuild metadata for disk %v completed with error: %v",
				response.stats.diskID,
				response.result)
		} else {
			err = opts.completedDisks.addCompletedDisk(response.stats.diskID)
			if err != nil {
				log.logError(
					ctx,
					"cannot store disk %v in a completed disks log: %v",
					response.stats.diskID,
					err)
			} else {
				log.logInfo(
					ctx,
					"rebuild metadata for disk %v completed: [%v,%v] time(ms): %v extra: %v",
					response.stats.diskID,
					response.stats.processed,
					response.stats.total,
					response.stats.operationDuration,
					response.stats.extra)
			}
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func prepareAndRun(rebuildType private_protos.ERebuildMetadataType, opts *options) {
	ctx, cancelCtx := context.WithCancel(context.Background())
	defer cancelCtx()

	logLevel := nbs.LOG_INFO
	if opts.verbose {
		logLevel = nbs.LOG_DEBUG
	}

	log := createRebuildMetadataLog(logLevel)

	opts.completedDisks = completedDisks{}
	if opts.completionLogFile != "" {
		err := opts.completedDisks.load(opts.completionLogFile)

		if err != nil {
			log.logError(ctx, "Error processing completion log: %v", err)
			return
		}
	}

	var err error
	opts.token, err = readToken(opts)
	if err != nil {
		log.logError(ctx, "Error setting up token: %v", err)
		return
	}

	opts.rebuildType = rebuildType
	if err := run(ctx, log, opts); err != nil {
		log.logError(ctx, "Error: %v", err)
	}
}

////////////////////////////////////////////////////////////////////////////////

func setupParameters(opts *options) *cobra.Command {
	usedBlockCountCmd := &cobra.Command{
		Use:   "used-block-count",
		Short: "Used Block Count Metadata rebuild",
		Run: func(cmd *cobra.Command, args []string) {
			prepareAndRun(private_protos.ERebuildMetadataType_USED_BLOCKS, opts)
		},
	}
	usedBlockCountCmd.Flags().Uint32VarP(&opts.batchSize, "batch-size", "", 1024, "number of blocks per batch")

	blockCountCmd := &cobra.Command{
		Use:   "block-count",
		Short: "Mixed/Merged Block Count rebuild",
		Run: func(cmd *cobra.Command, args []string) {
			prepareAndRun(private_protos.ERebuildMetadataType_BLOCK_COUNT, opts)
		},
	}
	blockCountCmd.Flags().Uint32VarP(&opts.batchSize, "batch-size", "", 1024, "number of blobs per batch")

	var rootCmd = &cobra.Command{
		Use:   "rebuild-metadata",
		Short: "Metadata rebuild tool",
	}
	rootCmd.PersistentFlags().StringVarP(&opts.inputFile, "input-file", "", "", "file with volumes to process")
	rootCmd.PersistentFlags().BoolVar(&opts.noAuth, "no-auth", false, "don't use IAM token")
	rootCmd.PersistentFlags().StringVarP(&opts.tokenFile, "auth-token", "", "token", "token file location")
	rootCmd.PersistentFlags().StringVarP(&opts.completionLogFile, "completion-log", "", "", "filename for list of processed volumes")
	rootCmd.PersistentFlags().StringVarP(&opts.host, "host", "", "localhost", "NBS hostname")
	rootCmd.PersistentFlags().IntVarP(&opts.port, "port", "", 9766, "NBS port")
	rootCmd.PersistentFlags().IntVarP(&opts.securePort, "secure-port", "", 9768, "NBS secure port")
	rootCmd.PersistentFlags().IntVarP(&opts.numWorkers, "workers", "", 1, "Number of workers")
	rootCmd.PersistentFlags().BoolVar(&opts.verbose, "verbose", false, "extra execution info")

	rootCmd.AddCommand(usedBlockCountCmd, blockCountCmd)

	return rootCmd
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var opts options

	rootCmd := setupParameters(&opts)

	if err := rootCmd.Execute(); err != nil {
		stdlog.Default().Fatalf("can't execute root command: %v", err)
	}
}
