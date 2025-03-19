package uniq

import (
	"context"
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type YDB interface {
	DescribeTable(ctx context.Context, path string, opts ...table.DescribeTableOption) (desc table.Description, err error)
}

func NewScheme(db YDB, params qtool.QueryParams) *Schemes {
	return &Schemes{db: db, qp: params}
}

type Schemes struct {
	db YDB
	qp qtool.QueryParams
}

type HashedPartitions []ydb.Value

func (s *Schemes) GetHashedPartitions(ctx context.Context) (result HashedPartitions, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	path := fmt.Sprintf("%s/%suniques/hashed", s.qp.DB, s.qp.RootPath)
	desc, err := s.db.DescribeTable(ctx, path, table.WithShardKeyBounds())
	if err != nil {
		return nil, fmt.Errorf("describe %s: %w", path, err)
	}

	for _, r := range desc.KeyRanges {
		if r.From != nil {
			result = append(result, r.From)
		}
	}
	return result, nil
}

type HashedSourceBatch struct {
	Records  []HashedSource
	From, To uint64
}

func (h HashedSourceBatch) SplitBy(size int) (result []HashedSourceBatch) {
	for i := 0; i < len(h.Records); i += size {
		end := i + size
		if end > len(h.Records) {
			end = len(h.Records)
		}
		recs := h.Records[i:end]
		result = append(result, HashedSourceBatch{
			Records: recs,
			From:    recs[0].KeyHash,
			To:      recs[len(recs)-1].KeyHash,
		})
	}
	return
}

func SplitHashBatch(borders HashedPartitions, rows []HashedSource) (result []HashedSourceBatch) {
	switch len(rows) {
	case 0:
		return nil
	case 1:
		return []HashedSourceBatch{{
			Records: rows,
			From:    rows[0].KeyHash,
			To:      rows[0].KeyHash,
		}}
	}

	sort.Slice(rows, func(i, j int) bool { return rows[i].KeyHash < rows[j].KeyHash })
	if len(borders) == 0 {
		return []HashedSourceBatch{{
			Records: rows,
			From:    rows[0].KeyHash,
			To:      rows[len(rows)-1].KeyHash,
		}}
	}

	borderInd := len(borders) // value for case all rows in last partition
	start := 0
	startKey := ydb.TupleValue(ydb.Uint64Value(rows[start].KeyHash))
	// Seek first row's partition
	for b := range borders {
		cmp, err := ydb.Compare(startKey, borders[b])
		if err != nil {
			panic(fmt.Errorf("ydb values comparision error: %s", err.Error()))
		}
		if cmp >= 0 {
			continue
		}
		borderInd = b
		break
	}
	if borderInd < len(borders) {
		for i, r := range rows {
			k := ydb.TupleValue(ydb.Uint64Value(r.KeyHash))
			cmp, err := ydb.Compare(k, borders[borderInd])
			if err != nil {
				panic(fmt.Errorf("ydb values comparision error: %s", err.Error()))
			}
			if cmp < 0 {
				continue
			}
			result = append(result, HashedSourceBatch{
				Records: rows[start:i],
				From:    rows[start].KeyHash,
				To:      rows[i-1].KeyHash,
			})
			start = i
			borderInd++
			if borderInd >= len(borders) {
				break
			}
		}
	}
	if start < len(rows) {
		result = append(result, HashedSourceBatch{
			Records: rows[start:],
			From:    rows[start].KeyHash,
			To:      rows[len(rows)-1].KeyHash,
		})
	}
	return
}
