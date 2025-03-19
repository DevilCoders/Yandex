package pg

import (
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
)

const (
	// DBName is billingDB database name in PostgreSQL
	DBName = "billingdb"
)

type trackRow struct {
	ClusterID   string             `db:"cluster_id"`
	ClusterType metadb.ClusterType `db:"cluster_type"`
	BillType    billingdb.BillType `db:"bill_type"`
	FromTS      time.Time          `db:"from_ts"`
	UntilTS     sql.NullTime       `db:"until_ts"`
}

func trackFromDB(r trackRow) billingdb.Track {
	ret := billingdb.Track{
		ClusterID:   r.ClusterID,
		ClusterType: r.ClusterType,
		BillType:    r.BillType,
		FromTS:      r.FromTS,
	}
	if r.UntilTS.Valid {
		ret.UntilTS.Set(r.UntilTS.Time)
	}

	return ret
}

type metricsBatchRow struct {
	BatchID      string    `db:"batch_id"`
	CreatedAT    time.Time `db:"created_at"`
	SeqNo        int64     `db:"seq_no"`
	RestartCount int64     `db:"restart_count"`
	Batch        []byte    `db:"batch"`
}

func metricsBatchFromDB(r metricsBatchRow) billingdb.Batch {
	return billingdb.Batch{
		ID: r.BatchID, SeqNo: r.SeqNo, Restarts: r.RestartCount, Data: r.Batch,
	}
}
