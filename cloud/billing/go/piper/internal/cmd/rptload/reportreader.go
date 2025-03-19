package main

import (
	"bufio"
	"compress/gzip"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type reportReader struct {
	logger printer
}

func NewReportReader(logger printer) *reportReader {
	return &reportReader{
		logger: logger,
	}
}

func (r *reportReader) Run(ctx context.Context, filename string) (<-chan []reports.ResourceReportsRow, func() (int, decimal.Decimal128, error)) {
	var readCnt int
	var totalCost decimal.Decimal128
	var err error
	output := make(chan []reports.ResourceReportsRow, 1)
	wg := sync.WaitGroup{}
	wg.Add(1)
	go (func() {
		defer func() { wg.Done() }()
		readCnt, totalCost, err = r.read(ctx, filename, output)
	})()
	return output, func() (int, decimal.Decimal128, error) {
		defer close(output)
		wg.Wait()
		return readCnt, totalCost, err
	}
}

func (r *reportReader) read(ctx context.Context, filename string, output chan<- []reports.ResourceReportsRow) (int, decimal.Decimal128, error) {
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

	decoder := json.NewDecoder(rd)

	totalCost := decimal.Decimal128{}
	cnt := 0
	started := time.Now()

	for {
		select {
		case <-ctx.Done():
			return cnt, totalCost, ctx.Err()
		default:
		}
		rowsJSON := make([]resourceReportsRowJSON, 0, 60000)
		err := decoder.Decode(&rowsJSON)
		if errors.Is(err, io.EOF) {
			break
		}
		if err != nil {
			return cnt, totalCost, err
		}

		rows := make([]reports.ResourceReportsRow, len(rowsJSON))
		for i, row := range rowsJSON {
			rows[i] = mapFromResourceReportsRowJSON(row)
		}

		cnt += len(rows)
		for _, row := range rows {
			totalCost = totalCost.Add(decimal.Decimal128(row.Cost))
		}

		select {
		case output <- rows:
		case <-ctx.Done():
			return cnt, totalCost, ctx.Err()
		}

		duration := time.Since(started).Seconds()
		r.logger.Printf("\tRead %d rows (%.2f records per second).\n", cnt, float64(cnt)/duration)
	}

	return cnt, totalCost, nil
}
