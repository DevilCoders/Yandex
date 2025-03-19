package uniq

import (
	"context"
	"database/sql"
	"errors"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func (q *Queries) GetHashedDuplicates(ctx context.Context, expire time.Time, rows HashedSourceBatch) (result []HashedDuplicate, err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows.Records))
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows.Records {
		param.Add(r)
	}

	query := getHashedDuplicatesQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query,
		sql.Named("values", param.List()),
	)
	err = qtool.WrapWithQuery(err, query)

	return result, err
}

func (q *Queries) PushHashedRecords(ctx context.Context, expire time.Time, rows HashedSourceBatch) (dupCount int, err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows.Records))

	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows.Records {
		param.Add(r)
	}
	var result struct {
		Cnt int `db:"cnt"`
	}

	query := checkHashedDuplicatesQuery.WithParams(q.qp)
	err = q.db.GetContext(ctx, &result, query,
		sql.Named("expire_at", ydb.DateValueFromTime(expire)),
		sql.Named("values", param.List()),
	)
	err = qtool.WrapWithQuery(err, query)

	return result.Cnt, err
}

func (q *Queries) InsertHashedRecords(ctx context.Context, expire time.Time, rows HashedSourceBatch) (err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows.Records))
	defer func() {
		if err == nil {
			tooling.QueryDone(ctx, nil)
			return
		}
		var oe *ydb.OpError
		if errors.As(err, &oe) && oe.Reason == ydb.StatusPreconditionFailed {
			tooling.QueryDone(ctx, nil)
			return
		}
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows.Records {
		param.Add(r)
	}

	query := insertHashedRecordsQuery.WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query,
		sql.Named("expire_at", ydb.DateValueFromTime(expire)),
		sql.Named("values", param.List()),
	)
	return qtool.WrapWithQuery(err, query)
}

func (q *Queries) ForcePushHashedRecords(ctx context.Context, expire time.Time, rows HashedSourceBatch) (err error) {
	ctx = tooling.QueryStarted(ctx)
	tooling.QueryWithRowsCount(ctx, len(rows.Records))
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	param := qtool.ListValues()
	for _, r := range rows.Records {
		param.Add(r)
	}

	query := pushHashedRecordsQuery.WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query,
		sql.Named("expire_at", ydb.DateValueFromTime(expire)),
		sql.Named("values", param.List()),
	)
	return qtool.WrapWithQuery(err, query)
}

type HashedSource struct {
	KeyHash    uint64 `db:"key_hash"`
	SourceHash uint64 `db:"source_hash"`
	Check      uint32 `db:"chk"`
}

type HashedDuplicate struct {
	KeyHash uint64 `db:"key_hash"`
	Check   uint32 `db:"chk"`
}

func (r HashedSource) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("key_hash", ydb.Uint64Value(r.KeyHash)),
		ydb.StructFieldValue("source_hash", ydb.Uint64Value(r.SourceHash)),
		ydb.StructFieldValue("chk", ydb.Uint32Value(r.Check)),
	)
}

var (
	hashedSourceStructType = ydb.Struct(
		ydb.StructField("key_hash", ydb.TypeUint64),
		ydb.StructField("source_hash", ydb.TypeUint64),
		ydb.StructField("chk", ydb.TypeUint32),
	)
	hashedSourceListType = ydb.List(hashedSourceStructType)
	hashedSourceCols     = []qtool.QueryString{
		"key_hash",
		"source_hash",
		"chk",
	}

	getHashedDuplicatesQuery = qtool.Query(
		qtool.Declare("values", hashedSourceListType),

		"SELECT table.key_hash as key_hash, table.chk as chk",
		"FROM as_table($values) as input",
		" INNER JOIN ", qtool.TableAs("uniques/hashed", "table"),
		"ON input.key_hash = table.key_hash",
		"WHERE table.source_hash != input.source_hash",
		";",
	)

	checkHashedDuplicatesQuery = qtool.Query(
		qtool.Declare("expire_at", ydb.TypeDate),
		qtool.Declare("values", hashedSourceListType),

		qtool.NamedQuery("joint",
			"SELECT", qtool.Cols(
				qtool.PrefixedCols("input",
					hashedSourceCols...,
				),
				"table.key_hash is null as new_row",
			),
			"FROM as_table($values) as input",
			" LEFT JOIN ", qtool.TableAs("uniques/hashed", "table"),
			"ON input.key_hash = table.key_hash",
			"WHERE table.key_hash is null or (table.source_hash != input.source_hash)",
		),

		"REPLACE INTO", qtool.Table("uniques/hashed"), "(expire_at,", qtool.Cols(hashedSourceCols...), ")",
		"SELECT $expire_at as expire_at,", qtool.Cols(hashedSourceCols...),
		"FROM $joint",
		"WHERE new_row;",
		";",

		"SELECT count(*) as cnt",
		"FROM $joint",
		"WHERE not new_row",
		";",
	)

	pushHashedRecordsQuery = qtool.Query(
		qtool.Declare("expire_at", ydb.TypeDate),
		qtool.Declare("values", hashedSourceListType),

		"REPLACE INTO", qtool.Table("uniques/hashed"), "(expire_at,", qtool.Cols(hashedSourceCols...), ")",
		"SELECT $expire_at as expire_at,", qtool.Cols(hashedSourceCols...),
		"FROM as_table($values)",
		";",
	)

	insertHashedRecordsQuery = qtool.Query(
		qtool.Declare("expire_at", ydb.TypeDate),
		qtool.Declare("values", hashedSourceListType),

		"INSERT INTO", qtool.Table("uniques/hashed"), "(expire_at,", qtool.Cols(hashedSourceCols...), ")",
		"SELECT $expire_at as expire_at,", qtool.Cols(hashedSourceCols...),
		"FROM as_table($values)",
		";",
	)
)
