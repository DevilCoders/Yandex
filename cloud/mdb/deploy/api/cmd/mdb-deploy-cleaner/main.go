package main

import (
	"context"
	"os"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	_ "a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb/pg" // Load PostgreSQL backend
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var age = 30 * 24 * time.Hour
var limit uint64 = 100
var waitAfterRemove = 30 * time.Second

func init() {
	pflag.DurationVar(&age, "age", age, "age of objects to cleanup")
	pflag.DurationVar(&waitAfterRemove, "wait-after-remove", waitAfterRemove, "time to wait before the next chunk will be removed")
	pflag.Uint64Var(&limit, "limit", limit, "limit of job results, shipments and minions removing at once")
	flags.RegisterConfigPathFlagGlobal()
}

func main() {
	pflag.Parse()
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		os.Exit(1)
	}

	ctx := context.Background()
	// Load db backend
	b, err := deploydb.Open("postgresql", logger)
	if err != nil {
		logger.Fatalf("Failed to open '%s' db: %s", "postgresql", err)
	}
	err = ready.Wait(ctx, b, &ready.DefaultErrorTester{Name: "deploy", L: logger}, time.Second)
	if err != nil {
		logger.Fatal("Failed to wait backend", log.Error(err))
	}

	var totalCount uint64
	for {
		count, err := b.CleanupUnboundJobResults(ctx, age, limit)
		if err != nil {
			logger.Fatal("Failed to cleanup unbound job results", log.Error(err))
		}
		totalCount += count
		if count < limit {
			break
		}
		time.Sleep(waitAfterRemove)
	}
	logger.Info("Clean of unbound job results is completed", log.UInt64("total_count", totalCount))

	totalCount = 0
	for {
		count, err := b.CleanupShipments(ctx, age, limit)
		if err != nil {
			logger.Fatal("Failed to cleanup shipments", log.Error(err))
		}
		totalCount += count
		if count < limit {
			break
		}
		time.Sleep(waitAfterRemove)
	}
	logger.Info("Clean of shipments is completed", log.UInt64("total_count", totalCount))

	totalCount = 0
	for {
		count, err := b.CleanupMinionsWithoutJobs(ctx, age, limit)
		if err != nil {
			logger.Fatal("Failed to cleanup minions without jobs", log.Error(err))
		}
		totalCount += count
		if count < limit {
			break
		}
		time.Sleep(waitAfterRemove)
	}
	logger.Info("Clean of minions without jobs is completed", log.UInt64("total_count", totalCount))
}
