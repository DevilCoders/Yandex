package cloudstorage

import (
	"compress/gzip"
	"context"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"path"
	"strconv"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ReportProvider interface {
	RangesByResourceID(id string) []invoicer.Range
}

type SimpleReport struct {
	ranges map[string][]invoicer.Range
}

var _ ReportProvider = &SimpleReport{}

func NewSimpleReport(ranges map[string][]invoicer.Range) *SimpleReport {
	return &SimpleReport{ranges: ranges}
}

func (r *SimpleReport) RangesByResourceID(id string) []invoicer.Range {
	return r.ranges[id]
}

type S3ReportConfig struct {
	Bucket      string `json:"bucket" yaml:"bucket"`
	PathPrefix  string `json:"path_prefix" yaml:"path_prefix"`
	ReportName  string `json:"report_name" yaml:"report_name"`
	ServiceCode string `json:"service_code" yaml:"service_code"`
	Operation   string `json:"operation" yaml:"operation"`
}

func DefaultS3ReportConfig() S3ReportConfig {
	return S3ReportConfig{
		ServiceCode: "AmazonS3",
		Operation:   "StandardStorage",
	}
}

const (
	timeScheme   = "20060102"
	manifestName = "%s-Manifest.json"
)

type S3Report struct {
	SimpleReport
	s3client s3.Client
	config   S3ReportConfig
	l        log.Logger
}

var _ ReportProvider = &S3Report{}

func NewS3Report(ctx context.Context, config S3ReportConfig, client s3.Client, l log.Logger, until time.Time) (*S3Report, error) {
	report := &S3Report{
		s3client: client,
		config:   config,
		l:        l,
	}

	return report, report.initialize(ctx, until)
}

func (r *S3Report) initialize(ctx context.Context, until time.Time) error {
	r.l.Info("initializing report invoicer")

	delimiter := "/"
	prefix := path.Join(r.config.PathPrefix, r.config.ReportName) + "/"
	r.SimpleReport.ranges = map[string][]invoicer.Range{}

	_, paths, err := r.s3client.ListObjects(ctx, r.config.Bucket, s3.ListObjectsOpts{
		Prefix:    &prefix,
		Delimiter: &delimiter,
	})
	if err != nil {
		return xerrors.Errorf("list report intervals: %s", err.Error())
	}

	if len(paths) == 0 {
		return xerrors.Errorf("no billing reports found")
	}

	for _, interval := range paths {
		parts := strings.Split(path.Base(interval.Prefix), "-")
		if len(parts) != 2 {
			r.l.Warnf("invalid report interval format: %q", interval.Prefix)
			continue
		}

		intervalEnd, err := time.Parse(timeScheme, parts[1])
		if err != nil {
			r.l.Warnf("invalid report interval format: %q: %s", interval.Prefix, err)
			continue
		}

		if intervalEnd.Add(time.Hour * 24 * 30).After(until) {
			if err := r.processReport(ctx, interval.Prefix); err != nil {
				r.l.Errorf("process report for %q: %s", interval.Prefix, err)
			}
		}
	}

	return nil
}

func (r *S3Report) processReport(ctx context.Context, interval string) error {
	manifest, err := r.s3client.GetObject(ctx, r.config.Bucket, path.Join(
		interval,
		fmt.Sprintf(manifestName, r.config.ReportName)),
	)
	if err != nil {
		return xerrors.Errorf("get report %q manifest: %s", interval, err.Error())
	}
	defer manifest.Close()

	var manifestData struct {
		ReportKeys []string `json:"reportKeys"`
	}
	if err := json.NewDecoder(manifest).Decode(&manifestData); err != nil {
		return err
	}

	for _, key := range manifestData.ReportKeys {
		if err := r.processReportPart(ctx, key); err != nil {
			r.l.Error(err.Error())
		}
	}

	return nil
}

func (r *S3Report) processReportPart(ctx context.Context, key string) error {
	r.l.Infof("loading report part %q", key)
	objectStream, err := r.s3client.GetObject(ctx, r.config.Bucket, key)
	if err != nil {
		return xerrors.Errorf("get report part %q: %s", key, err)
	}
	defer objectStream.Close()

	gzipReader, err := gzip.NewReader(objectStream)
	if err != nil {
		return xerrors.Errorf("decode gzip %q: %s", key, err)
	}

	csvReader := csv.NewReader(gzipReader)
	csvReader.ReuseRecord = true
	csvReader.LazyQuotes = true
	schema, err := schemaFromHeader(csvReader)
	if err != nil {
		return xerrors.Errorf("parse schema %q: %s", key, err)
	}

	var row []string
	for err == nil {
		row, err = csvReader.Read()
		if err == io.EOF {
			continue
		}

		if err != nil {
			return xerrors.Errorf("read next row: %s", err.Error())
		}

		if row[schema.ServiceIndex] == r.config.ServiceCode && row[schema.OperationIndex] == r.config.Operation {
			invoiceRange, err := rangeFromRow(schema, row)
			if err != nil {
				return xerrors.Errorf("parse row: %s", err.Error())
			}
			r.SimpleReport.ranges[row[schema.ResourceID]] = append(r.SimpleReport.ranges[row[schema.ResourceID]], invoiceRange)
		}
	}

	return nil
}

func rangeFromRow(schema dataSchema, data []string) (invoicer.Range, error) {
	var err error
	var result invoicer.Range

	result.FromTS, err = time.Parse(time.RFC3339, data[schema.UsageStartIndex])
	if err != nil {
		return invoicer.Range{}, xerrors.Errorf("parse usage start: %s", err.Error())
	}

	result.UntilTS, err = time.Parse(time.RFC3339, data[schema.UsageEndIndex])
	if err != nil {
		return invoicer.Range{}, xerrors.Errorf("parse usage end: %s", err.Error())
	}

	usage, err := strconv.ParseFloat(data[schema.UsageIndex], 64)
	if err != nil {
		return invoicer.Range{}, xerrors.Errorf("parse usage: %s", err.Error())
	}

	billStart, err := time.Parse(time.RFC3339, data[schema.BillingStartIndex])
	if err != nil {
		return invoicer.Range{}, xerrors.Errorf("parse bill start: %s", err.Error())
	}

	billEnd, err := time.Parse(time.RFC3339, data[schema.BillingEndIndex])
	if err != nil {
		return invoicer.Range{}, xerrors.Errorf("parse bill end: %s", err.Error())
	}

	result.Payload = &CloudStoragePayload{
		StorageSpace: int64(1024 * usage * billEnd.Sub(billStart).Hours()),
	}

	return result, nil
}

type dataSchema struct {
	ServiceIndex      int
	OperationIndex    int
	ResourceID        int
	UsageIndex        int
	UsageStartIndex   int
	UsageEndIndex     int
	BillingStartIndex int
	BillingEndIndex   int
}

func schemaFromHeader(csvReader *csv.Reader) (dataSchema, error) {
	row, err := csvReader.Read()
	if err != nil {
		return dataSchema{}, err
	}

	var result dataSchema

	for index, name := range row {
		switch name {
		case "lineItem/UsageAmount":
			result.UsageIndex = index
		case "product/servicecode":
			result.ServiceIndex = index
		case "lineItem/ResourceId":
			result.ResourceID = index
		case "lineItem/Operation":
			result.OperationIndex = index
		case "lineItem/UsageStartDate":
			result.UsageStartIndex = index
		case "lineItem/UsageEndDate":
			result.UsageEndIndex = index
		case "bill/BillingPeriodEndDate":
			result.BillingEndIndex = index
		case "bill/BillingPeriodStartDate":
			result.BillingStartIndex = index
		}
	}
	return result, nil
}
