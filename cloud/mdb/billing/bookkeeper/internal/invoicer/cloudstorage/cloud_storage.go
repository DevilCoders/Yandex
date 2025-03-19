package cloudstorage

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
)

type CloudStoragePayload struct {
	StorageSpace int64
}

type AWSCloudStorageUsageReportInvoicer struct {
	mdb       metadb.MetaDB
	report    ReportProvider
	threshold int64
	l         log.Logger
}

var _ invoicer.Invoicer = &AWSCloudStorageUsageReportInvoicer{}

func NewAWSUsageReportInvoicer(mdb metadb.MetaDB, report ReportProvider, billingThreshold int64, l log.Logger) *AWSCloudStorageUsageReportInvoicer {
	return &AWSCloudStorageUsageReportInvoicer{
		mdb:       mdb,
		report:    report,
		threshold: billingThreshold,
		l:         l,
	}
}

func (i *AWSCloudStorageUsageReportInvoicer) Invoice(ctx context.Context, clusterID string, fromTS, untilTS time.Time) (invoicer.Invoice, error) {
	details, err := i.mdb.ClickHouseCloudStorageDetails(ctx, clusterID)
	if err != nil {
		i.l.Warnf("cluster %q fetch details: %s", clusterID, err)
		return invoicer.Invoice{}, invoicer.ErrNoMetrics
	}

	ranges := i.report.RangesByResourceID(details.Bucket)
	invoice := invoicer.Invoice{
		Resource: invoicer.Resource{
			ClusterID:          clusterID,
			FolderID:           details.FolderID,
			CloudID:            details.CloudID,
			ClusterType:        metadb.ClickhouseCluster,
			CloudProvider:      details.CloudProvider,
			CloudRegion:        details.CloudRegion,
			ResourcePresetType: details.ResourcePresetType,
		},
		BillType: billingdb.BillTypeCloudStorage,
		FromTS:   fromTS,
		UntilTS:  fromTS,
	}

	for _, r := range ranges {
		if r.FromTS.Before(fromTS) {
			continue
		}

		if invoice.UntilTS.Before(r.UntilTS) {
			invoice.UntilTS = r.UntilTS
		}

		// Send only usage higher than threshold.
		if r.Payload.(*CloudStoragePayload).StorageSpace > i.threshold {
			invoice.Ranges = append(invoice.Ranges, r)
		}
	}

	if len(invoice.Ranges) == 0 {
		return invoicer.Invoice{}, invoicer.ErrNoMetrics
	}

	return invoice, nil
}
