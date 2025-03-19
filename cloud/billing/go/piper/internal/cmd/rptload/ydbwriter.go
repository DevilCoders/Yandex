package main

import (
	"context"
	"fmt"
	"sync"
	"time"

	"go.uber.org/atomic"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

type ShardingAlg string

const (
	ShardingAlgNone  ShardingAlg = "none"
	ShardingAlgHash  ShardingAlg = "hash"
	ShardingAlgRange ShardingAlg = "range"
	ShardingAlgYdb   ShardingAlg = "ydb"
)

type YDB interface {
	DescribeTable(ctx context.Context, path string, opts ...table.DescribeTableOption) (desc table.Description, err error)
}

type ydbWriter struct {
	logger              printer
	queries             reports.Queries
	inFlyKeys           map[reports.ResourceReportsRowKey]<-chan struct{}
	inFlyMu             sync.Mutex
	shardingAlg         ShardingAlg
	ydbSession          YDB
	dbPath              string
	partitions          []table.KeyRange
	partitionsMu        sync.Mutex
	partitionsUpdatedAt time.Time
}

const partitionsInfoExpiration = time.Second * 20

func NewYdbWriter(logger printer, queries reports.Queries, ydbSession YDB, dbPath string, shardingAlg ShardingAlg) *ydbWriter {
	return &ydbWriter{
		logger:      logger,
		queries:     queries,
		inFlyKeys:   make(map[reports.ResourceReportsRowKey]<-chan struct{}),
		shardingAlg: shardingAlg,
		ydbSession:  ydbSession,
		dbPath:      dbPath,
	}
}

func (w *ydbWriter) Run(ctx context.Context, workers int, limit int, input <-chan []reports.ResourceReportsRow) func() (int, error) {
	switch w.shardingAlg {
	case ShardingAlgNone:
		return w.runNotSharded(ctx, workers, limit, input)
	default:
		return w.runSharded(ctx, workers, limit, input)
	}
}

func (w *ydbWriter) runNotSharded(ctx context.Context, workers int, limit int, input <-chan []reports.ResourceReportsRow) func() (int, error) {
	writeCnt := atomic.NewInt32(0)
	eg, ctx := errgroup.WithContext(ctx)
	for i := 0; i < workers; i++ {
		i := i
		eg.Go(func() error {
			cnt, err := w.write(ctx, i, limit, input)
			writeCnt.Add(int32(cnt))
			return err
		})
	}
	return func() (int, error) {
		err := eg.Wait()
		return int(writeCnt.Load()), err
	}
}

func (w *ydbWriter) runSharded(ctx context.Context, workers int, limit int, input <-chan []reports.ResourceReportsRow) func() (int, error) {
	shardedInput := make([]chan []reports.ResourceReportsRow, workers)
	for i := 0; i < workers; i++ {
		shardedInput[i] = make(chan []reports.ResourceReportsRow, 10)
	}

	writeCnt := atomic.NewInt32(0)
	eg, ctx := errgroup.WithContext(ctx)

	eg.Go(func() error {
		for {
			select {
			case batch, ok := <-input:
				if !ok {
					for i := 0; i < workers; i++ {
						close(shardedInput[i])
					}
					return nil
				}
				sharded, err := w.shard(ctx, batch, workers)
				if err != nil {
					return err
				}
				for i, rows := range sharded {
					select {
					case shardedInput[i] <- rows:
					case <-ctx.Done():
						return ctx.Err()
					}
				}
			case <-ctx.Done():
				return ctx.Err()
			}
		}
	})

	for i := 0; i < workers; i++ {
		i := i
		eg.Go(func() error {
			cnt, err := w.write(ctx, i, limit, shardedInput[i])
			writeCnt.Add(int32(cnt))
			return err
		})
	}

	return func() (int, error) {
		err := eg.Wait()
		return int(writeCnt.Load()), err
	}
}

func (w *ydbWriter) shard(ctx context.Context, batch []reports.ResourceReportsRow, shards int) ([][]reports.ResourceReportsRow, error) {
	result := make([][]reports.ResourceReportsRow, shards)
	for i := 0; i < shards; i++ {
		result[i] = make([]reports.ResourceReportsRow, 0, int(1.1*float32(len(batch))/float32(shards)+1))
	}

	var err error
	for _, row := range batch {
		var n int
		switch w.shardingAlg {
		case ShardingAlgHash:
			n = w.shardByHash(row.BillingAccountID, shards)
		case ShardingAlgRange:
			n = w.shardByRange(row.BillingAccountID, shards)
		case ShardingAlgYdb:
			n, err = w.shardByYdb(ctx, row, shards)
			if err != nil {
				return nil, err
			}
		default:
			panic(fmt.Sprintf("unknown sharding algorithm: %s", w.shardingAlg))
		}
		result[n] = append(result[n], row)
	}

	return result, nil
}

func (w *ydbWriter) shardByHash(billingAccountID string, shards int) int {
	keyHash := cityhash.Hash64([]byte(billingAccountID))
	return int(keyHash % uint64(shards))
}

func (w *ydbWriter) shardByRange(billingAccountID string, shards int) int {
	v := 0
	const base = int('z' - '0' + 1)
	for _, c := range billingAccountID[3:5] {
		v = v*base + int(c-'0')
	}
	n := v * shards / (base * base)
	if n >= shards {
		panic(fmt.Sprintf("shardByRange failed, n=%d, v=%d, base=%d, ba=%s", n, v, base, billingAccountID))
	}
	return n
}

func (w *ydbWriter) shardByYdb(ctx context.Context, row reports.ResourceReportsRow, shards int) (int, error) {
	partitions, err := w.getTablePartitions(ctx)
	if err != nil {
		return 0, err
	}
	if len(partitions) == 0 {
		return w.shardByRange(row.BillingAccountID, shards), nil
	}

	k := row.YDBKeyTuple()
	for i, p := range partitions {
		if p.To != nil {
			cmp, err := ydb.Compare(k, p.To)
			if err != nil {
				panic(fmt.Errorf("ydb values comparision error: %s", err.Error()))
			}
			if cmp > 0 && i < len(partitions)-1 {
				continue
			}
		}
		n := i * shards / len(partitions)
		return n, nil
	}

	return w.shardByRange(row.BillingAccountID, shards), nil
}

func (w *ydbWriter) getTablePartitions(ctx context.Context) ([]table.KeyRange, error) {
	w.partitionsMu.Lock()
	defer func() { w.partitionsMu.Unlock() }()

	if time.Since(w.partitionsUpdatedAt) > partitionsInfoExpiration {
		var err error
		w.partitions, err = w.fetchTablePartitions(ctx)
		if err != nil {
			return nil, err
		}
		w.partitionsUpdatedAt = time.Now()
	}

	return w.partitions, nil
}

func (w *ydbWriter) fetchTablePartitions(ctx context.Context) (result []table.KeyRange, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	path := fmt.Sprintf("/%s/rptload/reports/realtime/resource_reports", w.dbPath)
	desc, err := w.ydbSession.DescribeTable(ctx, path, table.WithShardKeyBounds())
	if err != nil {
		return nil, fmt.Errorf("describe %s: %w", path, err)
	}

	return desc.KeyRanges, nil
}

func (w *ydbWriter) write(ctx context.Context, nWorker, limit int, input <-chan []reports.ResourceReportsRow) (int, error) {
	var writtenTotalCost decimal.Decimal128
	defer func() {
		w.logger.Printf("[w%d] writer done. writtenTotalCost = %s\n", nWorker, writtenTotalCost.String())
	}()
	w.logger.Printf("[w%d] writer started.\n", nWorker)

	written := 0
	for {
		select {
		case rows, ok := <-input:
			if !ok {
				return written, nil
			}
			w.logger.Printf("[w%d] Got %d records for write\n", nWorker, len(rows))
			for len(rows) > 0 {
				partSize := len(rows)
				if partSize > limit {
					partSize = limit
				}
				w.logger.Printf("[w%d] Writing %d records...\n", nWorker, partSize)
				err := w.pushBatch(ctx, nWorker, rows[:partSize])
				if err != nil {
					return written, fmt.Errorf("error on queries.PushResourceReports: %s", err)
				}
				w.logger.Printf("[w%d] Written %d records.\n", nWorker, partSize)
				for i := 0; i < partSize; i++ {
					writtenTotalCost = writtenTotalCost.Add(decimal.Decimal128(rows[i].Cost))
				}
				written += partSize
				rows = rows[partSize:]
			}
		case <-ctx.Done():
			return written, ctx.Err()
		}
	}
}

func (w *ydbWriter) pushBatch(ctx context.Context, nWorker int, rows []reports.ResourceReportsRow) error {
	// Чтобы избежать "Transaction Lock Invalidated", разделим пачку на две.
	// В одной пачке будут записи с ключами, которые не обрабатываются в это же время в других горутинах,
	// а во второй — записи с уже обрабатывающимися ключами.
	rowsGreen := make([]reports.ResourceReportsRow, 0, len(rows))
	rowsRed := make([]reports.ResourceReportsRow, 0)
	writeDone := make(chan struct{})
	redReadiness := make(map[<-chan struct{}]struct{})

	func() {
		w.inFlyMu.Lock()
		defer func() { w.inFlyMu.Unlock() }()

		for _, r := range rows {
			key := reports.ResourceReportsRowKey{
				BillingAccountID: r.BillingAccountID,
				Date:             r.Date,
				CloudID:          r.CloudID,
				SkuID:            r.SkuID,
				FolderID:         r.FolderID,
				ResourceID:       r.ResourceID,
				LabelsHash:       r.LabelsHash,
			}

			if readiness, exists := w.inFlyKeys[key]; !exists {
				rowsGreen = append(rowsGreen, r)
				w.inFlyKeys[key] = writeDone
			} else {
				rowsRed = append(rowsRed, r)
				redReadiness[readiness] = struct{}{}
			}
		}
	}()

	w.logger.Printf("\t[w%d] Have %d green records, and %d red rows\n", nWorker, len(rowsGreen), len(rowsRed))
	if len(rowsGreen) > 0 {
		updatedAt := qtool.UInt64Ts(time.Now())
		err := w.queries.PushResourceReports(ctx, updatedAt, rowsGreen...)
		w.logger.Printf("\t[w%d] PushResourceReports finished.\n", nWorker)
		func() {
			w.inFlyMu.Lock()
			defer func() { w.inFlyMu.Unlock() }()

			for _, r := range rowsGreen {
				key := reports.ResourceReportsRowKey{
					BillingAccountID: r.BillingAccountID,
					Date:             r.Date,
					CloudID:          r.CloudID,
					SkuID:            r.SkuID,
					FolderID:         r.FolderID,
					ResourceID:       r.ResourceID,
					LabelsHash:       r.LabelsHash,
				}

				delete(w.inFlyKeys, key)
			}
		}()
		close(writeDone)
		if err != nil {
			return err
		}
		w.logger.Printf("\t[w%d] Written %d green records\n", nWorker, len(rowsGreen))
	}

	if len(rowsRed) > 0 {
		for readiness := range redReadiness {
			select {
			case <-readiness:
			case <-ctx.Done():
				return ctx.Err()
			}
		}

		return w.pushBatch(ctx, nWorker, rowsRed)
	}
	return nil
}
