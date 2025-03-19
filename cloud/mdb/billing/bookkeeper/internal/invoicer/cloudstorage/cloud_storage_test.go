package cloudstorage

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	metadbmock "a.yandex-team.ru/cloud/mdb/billing/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/app"
)

func TestInvoice(t *testing.T) {
	l, err := app.DefaultToolLoggerConstructor()(app.DefaultLoggingConfig())
	require.NoError(t, err)

	ctrl := gomock.NewController(l)
	mdb := metadbmock.NewMockMetaDB(ctrl)
	mdb.EXPECT().ClickHouseCloudStorageDetails(gomock.Any(), gomock.Any()).DoAndReturn(func(_ context.Context, cid string) (metadb.ClickHouseCloudStorageDetails, error) {
		return metadb.ClickHouseCloudStorageDetails{
			CloudID:  "cloud1",
			FolderID: "folder1",
			Bucket:   fmt.Sprintf("cloud-storage-%s", cid),
		}, nil
	}).AnyTimes()

	ranges := []invoicer.Range{
		{FromTS: mustTime(t, "2000-01-01T00:00:00Z"), UntilTS: mustTime(t, "2000-01-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 1}},
		{FromTS: mustTime(t, "2000-01-02T00:00:00Z"), UntilTS: mustTime(t, "2000-01-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 2}},
		{FromTS: mustTime(t, "2000-02-01T00:00:00Z"), UntilTS: mustTime(t, "2000-02-01T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 3}},
		{FromTS: mustTime(t, "2000-02-02T00:00:00Z"), UntilTS: mustTime(t, "2000-02-02T01:00:00Z"), Payload: &CloudStoragePayload{StorageSpace: 4}},
	}

	storageInvoicer := NewAWSUsageReportInvoicer(mdb, NewSimpleReport(map[string][]invoicer.Range{
		"cloud-storage-cid1": ranges,
	}), 1, l)

	t.Run("invoice all, except low usage", func(t *testing.T) {
		invoice, err := storageInvoicer.Invoice(context.TODO(), "cid1", mustTime(t, "2000-01-01T00:00:00Z"), mustTime(t, "2000-02-04T01:00:00Z"))
		require.NoError(t, err)
		require.Equal(t, invoicer.Invoice{
			Resource: invoicer.Resource{
				ClusterID:   "cid1",
				FolderID:    "folder1",
				CloudID:     "cloud1",
				ClusterType: metadb.ClickhouseCluster,
			},
			BillType: billingdb.BillTypeCloudStorage,
			FromTS:   mustTime(t, "2000-01-01T00:00:00Z"),
			UntilTS:  mustTime(t, "2000-02-02T01:00:00Z"),
			Ranges:   ranges[1:],
		}, invoice)
	})

	t.Run("invoice only last day", func(t *testing.T) {
		invoice, err := storageInvoicer.Invoice(context.TODO(), "cid1", mustTime(t, "2000-02-01T01:00:00Z"), mustTime(t, "2000-02-04T01:00:00Z"))
		require.NoError(t, err)
		require.Equal(t, invoicer.Invoice{
			Resource: invoicer.Resource{
				ClusterID:   "cid1",
				FolderID:    "folder1",
				CloudID:     "cloud1",
				ClusterType: metadb.ClickhouseCluster,
			},
			BillType: billingdb.BillTypeCloudStorage,
			FromTS:   mustTime(t, "2000-02-01T01:00:00Z"),
			UntilTS:  mustTime(t, "2000-02-02T01:00:00Z"),
			Ranges:   ranges[3:],
		}, invoice)
	})

	t.Run("no new data", func(t *testing.T) {
		invoice, err := storageInvoicer.Invoice(context.TODO(), "cid1", mustTime(t, "2000-02-04T01:00:00Z"), mustTime(t, "2000-02-04T01:00:00Z"))
		require.ErrorIs(t, err, invoicer.ErrNoMetrics)
		require.Equal(t, invoicer.Invoice{}, invoice)

		invoice, err = storageInvoicer.Invoice(context.TODO(), "cid2", mustTime(t, "2000-02-04T01:00:00Z"), mustTime(t, "2000-02-04T01:00:00Z"))
		require.ErrorIs(t, err, invoicer.ErrNoMetrics)
		require.Equal(t, invoicer.Invoice{}, invoice)
	})
}
