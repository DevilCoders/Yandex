package ydb

import (
	"context"
	"errors"
	"fmt"
	"hash/crc32"
	"sync"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/uniq"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/ctxtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

type UniqSession struct {
	commonSession

	adapter *DataAdapter
}

func (a *DataAdapter) UniqSession() *UniqSession {
	if !a.EnableUniq {
		panic("misconfiguration: uniq sessions disabled in this ydb adapter")
	}
	return &UniqSession{
		adapter: a,
	}
}

func (s *UniqSession) FindDuplicates(
	ctx context.Context, scope entities.ProcessingScope, periodEnd time.Time, ids []entities.MetricIdentity,
) (duplicates []entities.MetricIdentity, err error) {
	if len(ids) == 0 {
		return nil, nil
	}

	// NOTE: Assume ids has been sorted proper way. In case of duplicated records first id will be kept.
	rows := make([]uniq.HashedSource, 0, len(ids))
	found := map[entities.MetricIdentity]bool{}
	hashIdx := map[uint64]int{}
	for i, id := range ids {
		key := id
		key.Offset = 0
		if found[key] {
			duplicates = append(duplicates, id)
			continue
		}
		found[key] = true

		row := s.makeMetricIdentityRow(scope.SourceID, id)
		hashIdx[row.KeyHash] = i

		rows = append(rows, row)
	}

	expireAt := ydbDate(periodEnd)

	dupHashes, warnHashes, err := s.findDuplicatedRows(ctx, expireAt, rows)
	if err != nil {
		return nil, err
	}
	if len(warnHashes) > 0 {
		logger := tooling.Logger(ctx)
		for _, wh := range warnHashes {
			idx, found := hashIdx[wh]
			if !found {
				panic(fmt.Sprintf("inconsistent hash value from db: %d", wh))
			}
			id := ids[idx]
			logger.Warn("metric id hash collision found", logf.Offset(id.Offset), logf.Value(id.Schema), logf.ErrorValue(id.MetricID))
		}
	}
	for _, dh := range dupHashes {
		idx, found := hashIdx[dh]
		if !found {
			panic(fmt.Sprintf("inconsistent hash value from db: %d", dh))
		}
		duplicates = append(duplicates, ids[idx])
	}

	return
}

func (s *UniqSession) findDuplicatedRows(
	ctx context.Context, expireAt time.Time, rows []uniq.HashedSource,
) (dupHashes []uint64, warnHashes []uint64, err error) {
	batches, err := s.adapter.splitUniqByPartitions(ctx, rows)
	if err != nil {
		return nil, nil, err
	}
	results := make([][]uniq.HashedDuplicate, len(batches))

	eg, ctx := errgroup.WithContext(ctx)
	for i := range batches {
		i := i
		eg.Go(func() error {
			batch := batches[i]
			result, err := s.deduplicatePartition(ctx, expireAt, batch)
			if err != nil {
				return fmt.Errorf("duplicates seek in batch %d from %d to %d: %w", i, batch.From, batch.To, err)
			}
			results[i] = result
			return nil
		})
	}
	if err := eg.Wait(); err != nil {
		return nil, nil, err
	}

	csSync := sync.Once{}
	var hashChecksums map[uint64]uint32
	for _, result := range results {
		if len(result) == 0 {
			continue
		}
		csSync.Do(func() {
			hashChecksums = make(map[uint64]uint32, len(rows))
			for _, r := range rows {
				hashChecksums[r.KeyHash] = r.Check
			}
		})
		for _, dup := range result {
			switch dup.Check {
			case hashChecksums[dup.KeyHash]:
				dupHashes = append(dupHashes, dup.KeyHash)
			default:
				warnHashes = append(warnHashes, dup.KeyHash)
			}
		}
	}

	return dupHashes, warnHashes, nil
}

func (s *UniqSession) deduplicatePartition(
	ctx context.Context, expireAt time.Time, batch uniq.HashedSourceBatch,
) (dup []uniq.HashedDuplicate, err error) {
	{ // try to insert in hope all records are new ones
		retryCtx := tooling.StartRetry(ctx)
		queries := uniq.New(s.adapter.db, s.adapter.queryParams)
		err = s.retryWrite(retryCtx, func() error {
			tooling.RetryIteration(retryCtx)
			return queries.InsertHashedRecords(retryCtx, expireAt, batch)
		})
		if err == nil {
			// no duplicates found
			return
		}
		var oe *ydb.OpError
		if !errors.As(err, &oe) || oe.Reason != ydb.StatusPreconditionFailed {
			// query error
			return nil, err
		}
		// assume that precondition failed caused by primary key and fallback to complex path
	}

	{
		// inserting only new records with full batch and get duplicates count back
		retryCtx := tooling.StartRetry(ctx)
		queries := uniq.New(s.adapter.db, s.adapter.queryParams)
		var dupCount int
		err = s.retryWrite(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)
			dupCount, resErr = queries.PushHashedRecords(retryCtx, expireAt, batch)
			return resErr
		})
		if err != nil {
			return nil, err
		}
		if dupCount == 0 {
			// strange, but possible
			tooling.Logger(ctx).Warn("no duplicates found after precondition failed on insert")
			return nil, nil
		}
	}

	// 1000 is default max rows returned by YDB without truncation
	for _, chunk := range batch.SplitBy(1000) {
		retryCtx := tooling.StartRetry(ctx)
		err = s.retryRead(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)

			tx, err := s.adapter.db.BeginTxx(retryCtx, readCommitted())
			if err != nil {
				return err
			}
			defer func() {
				autoTx(tx, resErr)
			}()

			queries := uniq.New(tx, s.adapter.queryParams)

			chunkDups, err := queries.GetHashedDuplicates(ctx, expireAt, chunk)
			dup = append(dup, chunkDups...)
			return err
		})
		if err != nil {
			return nil, err
		}
	}

	return dup, nil
}

