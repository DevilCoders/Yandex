package instances

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestListAction(t *testing.T) {
	suite.Run(t, new(ListActionTestSuite))
}

type ListActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *ListActionTestSuite) TestListOk() {
	cloudID := "cloud_id"
	li1 := &license.Instance{
		ID:      "id1",
		CloudID: cloudID,
		State:   license.ActiveInstanceState,
	}
	li2 := &license.Instance{
		ID:      "id2",
		CloudID: cloudID,
		State:   license.ActiveInstanceState,
	}
	li3 := &license.Instance{
		ID:      "id3",
		CloudID: cloudID,
		State:   license.DeprecatedInstanceState,
	}
	li4 := &license.Instance{
		ID:      "id4",
		CloudID: "another_cloud_id",
		State:   license.ActiveInstanceState,
	}

	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li1, nil)
	suite.Require().NoError(err)
	err = suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li2, nil)
	suite.Require().NoError(err)
	err = suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li3, nil)
	suite.Require().NoError(err)
	err = suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li4, nil)
	suite.Require().NoError(err)

	listResult, err := NewListAction(suite.Env).Do(context.Background(), ListParams{CloudID: cloudID})
	suite.Require().NoError(err)
	suite.Require().NotEmpty(listResult.Instances)

	lisValue := make([]license.Instance, 0, len(listResult.Instances))
	for _, li := range listResult.Instances {
		lisValue = append(lisValue, *li)
	}

	suite.Require().Contains(lisValue, *li1, *li2)
	suite.Require().NotContains(lisValue, *li3, *li4)
}

func (suite *ListActionTestSuite) TestListError() {
	listResult, err := NewListAction(suite.Env).Do(context.Background(), ListParams{CloudID: "no_id"})
	suite.Require().NoError(err)
	suite.Require().Len(listResult.Instances, 0)
}
