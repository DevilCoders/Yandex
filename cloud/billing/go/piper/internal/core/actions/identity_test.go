package actions

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type identityTestSuite struct {
	suite.Suite
	abcResolver           mocks.AbcResolver
	billingAccountsGetter mocks.BillingAccountsGetter
	foldersGetter         mocks.FoldersGetter

	billingAccount  entities.BillingAccount
	cloudAtTime     entities.CloudAtTime
	cloudBindings   []entities.CloudBinding
	resourceAtTime  entities.ResourceAtTime
	resourceBinding entities.ResourceBinding
}

func TestIdentity(t *testing.T) {
	suite.Run(t, new(identityTestSuite))
}

func (suite *identityTestSuite) SetupTest() {
	suite.billingAccountsGetter = mocks.BillingAccountsGetter{}
	suite.billingAccount = entities.BillingAccount{
		AccountID:       "ba_id",
		MasterAccountID: "",
	}

	suite.cloudAtTime = entities.CloudAtTime{
		CloudID: "cloud_id",
		At:      time.Unix(1000, 0).Add(-time.Second),
	}

	suite.cloudBindings = []entities.CloudBinding{
		{
			CloudID:        "cloud_id",
			BillingAccount: "ba_id1",
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Unix(100, 0),
				EffectiveTo:   time.Unix(101, 0),
			},
		},
		{
			CloudID:        "cloud_id",
			BillingAccount: "ba_id2",
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Unix(101, 0),
				EffectiveTo:   time.Unix(102, 0),
			},
		},
		{
			CloudID:        "cloud_id",
			BillingAccount: "ba_id3",
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Unix(102, 0),
				EffectiveTo:   time.Unix(103, 0),
			},
		},
		{
			CloudID:        "cloud_id",
			BillingAccount: "ba_id4",
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Unix(103, 0),
				EffectiveTo:   time.Unix(104, 0),
			},
		},
		{
			CloudID:        "cloud_id",
			BillingAccount: "ba_id",
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Unix(104, 0),
			},
		},
	}

	suite.resourceAtTime = entities.ResourceAtTime{
		ResourceBindingKey: entities.ResourceBindingKey{
			ResourceID:  "resource_id",
			BindingType: entities.TrackerResourceBinding,
		},
		At: time.Unix(1000, 0).Add(-time.Second),
	}

	suite.resourceBinding = entities.ResourceBinding{
		ResourceBindingKey: entities.ResourceBindingKey{
			ResourceID:  "resource_id",
			BindingType: entities.TrackerResourceBinding,
		},
		BindingTimes: entities.BindingTimes{
			EffectiveFrom: time.Unix(100, 0),
		},
		BillingAccount: "ba_id",
	}

	suite.abcResolver.On("ResolveAbc", mock.Anything, mock.Anything, []int64{}).
		Return([]entities.AbcFolder{}, nil)
	suite.foldersGetter.On("GetFolders", mock.Anything, mock.Anything, []string{}).
		Return([]entities.Folder{}, nil)
	suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{}).
		Return([]entities.CloudBinding{}, nil)
	suite.billingAccountsGetter.On("GetResourceBindings", mock.Anything, mock.Anything, []entities.ResourceAtTime{}).
		Return([]entities.ResourceBinding{}, nil)
	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{}).
		Return([]entities.BillingAccount{}, nil)
}

func (suite *identityTestSuite) TestResolveBillingAccount() {
	metric := entities.EnrichedMetric{}
	metric.BillingAccountID = "ba_id"
	metrics := []entities.EnrichedMetric{metric}

	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{"ba_id"}).
		Return([]entities.BillingAccount{suite.billingAccount}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(unresolved)
	suite.Require().Len(valid, 1)
	suite.Equal("ba_id", valid[0].ReshardingKey)
}

func (suite *identityTestSuite) TestUnresolvedBillingAccount() {
	metric := entities.EnrichedMetric{}
	metric.BillingAccountID = "ba_id"
	metrics := []entities.EnrichedMetric{metric}

	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{"ba_id"}).
		Return([]entities.BillingAccount{}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(unresolved, 1)
}

