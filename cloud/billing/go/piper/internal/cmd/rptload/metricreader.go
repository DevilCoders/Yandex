package main

import (
	"bufio"
	"compress/gzip"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"
	"sync"
	"time"

	"go.uber.org/atomic"
	"golang.org/x/sync/errgroup"
	"golang.org/x/sync/semaphore"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type metricReader struct {
	logger printer
}

func NewMetricReader(logger printer) *metricReader {
	return &metricReader{
		logger: logger,
	}
}

func (r *metricReader) Run(ctx context.Context, filename string, maxRecords int) (<-chan types.EnrichedQueueMetric, func() (int, decimal.Decimal128, error)) {
	var readCnt int
	var totalCost decimal.Decimal128
	var err error
	output := make(chan types.EnrichedQueueMetric, 1000)
	wg := sync.WaitGroup{}
	wg.Add(1)
	go (func() {
		defer func() { wg.Done() }()
		readCnt, totalCost, err = r.read(ctx, filename, maxRecords, output)
	})()
	return output, func() (int, decimal.Decimal128, error) {
		defer close(output)
		wg.Wait()
		return readCnt, totalCost, err
	}
}

func (r *metricReader) read(ctx context.Context, filename string, maxRecords int, output chan<- types.EnrichedQueueMetric) (int, decimal.Decimal128, error) {
	defer func() {
		r.logger.Println("metricReader done.")
	}()
	r.logger.Println("metricReader started.")

	gzipped := strings.HasSuffix(filename, ".gz")

	f, err := os.Open(filename)
	if err != nil {
		return 0, decimal.Decimal128{}, fmt.Errorf("open file %s error: %w", filename, err)
	}
	defer func() { _ = f.Close() }()

	var rd io.Reader
	rd = bufio.NewReaderSize(f, 10*1024*1024)
	if gzipped {
		gzr, err := gzip.NewReader(rd)
		if err != nil {
			return 0, decimal.Decimal128{}, fmt.Errorf("gzip.NewMetricReader: %w", err)
		}
		defer func() { _ = gzr.Close() }()
		rd = gzr
	}

	reader := bufio.NewReader(rd)
	//decoder := json.NewDecoder(metricReader)

	totalCost := decimal.Decimal128{}
	totalCostMut := sync.Mutex{}
	pCnt := atomic.NewInt32(0)
	rCnt := 0
	started := time.Now()
	parseSem := semaphore.NewWeighted(8)
	parseEg, parseCtx := errgroup.WithContext(ctx)

	for rCnt < maxRecords || maxRecords == 0 {
		select {
		case <-ctx.Done():
			return rCnt, totalCost, ctx.Err()
		default:
		}
		// json.Decoder is slower than easyjson by lines here (25k rps vs 60k rps)
		// err := decoder.Decode(&record)
		line, err := reader.ReadBytes('\n')
		if errors.Is(err, io.EOF) {
			break
		}
		if err != nil {
			return rCnt, totalCost, err
		}
		rCnt++

		err = parseSem.Acquire(ctx, 1)
		if err != nil {
			return rCnt, totalCost, err
		}
		parseEg.Go(func() error {
			defer func() { parseSem.Release(1) }()

			var record types.EnrichedQueueMetric
			err := record.UnmarshalJSON(line)
			if err != nil {
				return err
			}

			select {
			case <-parseCtx.Done():
				return parseCtx.Err()
			default:
			}

			totalCostMut.Lock()
			totalCost = totalCost.Add(decimal.Decimal128(record.Cost))
			totalCostMut.Unlock()

			select {
			case output <- record:
			case <-parseCtx.Done():
				return parseCtx.Err()
			}

			pCnt.Inc()
			c := int(pCnt.Load())
			if c%50000 == 0 || c == maxRecords {
				duration := time.Since(started).Seconds()
				r.logger.Printf("\tRead %d rows (%.2f records per second).\n", c, float64(c)/duration)
			}

			return nil
		})

	}
	err = parseEg.Wait()

	return rCnt, totalCost, err
}