func (s *UniqSession) makeMetricIdentityRow(src string, id entities.MetricIdentity) uniq.HashedSource {
	key := []byte(fmt.Sprintf("%s:%s", id.Schema, id.MetricID))
	return uniq.HashedSource{
		KeyHash:    cityhash.Hash64(key),
		SourceHash: cityhash.Hash64([]byte(fmt.Sprintf("%s:%d", src, id.Offset))),
		Check:      crc32.ChecksumIEEE(key),
	}
}

func (a *DataAdapter) splitUniqByPartitions(ctx context.Context, rows []uniq.HashedSource) ([]uniq.HashedSourceBatch, error) {
	partitions, err := a.uniq.getPartitions(ctxtool.WithGlobalCancel(ctx, a.runCtx), a.schemeDB, a.queryParams)
	if err != nil {
		return nil, err
	}
	return uniq.SplitHashBatch(partitions, rows), nil
}

type uniqCache struct {
	baseStore

	partitions uniq.HashedPartitions
}

func (c *uniqCache) getPartitions(ctx context.Context, schemeDB SchemeSessionGetter, qp qtool.QueryParams) (uniq.HashedPartitions, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.consistent() {
		return c.partitions, nil
	}
	if c.expired() {
		c.partitions = nil
	}

	err := c.refreshPartitions(ctx, schemeDB, qp)
	if err == nil {
		c.updated()
	}

	return c.partitions, err
}

func (c *uniqCache) refreshPartitions(ctx context.Context, schemeDB SchemeSessionGetter, qp qtool.QueryParams) error {
	ydb, err := schemeDB.Get(ctx)
	if err != nil {
		return c.selectSentinel(ErrUniqPartitionsExpired, ErrUniqPartitionsRefresh).Wrap(err)
	}
	defer schemeDB.Put(ctx, ydb)
	schemes := uniq.NewScheme(ydb, qp)

	partitions, err := schemes.GetHashedPartitions(ctx)
	if err != nil {
		return c.selectSentinel(ErrUniqPartitionsExpired, ErrUniqPartitionsRefresh).Wrap(err)
	}
	c.partitions = partitions
	return nil
}
