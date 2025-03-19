package invoicer

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/timeutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrNoMetrics = xerrors.NewSentinel("no metrics for cluster generated")
)

type Invoice struct {
	Resource Resource
	BillType billingdb.BillType
	FromTS   time.Time
	UntilTS  time.Time
	Ranges   []Range
}

type Resource struct {
	ClusterID          string
	FolderID           string
	CloudID            string
	ClusterType        metadb.ClusterType
	CloudProvider      string
	CloudRegion        string
	ResourcePresetType string
}

type Range struct {
	FromTS  time.Time
	UntilTS time.Time
	Payload interface{}
}

type Invoicer interface {
	Invoice(ctx context.Context, clusterID string, from, until time.Time) (Invoice, error)
}

func ChunkInvoices(invoices []Invoice, chunkSize int) [][]Invoice {
	if chunkSize < 1 {
		chunkSize = 1
	}
	var chunks [][]Invoice
	for {
		if len(invoices) == 0 {
			break
		}
		if len(invoices) < chunkSize {
			chunkSize = len(invoices)
		}
		chunks = append(chunks, invoices[0:chunkSize])
		invoices = invoices[chunkSize:]
	}
	return chunks
}

func SplitRangeByHours(r Range) []Range {
	var ranges []Range
	ts := timeutil.SplitHours(r.FromTS, r.UntilTS)
	for i := 0; i < len(ts)-1; i++ {
		ranges = append(ranges, Range{
			FromTS:  ts[i],
			UntilTS: ts[i+1],
			Payload: r.Payload,
		})
	}
	return ranges
}
