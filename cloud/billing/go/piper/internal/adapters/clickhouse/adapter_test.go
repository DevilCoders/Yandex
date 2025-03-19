package clickhouse

import (
	"context"
	"testing"

	clickhouseclient "a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/client"

	"github.com/stretchr/testify/suite"
)

type adapterTestSuite struct {
	clickhouseMockSuite
}

func TestAdapter(t *testing.T) {
	suite.Run(t, new(adapterTestSuite))
}

func (suite *adapterTestSuite) TestNewAdapter() {
	ctx := context.Background()
	clusterNode := clickhouseclient.ClusterNode{
		Name:   "test_node",
		DBConn: suite.dbx(),
	}
	cluster, err := clickhouseclient.NewCluster(ctx, [][]clickhouseclient.ClusterNode{{clusterNode}})
	suite.Require().NoError(err)
	a, err := New(ctx, cluster)
	suite.Require().NoError(err)
	suite.Require().NotNil(a)
}

func (suite *adapterTestSuite) TestSession() {
	ctx := context.Background()
	clusterNode := clickhouseclient.ClusterNode{
		Name:   "test_node",
		DBConn: suite.dbx(),
	}
	cluster, err := clickhouseclient.NewCluster(ctx, [][]clickhouseclient.ClusterNode{{clusterNode}})
	suite.Require().NoError(err)
	a, err := New(ctx, cluster)
	suite.Require().NoError(err)

	session := a.Session()
	suite.Require().NotNil(session)
	suite.Equal(a, session.adapter)
}

type baseSessionTestSuite struct {
	clickhouseMockSuite

	ctx       context.Context
	ctxCancel context.CancelFunc
	session   *Session
}

func (suite *baseSessionTestSuite) SetupTest() {
	suite.clickhouseMockSuite.SetupTest()

	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())
	clusterNode := clickhouseclient.ClusterNode{
		Name:   "test_node",
		DBConn: suite.dbx(),
	}
	cluster, err := clickhouseclient.NewCluster(suite.ctx, [][]clickhouseclient.ClusterNode{{clusterNode}})
	suite.Require().NoError(err)
	a, err := New(suite.ctx, cluster)
	suite.Require().NoError(err)

	suite.session = a.Session()
}
