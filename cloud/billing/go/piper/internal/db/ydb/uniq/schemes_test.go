package uniq

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type schemesTestSuite struct {
	baseSuite
}

func TestSchemes(t *testing.T) {
	suite.Run(t, new(schemesTestSuite))
}

func (suite *schemesTestSuite) TestHashedPartitions() {
	const parts = 32

	partitions, err := suite.schemes.GetHashedPartitions(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(partitions, parts-1)

	maxUint := ^uint64(0)
	step := maxUint / uint64(parts)
	value := step / 2

	var rows []HashedSource
	for i := 0; i < parts; i++ {
		rows = append(rows, HashedSource{KeyHash: value})
		value += step
	}
	suite.Require().Len(rows, parts)

	batches := SplitHashBatch(partitions, rows)
	suite.Require().Len(batches, parts)

	for i, row := range rows {
		suite.Require().Len(batches[i].Records, 1)
		suite.Equal(row.KeyHash, batches[i].From, "%d != %d", row.KeyHash, batches[i].From)
		suite.Equal(batches[i].From, batches[i].To, "%d != %d", batches[i].From, batches[i].From)
	}
}

func (suite *schemesTestSuite) TestSkipHashedPartitions() {
	const parts = 32

	partitions, err := suite.schemes.GetHashedPartitions(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(partitions, parts-1)

	maxUint := ^uint64(0)
	step := maxUint / uint64(parts)

	var rows []HashedSource
	for i := 14; i < 18; i++ {
		rows = append(rows, HashedSource{KeyHash: step/2 + step*uint64(i)})
	}
	suite.Require().Len(rows, 4)

	batches := SplitHashBatch(partitions, rows)
	suite.Require().Len(batches, 4)

	for i, row := range rows {
		suite.Require().Len(batches[i].Records, 1)
		suite.Equal(row.KeyHash, batches[i].From, "%d != %d", row.KeyHash, batches[i].From)
		suite.Equal(batches[i].From, batches[i].To, "%d != %d", batches[i].From, batches[i].From)
	}
}

func (suite *schemesTestSuite) TestOnlyLastPartition() {
	const parts = 32

	partitions, err := suite.schemes.GetHashedPartitions(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(partitions, parts-1)

	maxUint := ^uint64(0)

	var rows []HashedSource
	for i := 0; i < parts; i++ {
		rows = append(rows, HashedSource{KeyHash: maxUint - uint64(i)})
	}
	suite.Require().Len(rows, parts)

	batches := SplitHashBatch(partitions, rows)
	suite.Require().Len(batches, 1)
}

func (suite *schemesTestSuite) TestHashedEmptyPartitionsSplit() {
	maxUint := ^uint64(0)

	rows := []HashedSource{
		{KeyHash: 0},
		{KeyHash: maxUint},
	}
	batches := SplitHashBatch(nil, rows)
	suite.Require().Len(batches, 1)
}

func (suite *schemesTestSuite) TestHashedEmptyRowsSplit() {
	rows := []HashedSource{}
	batches := SplitHashBatch(nil, rows)
	suite.Require().Empty(batches)
}

func (suite *schemesTestSuite) TestHashedSingleRowsSplit() {
	rows := []HashedSource{
		{KeyHash: 0},
	}
	batches := SplitHashBatch(nil, rows)
	suite.Require().Len(batches, 1)
}

func (suite *schemesTestSuite) TestHashedBatchSplit() {
	var batch HashedSourceBatch
	for i := 0; i < 12; i++ {
		batch.Records = append(batch.Records, HashedSource{KeyHash: uint64(i)})
	}

	splited := batch.SplitBy(5)
	suite.Require().Len(splited, 3)
	suite.Len(splited[0].Records, 5)
	suite.EqualValues(splited[0].From, 0)
	suite.EqualValues(splited[0].To, 4)

	suite.Len(splited[1].Records, 5)
	suite.EqualValues(splited[1].From, 5)
	suite.EqualValues(splited[1].To, 9)

	suite.Len(splited[2].Records, 2)
	suite.EqualValues(splited[2].From, 10)
	suite.EqualValues(splited[2].To, 11)
}
