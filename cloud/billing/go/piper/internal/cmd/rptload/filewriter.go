package main

import (
	"bufio"
	"compress/flate"
	"compress/gzip"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"strings"
	"sync"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type fileWriter struct {
	logger printer
}

func NewFileWriter(logger printer) *fileWriter {
	return &fileWriter{
		logger: logger,
	}
}

func (w *fileWriter) Run(ctx context.Context, filename string, input <-chan []reports.ResourceReportsRow) func() (int, error) {
	var writeCnt int
	var err error
	wg := sync.WaitGroup{}
	wg.Add(1)
	go (func() {
		defer func() { wg.Done() }()
		writeCnt, err = w.write(ctx, filename, input)
	})()
	return func() (int, error) {
		wg.Wait()
		return writeCnt, err
	}
}

func (w *fileWriter) write(ctx context.Context, filename string, input <-chan []reports.ResourceReportsRow) (int, error) {
	var writtenTotalCost decimal.Decimal128
	defer func() {
		w.logger.Printf("writer done. writtenTotalCost = %s\n", writtenTotalCost.String())
	}()
	w.logger.Printf("writer started.\n")

	gzipped := strings.HasSuffix(filename, ".gz")

	f, err := os.Create(filename)
	if err != nil {
		return 0, fmt.Errorf("create file %s error: %w", filename, err)
	}
	defer func() { _ = f.Close() }()

	var wr io.Writer
	bwr := bufio.NewWriterSize(f, 1*1024*1024)
	wr = bwr
	defer func() {
		_ = bwr.Flush()
	}()

	if gzipped {
		gzw, err := gzip.NewWriterLevel(wr, flate.BestCompression)
		if err != nil {
			panic(err)
		}
		defer func() { _ = gzw.Close() }()
		wr = gzw
	}

	encoder := json.NewEncoder(wr)

	written := 0
	for {
		select {
		case rows, ok := <-input:
			if !ok {
				return written, nil
			}
			w.logger.Printf("Writing %d records...\n", len(rows))
			rowsJSON := make([]resourceReportsRowJSON, len(rows))
			for i, row := range rows {
				rowsJSON[i] = mapToResourceReportsRowJSON(row)
			}
			err := encoder.Encode(rowsJSON)
			if err != nil {
				return written, fmt.Errorf("error on encoder.Encode: %s", err)
			}
			w.logger.Printf("Written %d records.\n", len(rows))
			for _, r := range rows {
				writtenTotalCost = writtenTotalCost.Add(decimal.Decimal128(r.Cost))
			}
			written += len(rows)
		case <-ctx.Done():
			return written, ctx.Err()
		}
	}
}