func (suite *identityTestSuite) TestResolveResources() {
	metric := entities.EnrichedMetric{}
	metric.ResourceID = "resource_id"
	metric.ResourceBindingType = entities.TrackerResourceBinding
	metric.Usage.Finish = suite.resourceAtTime.At.Add(time.Second)
	metrics := []entities.EnrichedMetric{metric}

	suite.billingAccountsGetter.On("GetResourceBindings", mock.Anything, mock.Anything, []entities.ResourceAtTime{suite.resourceAtTime}).
		Return([]entities.ResourceBinding{suite.resourceBinding}, nil)
	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{"ba_id"}).
		Return([]entities.BillingAccount{suite.billingAccount}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(unresolved)
	suite.Require().Len(valid, 1)
	suite.Require().Equal("ba_id", valid[0].BillingAccountID)
}

func (suite *identityTestSuite) TestUnResolveResources() {
	metric := entities.EnrichedMetric{}
	metric.ResourceID = "resource_id"
	metric.ResourceBindingType = entities.TrackerResourceBinding
	metric.Usage.Finish = suite.resourceAtTime.At.Add(time.Second)
	metrics := []entities.EnrichedMetric{metric}

	resourceBinding := suite.resourceBinding
	resourceBinding.BindingTimes = entities.BindingTimes{
		EffectiveFrom: suite.resourceAtTime.At.Add(time.Second),
	}

	suite.billingAccountsGetter.On("GetResourceBindings", mock.Anything, mock.Anything, []entities.ResourceAtTime{suite.resourceAtTime}).
		Return([]entities.ResourceBinding{resourceBinding}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(unresolved, 1)
	suite.Require().Empty(metrics[0].BillingAccountID)
}

func (suite *identityTestSuite) TestResolveClouds() {
	for _, b := range suite.cloudBindings {
		test := struct {
			name      string
			usageTime time.Time
			baID      string
		}{
			name:      b.BillingAccount,
			usageTime: b.EffectiveFrom,
			baID:      b.BillingAccount,
		}

		suite.Run(test.name, func() {
			metric := entities.EnrichedMetric{}
			metric.CloudID = "cloud_id"
			metric.Usage.Finish = test.usageTime.Add(time.Second)
			metrics := []entities.EnrichedMetric{metric}

			suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{{CloudID: "cloud_id", At: test.usageTime}}).
				Return(suite.cloudBindings, nil)
			suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{test.baID}).
				Return([]entities.BillingAccount{{
					AccountID:       test.baID,
					MasterAccountID: "",
				}}, nil)

			valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
				&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

			suite.Require().NoError(err)
			suite.Require().Empty(unresolved)
			suite.Require().Len(valid, 1)
			suite.Require().Equal(test.baID, valid[0].BillingAccountID)
		})
	}
}

func (suite *identityTestSuite) TestResolveCloudsStartEqFinish() {
	for _, b := range suite.cloudBindings {
		test := struct {
			name      string
			usageTime time.Time
			baID      string
		}{
			name:      b.BillingAccount,
			usageTime: b.EffectiveFrom,
			baID:      b.BillingAccount,
		}

		suite.Run(test.name, func() {
			metric := entities.EnrichedMetric{}
			metric.CloudID = "cloud_id"
			metric.Usage.Start = test.usageTime
			metric.Usage.Finish = test.usageTime
			metrics := []entities.EnrichedMetric{metric}

			suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{{CloudID: "cloud_id", At: test.usageTime}}).
				Return(suite.cloudBindings, nil)
			suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{test.baID}).
				Return([]entities.BillingAccount{{
					AccountID:       test.baID,
					MasterAccountID: "",
				}}, nil)

			valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
				&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

			suite.Require().NoError(err)
			suite.Require().Empty(unresolved)
			suite.Require().Len(valid, 1)
			suite.Require().Equal(test.baID, valid[0].BillingAccountID)
		})
	}
}

