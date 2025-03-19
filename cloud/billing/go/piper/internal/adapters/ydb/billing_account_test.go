package ydb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type baTestSuite struct {
	baseMetaSessionTestSuite
}

func TestBillingAccount(t *testing.T) {
	suite.Run(t, new(baTestSuite))
}

func (suite *baTestSuite) SetupTest() {
	suite.baseMetaSessionTestSuite.SetupTest()
}

func (suite *baTestSuite) TestGetBillingAccountsEmpty() {
	got, err := suite.session.GetBillingAccounts(suite.ctx, entities.ProcessingScope{}, []string{})
	suite.Require().NoError(err)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetBillingAccounts() {
	baResult := suite.mock.NewRows([]string{"id", "master_account_id"}).
		AddRow("ba1.1", "ma1").
		AddRow("ba1.2", "ma1").
		AddRow("ba2", "ma2").
		AddRow("ba_lonely", nil)

	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/billing_accounts`").WillReturnRows(baResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetBillingAccounts(suite.ctx, entities.ProcessingScope{}, []string{"ba1.1", "ba1.2", "ba2", "ba_lonely"})
	suite.Require().NoError(err)
	want := []entities.BillingAccount{
		{AccountID: "ba1.1", MasterAccountID: "ma1"},
		{AccountID: "ba1.2", MasterAccountID: "ma1"},
		{AccountID: "ba2", MasterAccountID: "ma2"},
		{AccountID: "ba_lonely"},
	}
	suite.ElementsMatch(want, got)
	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *baTestSuite) TestGetBillingAccountsParted() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	suite.mock.ExpectBegin()
	baResult1 := suite.mock.NewRows([]string{"id", "master_account_id"}).
		AddRow("ba1.1", "ma1").
		AddRow("ba2", "ma2")
	suite.mock.ExpectQuery("FROM `meta/billing_accounts`").WillReturnRows(baResult1).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	suite.mock.ExpectBegin()
	baResult2 := suite.mock.NewRows([]string{"id", "master_account_id"}).
		AddRow("ba1.2", "ma1").
		AddRow("ba_lonely", nil)
	suite.mock.ExpectQuery("FROM `meta/billing_accounts`").WillReturnRows(baResult2).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetBillingAccounts(suite.ctx, entities.ProcessingScope{}, []string{"ba1.1", "ba1.2", "ba2", "ba_lonely"})
	suite.Require().NoError(err)
	want := []entities.BillingAccount{
		{AccountID: "ba1.1", MasterAccountID: "ma1"},
		{AccountID: "ba1.2", MasterAccountID: "ma1"},
		{AccountID: "ba2", MasterAccountID: "ma2"},
		{AccountID: "ba_lonely"},
	}
	suite.ElementsMatch(want, got)
}

func (suite *baTestSuite) TestGetBillingAccountsError() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	baResult := suite.mock.NewRows([]string{"id", "master_account_id"})
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/billing_accounts`").WillReturnRows(baResult).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	suite.mock.ExpectBegin().WillReturnError(errTest)

	got, err := suite.session.GetBillingAccounts(suite.ctx, entities.ProcessingScope{}, []string{"ba1.1", "ba1.2", "ba2", "ba_lonely"})
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetCloudBindingsEmpty() {
	got, err := suite.session.GetCloudBindings(suite.ctx, entities.ProcessingScope{}, []entities.CloudAtTime{})
	suite.Require().NoError(err)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetCloudBindings() {
	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("cloud", "cld1.1", "ba1", uint64(0), uint64(1000)).
		AddRow("cloud", "cld1.2", "ba1", uint64(0), uint64(1000)).
		AddRow("cloud", "cld2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetCloudBindings(suite.ctx, entities.ProcessingScope{}, []entities.CloudAtTime{
		{CloudID: "cld1.1", At: time.Unix(100, 0)},
		{CloudID: "cld1.2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(200, 0)},
	})
	suite.Require().NoError(err)
	want := []entities.CloudBinding{
		{CloudID: "cld1.1", BillingAccount: "ba1", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
		{CloudID: "cld1.2", BillingAccount: "ba1", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
		{CloudID: "cld2", BillingAccount: "ba2", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
	}

	suite.ElementsMatch(want, got)
	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *baTestSuite) TestGetCloudBindingsParted() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	result1 := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("cloud", "cld1.1", "ba1", uint64(0), uint64(1000)).
		AddRow("cloud", "cld1.2", "ba1", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result1).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	result2 := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("cloud", "cld2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result2).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetCloudBindings(suite.ctx, entities.ProcessingScope{}, []entities.CloudAtTime{
		{CloudID: "cld1.1", At: time.Unix(100, 0)},
		{CloudID: "cld1.2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(200, 0)},
	})
	suite.Require().NoError(err)
	want := []entities.CloudBinding{
		{CloudID: "cld1.1", BillingAccount: "ba1", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
		{CloudID: "cld1.2", BillingAccount: "ba1", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
		{CloudID: "cld2", BillingAccount: "ba2", BindingTimes: entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)}},
	}

	suite.ElementsMatch(want, got)
	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *baTestSuite) TestGetCloudBindingsError() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	suite.mock.ExpectBegin().WillReturnError(errTest)

	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("cloud", "cld2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetCloudBindings(suite.ctx, entities.ProcessingScope{}, []entities.CloudAtTime{
		{CloudID: "cld1.1", At: time.Unix(100, 0)},
		{CloudID: "cld1.2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(100, 0)},
		{CloudID: "cld2", At: time.Unix(200, 0)},
	})
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetCloudBindingsDBTypeError() {
	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("bad type", "cld", "ba", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	_, err := suite.session.GetCloudBindings(suite.ctx, entities.ProcessingScope{}, []entities.CloudAtTime{
		{CloudID: "cld", At: time.Unix(100, 0)},
	})
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrResourceBindingType)
}

func (suite *baTestSuite) TestGetResourceBindingsEmpty() {
	got, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{})
	suite.Require().NoError(err)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetResourceBindings() {
	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("tracker", "rs1.1", "ba1", uint64(0), uint64(1000)).
		AddRow("tracker", "rs1.2", "ba1", uint64(0), uint64(1000)).
		AddRow("tracker", "rs2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
	})
	suite.Require().NoError(err)
	want := []entities.ResourceBinding{
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba1",
		},
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.2", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba1",
		},
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba2",
		},
	}

	suite.ElementsMatch(want, got)
	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *baTestSuite) TestGetResourceBindingsParted() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	result1 := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("tracker", "rs1.1", "ba1", uint64(0), uint64(1000)).
		AddRow("tracker", "rs1.2", "ba1", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result1).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	result2 := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("tracker", "rs2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result2).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
	})
	suite.Require().NoError(err)
	want := []entities.ResourceBinding{
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba1",
		},
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.2", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba1",
		},
		{
			ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding},
			BindingTimes:       entities.BindingTimes{EffectiveFrom: time.Unix(0, 0), EffectiveTo: time.Unix(1000, 0)},
			BillingAccount:     "ba2",
		},
	}

	suite.ElementsMatch(want, got)
	suite.NoError(suite.mock.ExpectationsWereMet())
}

