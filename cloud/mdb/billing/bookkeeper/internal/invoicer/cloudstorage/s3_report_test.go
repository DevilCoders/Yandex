package cloudstorage

import (
	"bytes"
	"compress/gzip"
	"context"
	"io"
	"io/ioutil"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	s3mock "a.yandex-team.ru/cloud/mdb/internal/s3/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestS3ReportLoading(t *testing.T) {
	l, err := app.DefaultToolLoggerConstructor()(app.DefaultLoggingConfig())
	require.NoError(t, err)
	ctrl := gomock.NewController(t)
	s3client := newS3Client(t, ctrl)
	config := S3ReportConfig{
		PathPrefix:  "report",
		ReportName:  "report",
		ServiceCode: "AmazonS3",
		Operation:   "StandardStorage",
	}

	t.Run("Read all data", func(t *testing.T) {
		report, err := NewS3Report(context.TODO(), config, s3client, l, mustTime(t, "2000-02-04T01:00:00Z"))
		require.NoError(t, err)
		require.Equal(t, []invoicer.Range{
			{FromTS: mustTime(t, "2000-01-01T00:00:00Z"), UntilTS: mustTime(t, "2000-01-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 1024 * 24}},
			{FromTS: mustTime(t, "2000-02-01T00:00:00Z"), UntilTS: mustTime(t, "2000-02-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 24}},
			{FromTS: mustTime(t, "2000-02-02T00:00:00Z"), UntilTS: mustTime(t, "2000-02-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 16 * 24}},
		}, report.RangesByResourceID("cloud-storage-cid1"))
		require.Equal(t, []invoicer.Range{
			{FromTS: mustTime(t, "2000-01-01T00:00:00Z"), UntilTS: mustTime(t, "2000-01-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 1024 * 24}},
			{FromTS: mustTime(t, "2000-02-01T00:00:00Z"), UntilTS: mustTime(t, "2000-02-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 24}},
			{FromTS: mustTime(t, "2000-02-02T00:00:00Z"), UntilTS: mustTime(t, "2000-02-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 16 * 24}},
		}, report.RangesByResourceID("cloud-storage-cid2"))
		require.Equal(t, []invoicer.Range(nil), report.RangesByResourceID("cloud-storage-cid3"))
	})

	t.Run("Read only last month", func(t *testing.T) {
		report, err := NewS3Report(context.TODO(), config, s3client, l, mustTime(t, "2000-03-04T01:00:00Z"))
		require.NoError(t, err)
		require.Equal(t, []invoicer.Range{
			{FromTS: mustTime(t, "2000-02-01T00:00:00Z"), UntilTS: mustTime(t, "2000-02-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 24}},
			{FromTS: mustTime(t, "2000-02-02T00:00:00Z"), UntilTS: mustTime(t, "2000-02-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 16 * 24}},
		}, report.RangesByResourceID("cloud-storage-cid1"))
		require.Equal(t, []invoicer.Range{
			{FromTS: mustTime(t, "2000-02-01T00:00:00Z"), UntilTS: mustTime(t, "2000-02-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 24}},
			{FromTS: mustTime(t, "2000-02-02T00:00:00Z"), UntilTS: mustTime(t, "2000-02-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 16 * 24}},
		}, report.RangesByResourceID("cloud-storage-cid2"))
		require.Equal(t, []invoicer.Range(nil), report.RangesByResourceID("cloud-storage-cid3"))
	})
}

const (
	dataHeader = "product/servicecode,lineItem/Operation,lineItem/ResourceId,bill/BillingPeriodStartDate,bill/BillingPeriodEndDate,lineItem/UsageStartDate,lineItem/UsageEndDate,lineItem/UsageAmount\n"
	oldData    = dataHeader +
		"AmazonS3,StandardStorage,cloud-storage-cid1,2000-01-01T00:00:00Z,2000-02-01T00:00:00Z,2000-01-01T00:00:00Z,2000-01-01T01:00:00Z,0.0322580655\n" +
		"AmazonS3,StandardStorage,cloud-storage-cid2,2000-01-01T00:00:00Z,2000-02-01T00:00:00Z,2000-01-01T00:00:00Z,2000-01-01T01:00:00Z,0.0322580655\n"
	newDataP1 = dataHeader +
		"AmazonS3,StandardStorage,cloud-storage-cid1,2000-02-01T00:00:00Z,2000-03-01T00:00:00Z,2000-02-01T00:00:00Z,2000-02-01T01:00:00Z,0.0000336746\n" +
		"AmazonS3,StandardStorage,cloud-storage-cid2,2000-02-01T00:00:00Z,2000-03-01T00:00:00Z,2000-02-01T00:00:00Z,2000-02-01T01:00:00Z,0.0000336746\n"
	newDataP2 = dataHeader +
		"AmazonS3,StandardStorage,cloud-storage-cid1,2000-02-01T00:00:00Z,2000-03-01T00:00:00Z,2000-02-02T00:00:00Z,2000-02-02T01:00:00Z,0.0005387932\n" +
		"AmazonS3,StandardStorage,cloud-storage-cid2,2000-02-01T00:00:00Z,2000-03-01T00:00:00Z,2000-02-02T00:00:00Z,2000-02-02T01:00:00Z,0.0005387932\n"
)

func encodeData(t *testing.T, data string) io.ReadCloser {
	buffer := bytes.NewBuffer(nil)
	writer := gzip.NewWriter(buffer)
	_, err := writer.Write([]byte(data))
	require.NoError(t, err)
	require.NoError(t, writer.Close())
	return ioutil.NopCloser(buffer)
}

func newS3Client(t *testing.T, ctrl *gomock.Controller) *s3mock.MockClient {
	s3Client := s3mock.NewMockClient(ctrl)
	s3Client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, _ string, opts s3.ListObjectsOpts) ([]s3.Object, []s3.Prefix, error) {
			if *opts.Prefix == "report/report/" {
				return nil, []s3.Prefix{
					{Prefix: "report/report/20000101-20000201"},
					{Prefix: "report/report/20000201-20000301"},
				}, nil
			}
			t.Errorf("unexpected list path %q", *opts.Prefix)
			return nil, nil, nil
		}).AnyTimes()
	s3Client.EXPECT().GetObject(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
		func(_ context.Context, _, key string) (io.ReadCloser, error) {
			switch key {
			case "report/report/20000101-20000201/report-Manifest.json":
				return ioutil.NopCloser(bytes.NewReader([]byte("{\"reportKeys\":[\"report/report/20000101-20000201/ver/part1.csv.gz\"]}"))), nil
			case "report/report/20000201-20000301/report-Manifest.json":
				return ioutil.NopCloser(bytes.NewReader([]byte("{\"reportKeys\":[\"report/report/20000201-20000301/ver/part1.csv.gz\",\"report/report/20000201-20000301/ver/part2.csv.gz\"]}"))), nil
			case "report/report/20000101-20000201/ver/part1.csv.gz":
				return encodeData(t, oldData), nil
			case "report/report/20000201-20000301/ver/part1.csv.gz":
				return encodeData(t, newDataP1), nil
			case "report/report/20000201-20000301/ver/part2.csv.gz":
				return encodeData(t, newDataP2), nil
			default:
				return nil, xerrors.Errorf("invalid key %q", key)
			}
		},
	).AnyTimes()

	return s3Client
}

func mustTime(t *testing.T, data string) time.Time {
	res, err := time.Parse(time.RFC3339, data)
	require.NoError(t, err)
	return res
}
