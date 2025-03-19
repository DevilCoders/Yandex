package main

import (
	"context"
	"time"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type cmdMetricsToFile struct {
	ctx                 context.Context
	filename            string
	outputFilename      string
	maxRecords          int
	aggregatedBatchSize int
	aggregators         int
}

func NewCmdMetricsToFile(ctx context.Context) *cmdMetricsToFile {
	return &cmdMetricsToFile{ctx: ctx}
}

func (c *cmdMetricsToFile) BuildCobraCommand() *cobra.Command {
	pushJSONCmd := &cobra.Command{
		Use: "metricstofile",
		Run: c.run,
	}

	pushJSONCmd.PersistentFlags().StringVarP(&c.filename, "filename", "f", "", ".json or .json.gz filename")
	pushJSONCmd.PersistentFlags().StringVarP(&c.outputFilename, "output", "o", "", "output .json or .json.gz filename")
	pushJSONCmd.PersistentFlags().IntVarP(&c.maxRecords, "records", "r", 0, "maximum records to read, 0=inf")
	pushJSONCmd.PersistentFlags().IntVarP(&c.aggregatedBatchSize, "batchsize", "b", 60*1000, "aggregated batch size")
	pushJSONCmd.PersistentFlags().IntVarP(&c.aggregators, "aggregators", "a", 1, "aggregator workers count")
	if err := pushJSONCmd.MarkPersistentFlagRequired("filename"); err != nil {
		panic(err)
	}
	if err := pushJSONCmd.MarkPersistentFlagRequired("output"); err != nil {
		panic(err)
	}
	return pushJSONCmd
}

func (c *cmdMetricsToFile) run(cmd *cobra.Command, _ []string) {
	var readCnt int
	var totalCost decimal.Decimal128
	var writeCnt int

	reader := NewMetricReader(cmd)
	aggregator := NewAggregator(cmd)
	writer := NewFileWriter(cmd)

	cmd.Printf("Performing push data from plain json-lines file %s...\n", c.filename)
	cmd.Printf("filename = %s\n", c.filename)
	cmd.Printf("outputFilename = %s\n", c.outputFilename)
	cmd.Printf("maxRecords = %d\n", c.maxRecords)
	cmd.Printf("aggregatedBatchSize = %d\n", c.aggregatedBatchSize)
	cmd.Printf("aggregators = %d\n", c.aggregators)

	started := time.Now()

	eg, ctx := errgroup.WithContext(c.ctx)

	r, rPromise := reader.Run(ctx, c.filename, c.maxRecords)
	eg.Go(func() (err error) { readCnt, totalCost, err = rPromise(); return })

	a, aggrPromise := aggregator.Run(ctx, c.aggregators, r, c.aggregatedBatchSize)
	eg.Go(func() error { return aggrPromise() })

	wrPromise := writer.Run(ctx, c.outputFilename, a)
	eg.Go(func() (err error) { writeCnt, err = wrPromise(); return })

	err := eg.Wait()
	if err != nil {
		cmd.PrintErrf("Processing error: %s\n", err.Error())
		return
	}

	duration := time.Since(started).Seconds()
	cmd.Println("All done.")
	cmd.Printf("Read: %d records on %.2f seconds (%.2f rec/sec).\n", readCnt, duration, float64(readCnt)/duration)
	cmd.Printf("Total read cost = %s.\n", totalCost.String())
	cmd.Printf("Write: %d records on %.2f seconds (%.2f rec/sec).\n", writeCnt, duration, float64(writeCnt)/duration)
}
