package billingdb

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh BillingDB

var (
	ErrDataNotFound = xerrors.NewSentinel("data not found")
	ErrLockNotTaken = xerrors.NewSentinel("lock not taken")
)

type BillType string

const (
	BillTypeBackup       = BillType("BACKUP")
	BillTypeCloudStorage = BillType("CH_CLOUD_STORAGE")
)

var billTypeMap = map[BillType]struct{}{
	BillTypeBackup:       {},
	BillTypeCloudStorage: {},
}

// ParseBillType builds BillType from string
func ParseBillType(str string) (BillType, error) {
	ct := BillType(str)
	if _, ok := billTypeMap[ct]; !ok {
		return "", xerrors.Errorf("unknown bill type: %s", ct)
	}
	return ct, nil
}

type Track struct {
	ClusterID   string
	ClusterType metadb.ClusterType
	BillType    BillType
	FromTS      time.Time
	UntilTS     optional.Time
}

type Metrics interface {
	Marshal() ([]byte, error)
	ID() string
}

type Batch struct {
	ID       string
	SeqNo    int64
	Restarts int64
	Data     []byte
}

// BillingDB ...
type BillingDB interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	Tracks(ctx context.Context, billType BillType) ([]Track, error)
	UpdateClusterTrack(ctx context.Context, cid string, ts time.Time, billType BillType) error
	EnqueueMetrics(ctx context.Context, metrics Metrics, billType BillType) error
	DequeueBatch(ctx context.Context, billType BillType) (Batch, error)
	CompleteBatch(ctx context.Context, batchID string, startedTS, finishedTS time.Time) error
	PostponeBatch(ctx context.Context, batchID string) error
	TryGetLock(ctx context.Context, billType BillType) error
}
