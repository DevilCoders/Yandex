package exporter

import (
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/backup"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/cloudstorage"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/model"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/library/go/core/xerrors"
)

/*
https://st.yandex-team.ru/CLOUD-101721
{
  "cloud_id": "{пользовательское облако}",
  "folder_id": "{пользовательский фолдер}",
  "resource_id": "{cluster_id}",
  "tags": { "cluster_type": "clickhouse_cluster"},
  "id": "3b5baf52-5a41-4b2c-8bc0-61cf88564397",
  "usage": {
    "finish": 1537280286,
    "type": "delta",
    "start": 1537280226,
    "quantity": "{количество байто секунд}",
    "unit": "byte*second"
  },
  "version": "v1alpha1",
  "source_id": "{fqdn машинки с которой отправляется метрика}",
  "schema": "mdb.db.backup.v1",
  "labels": {"key": "value"},
  "source_wt": 1537280286 # время генерации метрики
}
*/

type MetricsFromInvoice func(i invoicer.Invoice, idgen generator.IDGenerator, sourceID string) ([]model.Metric, error)

type BillingExporter struct {
	exporters map[billingdb.BillType]MetricsFromInvoice
	idGen     generator.IDGenerator
	sourceID  string
}

func NewBillingExporter(idGen generator.IDGenerator, sourceID string) BillingExporter {
	return BillingExporter{
		exporters: map[billingdb.BillType]MetricsFromInvoice{
			billingdb.BillTypeBackup:       backup.YCBackupMetricsFromInvoice,
			billingdb.BillTypeCloudStorage: cloudstorage.DCCloudStorageMetricsFromInvoice,
		},
		idGen:    idGen,
		sourceID: sourceID,
	}
}

func (exp *BillingExporter) Export(i invoicer.Invoice) ([]model.Metric, error) {
	if exporter, ok := exp.exporters[i.BillType]; ok {
		return exporter(i, exp.idGen, exp.sourceID)
	}

	return nil, xerrors.Errorf("unknown bill type %s in invoice: %v", i.BillType, i)
}
