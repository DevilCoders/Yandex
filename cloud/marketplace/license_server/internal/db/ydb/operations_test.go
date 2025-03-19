package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestOperations(t *testing.T) {
	suite.Run(t, new(OperationsTestSuite))
}

type OperationsTestSuite struct {
	BaseYDBTestSuite
}

func (suite *OperationsTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()
}

func (suite *OperationsTestSuite) TestGetByIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertOperation(ctx, &Operation{ID: "id1"}, nil)
	suite.Require().NoError(err)

	op, err := suite.provider.GetOperation(ctx, "id1", nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(op)
	suite.Require().Equal("id1", op.ID)
}

func (suite *OperationsTestSuite) TestUpsertOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertOperation(ctx, &Operation{ID: "id1"}, nil)
	suite.Require().NoError(err)

	op, err := suite.provider.GetOperation(ctx, "id1", nil)
	suite.Require().NoError(err)

	op.Done = true

	err = suite.provider.UpsertOperation(ctx, op, nil)
	suite.Require().NoError(err)

	opUpserted, err := suite.provider.GetOperation(ctx, op.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(opUpserted)
	suite.Require().Equal(op.Done, opUpserted.Done)
}