func (suite *baTestSuite) TestGetResourceBindingsError() {
	suite.session.batchSizeOverride = 2
	suite.mock.MatchExpectationsInOrder(false)

	suite.mock.ExpectBegin().WillReturnError(errTest)

	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("cloud", "cld2", "ba2", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	got, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs2", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
	})
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
	suite.Empty(got)
}

func (suite *baTestSuite) TestGetResourceBindingsUnknownType() {
	_, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "r", BindingType: entities.NoResourceBinding}, At: time.Unix(100, 0)},
	})
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrResourceBindingType)
}

func (suite *baTestSuite) TestGetResourceBindingsDBTypeError() {
	result := suite.mock.NewRows([]string{"service_instance_type", "service_instance_id", "billing_account_id", "effective_time", "effective_to"}).
		AddRow("bad type", "r", "ba1", uint64(0), uint64(1000))
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/service_instance_bindings/bindings`").WillReturnRows(result).RowsWillBeClosed()
	suite.mock.ExpectCommit()

	_, err := suite.session.GetResourceBindings(suite.ctx, entities.ProcessingScope{}, []entities.ResourceAtTime{
		{ResourceBindingKey: entities.ResourceBindingKey{ResourceID: "rs1.1", BindingType: entities.TrackerResourceBinding}, At: time.Unix(100, 0)},
	})
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrResourceBindingType)
}
