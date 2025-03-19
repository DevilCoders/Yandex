package main

import (
	"context"
	"time"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type cmdReportToYdb struct {
	ctx            context.Context
	filename       string
	writers        int
	shardingAlg    string
	limit          int
	connectionName string
}

func NewCmdReportToYdb(ctx context.Context) *cmdReportToYdb {
	return &cmdReportToYdb{ctx: ctx}
}

func (c *cmdReportToYdb) BuildCobraCommand() *cobra.Command {
	pushJSONCmd := &cobra.Command{
		Use: "reporttoydb",
		Run: c.run,
	}

	pushJSONCmd.PersistentFlags().StringVarP(&c.filename, "filename", "f", "", ".json or .json.gz filename")
	pushJSONCmd.PersistentFlags().IntVarP(&c.writers, "writers", "w", 1, "writer workers count")
	pushJSONCmd.PersistentFlags().StringVarP(&c.shardingAlg, "shardingalg", "s", "none", "sharding algorithm: none|hash|range|ydb")
	pushJSONCmd.PersistentFlags().IntVarP(&c.limit, "limit", "l", 20000, "write batch limit")
	pushJSONCmd.PersistentFlags().StringVarP(&c.connectionName, "connection", "c", "local", "ydb connection name: local|bench")
	if err := pushJSONCmd.MarkPersistentFlagRequired("filename"); err != nil {
		panic(err)
	}
	return pushJSONCmd
}

func (c *cmdReportToYdb) run(cmd *cobra.Command, _ []string) {
	var readCnt int
	var totalCost decimal.Decimal128
	var writeCnt int

	reader := NewReportReader(cmd)
	tc := DBConnect(c.ctx, ydbs[c.connectionName])
	queries := reports.NewQueries(GetDB(c.ctx, tc), qtool.QueryParams{RootPath: "rptload/"})
	ses, err := GetPool(tc).Get(c.ctx)
	if err != nil {
		cmd.PrintErrf("GetPool(tc).Get(c.ctx) error: %s\n", err.Error())
		return
	}
	writer := NewYdbWriter(cmd, queries, ses, ydbs[c.connectionName].DBPath, ShardingAlg(c.shardingAlg))

	cmd.Printf("Performing push data from json-lines file %s...\n", c.filename)
	cmd.Printf("filename = %s\n", c.filename)
	cmd.Printf("writers = %d\n", c.writers)
	cmd.Printf("shardingAlg = %s\n", c.shardingAlg)
	cmd.Printf("limit = %d\n", c.limit)
	cmd.Printf("connectionName = %s\n", c.connectionName)

	started := time.Now()

	eg, ctx := errgroup.WithContext(c.ctx)

	r, rPromise := reader.Run(ctx, c.filename)
	eg.Go(func() (err error) { readCnt, totalCost, err = rPromise(); return })

	wrPromise := writer.Run(ctx, c.writers, c.limit, r)
	eg.Go(func() (err error) { writeCnt, err = wrPromise(); return })

	err = eg.Wait()
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
