package simplebackup

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
)

type SimpleBackupInvoicer struct {
	mdb metadb.MetaDB
}

type BackupPayload struct {
	StorageSpace int64
}

var _ invoicer.Invoicer = &SimpleBackupInvoicer{}

func NewSimpleBackupInvoicer(mdb metadb.MetaDB) *SimpleBackupInvoicer {
	return &SimpleBackupInvoicer{
		mdb: mdb,
	}
}

func (bi *SimpleBackupInvoicer) Invoice(ctx context.Context, clusterID string, fromTS, untilTS time.Time) (invoicer.Invoice, error) {
	backups, err := bi.mdb.ListBackups(ctx, clusterID, fromTS, untilTS)
	if err != nil {
		return invoicer.Invoice{}, err
	}
	var usedSpace int64
	for _, b := range backups {
		usedSpace += b.DataSize
	}

	grantedSpace, err := bi.mdb.ClusterSpace(ctx, clusterID)
	if err != nil {
		return invoicer.Invoice{}, err
	}

	cluster, err := bi.mdb.ClusterMeta(ctx, clusterID)
	if err != nil {
		return invoicer.Invoice{}, err
	}
	invoice := invoicer.Invoice{
		Resource: invoicer.Resource{
			ClusterID:   clusterID,
			FolderID:    cluster.FolderID,
			CloudID:     cluster.CloudID,
			ClusterType: cluster.Type,
		},
		FromTS:   fromTS,
		UntilTS:  untilTS,
		BillType: billingdb.BillTypeBackup,
	}

	if usedSpace > grantedSpace {
		invoice.Ranges = append(invoice.Ranges, invoicer.Range{
			FromTS:  fromTS,
			UntilTS: untilTS,
			Payload: BackupPayload{StorageSpace: usedSpace - grantedSpace},
		})
	}

	return invoice, nil
}
