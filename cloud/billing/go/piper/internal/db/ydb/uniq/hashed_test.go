package uniq

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type hashTestSuite struct {
	baseSuite
}

func TestHash(t *testing.T) {
	suite.Run(t, new(hashTestSuite))
}

func (suite *hashTestSuite) TestTableCompliance() {
	dupCnt, err := suite.queries.PushHashedRecords(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().NoError(err)
	suite.Require().Zero(dupCnt)
	dups, err := suite.queries.GetHashedDuplicates(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().NoError(err)
	suite.Require().Empty(dups)

	rows, err := suite.queries.getAllHashed(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(rows, 1)
}

func (suite *hashTestSuite) TestTableInsertCompliance() {
	err := suite.queries.InsertHashedRecords(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().NoError(err)

	rows, err := suite.queries.getAllHashed(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(rows, 1)
}

func (suite *hashTestSuite) TestInsertError() {
	err := suite.queries.InsertHashedRecords(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().NoError(err)
	err = suite.queries.InsertHashedRecords(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().Error(err)

	opErr := &ydb.OpError{}
	suite.Require().True(errors.As(err, &opErr))
	suite.Require().Equal(ydb.StatusPreconditionFailed, opErr.Reason)
}

func (suite *hashTestSuite) TestTableForcePushCompliance() {
	err := suite.queries.ForcePushHashedRecords(suite.ctx, time.Now(), HashedSourceBatch{Records: []HashedSource{{}}})
	suite.Require().NoError(err)

	rows, err := suite.queries.getAllHashed(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(rows, 1)
}

func (suite *hashTestSuite) TestPushRows() {
	expire := time.Now().Add(time.Hour * 24).Truncate(time.Hour * 24).UTC()
	metrics := HashedSourceBatch{
		Records: []HashedSource{
			{
				KeyHash:    1,
				SourceHash: 10,
				Check:      100,
			},
			{
				KeyHash:    2,
				SourceHash: 20,
				Check:      200,
			},
		},
	}

	dupCnt, err := suite.queries.PushHashedRecords(suite.ctx, time.Time(expire), metrics)
	suite.Require().NoError(err)
	suite.Require().Zero(dupCnt)

	want := []HashedSourceRow{}
	for _, m := range metrics.Records {
		want = append(want, HashedSourceRow{
			KeyHash:    m.KeyHash,
			SourceHash: m.SourceHash,
			Check:      m.Check,
			ExpireAt:   qtool.Date(expire),
		})
	}

	got, err := suite.queries.getAllHashed(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().EqualValues(want, got)
}

func (suite *hashTestSuite) TestDuplicatesRows() {
	expire := time.Now().Add(time.Hour * 24).Truncate(time.Hour * 24).UTC()
	metrics := HashedSourceBatch{
		Records: []HashedSource{
			{
				KeyHash:    1,
				SourceHash: 10,
				Check:      100,
			},
			{
				KeyHash:    2,
				SourceHash: 20,
				Check:      200,
			},
		},
	}

	_, err := suite.queries.PushHashedRecords(suite.ctx, expire, metrics)
	suite.Require().NoError(err)

	retry, err := suite.queries.PushHashedRecords(suite.ctx, expire, metrics)
	suite.Require().NoError(err)
	suite.Require().Empty(retry)

	metrics.Records[0].SourceHash += 1
	metrics.Records[1].SourceHash += 2
	want := []HashedDuplicate{}
	for _, m := range metrics.Records {
		want = append(want, HashedDuplicate{KeyHash: m.KeyHash, Check: m.Check})
	}

	dupCnt, err := suite.queries.PushHashedRecords(suite.ctx, expire, metrics)
	suite.Require().NoError(err)
	suite.Require().EqualValues(len(want), dupCnt)

	dups, err := suite.queries.GetHashedDuplicates(suite.ctx, expire, metrics)
	suite.Require().NoError(err)
	suite.Require().EqualValues(want, dups)
}

func (q *Queries) getAllHashed(ctx context.Context) (result []HashedSourceRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllHashedQuery.WithParams(q.qp))
	return
}

type HashedSourceRow struct {
	KeyHash    uint64     `db:"key_hash"`
	SourceHash uint64     `db:"source_hash"`
	Check      uint32     `db:"chk"`
	ExpireAt   qtool.Date `db:"expire_at"`
}

func (r HashedSourceRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("key_hash", ydb.Uint64Value(r.KeyHash)),
		ydb.StructFieldValue("source_hash", ydb.Uint64Value(r.SourceHash)),
		ydb.StructFieldValue("chk", ydb.Uint32Value(r.Check)),
		ydb.StructFieldValue("expire_at", r.ExpireAt.Value()),
	)
}

var (
	hashedSourceRowStructType = ydb.Struct(
		ydb.StructField("schema_id", ydb.TypeUint32),
		ydb.StructField("metric_id", ydb.TypeUTF8),
		ydb.StructField("source", ydb.TypeUTF8),
		ydb.StructField("expire_at", ydb.TypeDate),
	)
	hashedSourceRowListType = ydb.List(hashedSourceRowStructType)
	getAllHashedQuery       = qtool.Query(
		"SELECT * FROM", qtool.Table("uniques/hashed"),
		"ORDER BY key_hash;",
	)
)