func (suite *identityTestSuite) TestUnResolveClouds() {
	metric := entities.EnrichedMetric{}
	metric.CloudID = "cloud_id"
	metric.Usage.Finish = suite.cloudAtTime.At.Add(time.Second)
	metrics := []entities.EnrichedMetric{metric}

	cloudBinding := suite.cloudBindings[0]
	cloudBinding.BindingTimes = entities.BindingTimes{
		EffectiveFrom: suite.cloudAtTime.At.Add(time.Second),
	}

	suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{suite.cloudAtTime}).
		Return([]entities.CloudBinding{cloudBinding}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(unresolved, 1)
	suite.Require().Empty(metrics[0].BillingAccountID)
}

func (suite *identityTestSuite) TestResolveFolders() {
	metric := entities.EnrichedMetric{}
	metric.FolderID = "folder_id"
	metric.Usage.Finish = suite.cloudAtTime.At.Add(time.Second)
	metrics := []entities.EnrichedMetric{metric}

	folder := entities.Folder{
		FolderID: "folder_id",
		CloudID:  "cloud_id",
	}

	suite.foldersGetter.On("GetFolders", mock.Anything, mock.Anything, []string{metric.FolderID}).
		Return([]entities.Folder{folder}, nil)
	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{"ba_id"}).
		Return([]entities.BillingAccount{suite.billingAccount}, nil)
	suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{suite.cloudAtTime}).
		Return(suite.cloudBindings, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(unresolved)
	suite.Require().Len(valid, 1)
	suite.Require().Equal("cloud_id", valid[0].CloudID)
}

func (suite *identityTestSuite) TestUnResolveFolders() {
	metric := entities.EnrichedMetric{}
	metric.FolderID = "unresolved_folder_id"
	metrics := []entities.EnrichedMetric{metric}

	suite.foldersGetter.On("GetFolders", mock.Anything, mock.Anything, []string{metric.FolderID}).
		Return([]entities.Folder{}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(unresolved, 1)
	suite.Require().Empty(metrics[0].CloudID)
}

func (suite *identityTestSuite) TestResolveAbc() {
	metric := entities.EnrichedMetric{}
	metric.AbcID = 1
	metric.AbcFolderID = "abc_folder_id"
	metric.Usage.Finish = suite.cloudAtTime.At.Add(time.Second)
	metrics := []entities.EnrichedMetric{metric}

	abcFolder := entities.AbcFolder{
		AbcID:       1,
		AbcFolderID: "abc_folder_id",
		CloudID:     "cloud_id",
	}

	suite.abcResolver.On("ResolveAbc", mock.Anything, mock.Anything, []int64{metric.AbcID}).
		Return([]entities.AbcFolder{abcFolder}, nil)
	suite.billingAccountsGetter.On("GetBillingAccounts", mock.Anything, mock.Anything, []string{"ba_id"}).
		Return([]entities.BillingAccount{suite.billingAccount}, nil)
	suite.billingAccountsGetter.On("GetCloudBindings", mock.Anything, mock.Anything, []entities.CloudAtTime{suite.cloudAtTime}).
		Return(suite.cloudBindings, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(unresolved)
	suite.Require().Len(valid, 1)
	suite.Require().Equal("cloud_id", metrics[0].CloudID)
	suite.Require().Equal("abc_folder_id", metrics[0].AbcFolderID)
}

func (suite *identityTestSuite) TestUnResolveAbc() {
	metric := entities.EnrichedMetric{}
	metric.AbcID = 2
	metric.AbcFolderID = "abc_folder_id"
	metrics := []entities.EnrichedMetric{metric}

	suite.abcResolver.On("ResolveAbc", mock.Anything, mock.Anything, []int64{metric.AbcID}).
		Return([]entities.AbcFolder{}, nil)

	valid, unresolved, err := ResolveMetricsIdentity(context.Background(), entities.ProcessingScope{},
		&suite.billingAccountsGetter, &suite.foldersGetter, &suite.abcResolver, metrics)

	suite.Require().NoError(err)
	suite.Require().Empty(valid)
	suite.Require().Len(unresolved, 1)
	suite.Require().Empty(metrics[0].CloudID)
	suite.Require().Empty(metrics[0].AbcFolderID)
}
