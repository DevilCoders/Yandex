package handlers

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers/mocks"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type ydbPresenterTestSuite struct {
	suite.Suite
	messagesMocks

	target  *mocks.YDBPresenterTarget
	handler *YDBPresenterHandler
}

func TestYDBPresenter(t *testing.T) {
	suite.Run(t, new(ydbPresenterTestSuite))
}

func (suite *ydbPresenterTestSuite) SetupTest() {
	suite.messagesMocks.SetupTest()

	suite.target = &mocks.YDBPresenterTarget{}
	suite.handler = NewYDBPresenterHandler("test/src", "presenter:ydb", "presenter", suite.getTarget)
	suite.handler.clock = getClock()

	suite.pushMessage(`{
	  "usage": {
		"quantity": 59,
		"start": 1647520020,
		"finish": 1647520079,
		"unit": "byte*second",
		"type": "delta"
	  },
	  "tags": {
		"ydb_size": 0
	  },
	  "id": "72057594046678944-33099-1647520020-1647520079-0",
	  "cloud_id": "aoe6k5na87r0bo4nicic",
	  "source_wt": 1647520080,
	  "source_id": "sless-docapi-ydb-storage",
	  "resource_id": "cc832n76818nr2be0lje",
	  "schema": "ydb.serverless.v1",
	  "folder_id": "aoesdnouslbivb50ikbb",
	  "version": "1.0.0",
	  "message_write_ts": 1647520206,
	  "sequence_id": 2387224,
	  "abc_id": null,
	  "labels": {
		"user_labels": {},
		"system_labels": {
		  "folder_id": "aoesdnouslbivb50ikbb"
		}
	  },
	  "sku_id": "a6qiqvpuvujh28ggku9h",
	  "billing_account_id": "a6q66pqrnlvt8lob5hc0",
	  "master_account_id": null,
	  "sku_name": "ydb.v1.serverless.storage",
	  "pricing_unit": "gbyte*hour",
	  "is_expired": false,
	  "labels_hash": 9372812463371188000,
	  "labels_json": "{\"system_labels\": {\"folder_id\": \"aoesdnouslbivb50ikbb\"}, \"user_labels\": {}}",
	  "is_user_labels_allowed": true,
	  "start_time": 1647518400,
	  "end_time": 1647521999,
	  "usage_time": 1647520078,
	  "pricing_quantity": "1",
	  "resharding_key": "a6q66pqrnlvt8lob5hc0",
	  "cloud_name": "atroynikov",
	  "folder_name": "default",
	  "pricing_version_id": "a6qq5ose3mmiasb8smiq",
	  "rate_id": 0,
	  "sku_overridden": false,
	  "currency": "RUB",
	  "service_id": "a6qydbtgqogjqcrlbehu",
	  "publisher_account_id": null,
	  "publisher_balance_client_id": null,
	  "publisher_currency": null,
	  "tiered_pricing_quantity": "0",
	  "unit_price": "0",
	  "currency_multiplier": "1",
	  "credit": "3",
	  "cost": "2",
	  "expense": "0",
	  "rewarded_expense": "0",
	  "revenue": "0",
	  "cud_credit": "4",
	  "cud_compensated_pricing_quantity": "0",
	  "monetary_grant_credit": "5",
	  "volume_incentive_credit": "6",
	  "service_credit": "0",
	  "trial_credit": "0",
	  "disabled_credit": "0",
	  "var_incentive_credit": "0",
	  "volume_reward": "0",
	  "reward": "0",
	  "credit_charges": [],
	  "volume_reward_info": []
	}`)
}

func (suite *ydbPresenterTestSuite) getTarget() YDBPresenterTarget {
	return suite.target
}

func (suite *ydbPresenterTestSuite) TestPush() {
	var calbackErr error

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))
	suite.reporter.On("Consumed")
	suite.target.On("PushPresenterMetric", mock.Anything, mock.Anything, mock.Anything).Once().Return(nil)
	suite.target.On("FlushPresenterMetric", mock.Anything).Return(nil)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().NoError(calbackErr)

	var pushCall mock.Call
	for _, c := range suite.target.Calls {
		if c.Method == "PushPresenterMetric" {
			pushCall = c
		}
	}
	suite.Require().Equal("PushPresenterMetric", pushCall.Method)

	wantScope := entities.ProcessingScope{
		SourceName:       "test/src",
		SourceType:       "logbroker-grpc",
		SourceID:         "test-source",
		StartTime:        getClock().Now(),
		Hostname:         "test-host",
		Pipeline:         "presenter",
		MinMessageOffset: 1,
		MaxMessageOffset: 1,
	}
	wantMetric := entities.PresenterMetric{
		EnrichedMetric: entities.EnrichedMetric{
			SourceMetric: entities.SourceMetric{
				MetricID:         "72057594046678944-33099-1647520020-1647520079-0",
				Schema:           "ydb.serverless.v1",
				Version:          "1.0.0",
				CloudID:          "aoe6k5na87r0bo4nicic",
				FolderID:         "aoesdnouslbivb50ikbb",
				AbcID:            0,
				AbcFolderID:      "",
				ResourceID:       "cc832n76818nr2be0lje",
				BillingAccountID: "a6q66pqrnlvt8lob5hc0",
				SkuID:            "a6qiqvpuvujh28ggku9h",
				Labels: entities.Labels{
					User: map[string]string{},
				},
				Tags: "{ \t\t\"ydb_size\": 0 \t  }",
				Usage: entities.MetricUsage{
					Quantity: decimal.Must(decimal.FromString("59")),
					Start:    time.Unix(1647520020, 0),
					Finish:   time.Unix(1647520079, 0),
					Unit:     "byte*second",
					Type:     entities.DeltaUsage,
					RawType:  "delta",
				},
				SourceID:         "sless-docapi-ydb-storage",
				SourceWT:         time.Unix(1647520080, 0),
				MessageWriteTime: suite.messages.LastWriteTime,
				MessageOffset:    suite.messages.LastOffset,
			},
			SkuInfo: entities.SkuInfo{
				SkuName:      "ydb.v1.serverless.storage",
				PricingUnit:  "gbyte*hour",
				SkuUsageType: entities.DeltaUsage,
			},
			//Period: entities.MetricPeriod{},
			PricingQuantity: decimal.Must(decimal.FromString("1")),
			//Products:            nil,
			//ResourceBindingType: 0,
			//TagsOverride:        nil,
			MasterAccountID: "",
			ReshardingKey:   "a6q66pqrnlvt8lob5hc0",
		},
		LabelsHash:            9372812463371188000,
		Cost:                  decimal.Must(decimal.FromString("2")),
		Credit:                decimal.Must(decimal.FromString("3")),
		CudCredit:             decimal.Must(decimal.FromString("4")),
		MonetaryGrantCredit:   decimal.Must(decimal.FromString("5")),
		VolumeIncentiveCredit: decimal.Must(decimal.FromString("6")),
	}

	suite.EqualValues(wantScope, pushCall.Arguments[1])
	suite.EqualValues(wantMetric, pushCall.Arguments[2])
}

func (suite *ydbPresenterTestSuite) TestError() {
	var calbackErr error

	testErr := errors.New("testErr")

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))
	suite.target.On("PushPresenterMetric", mock.Anything, mock.Anything, mock.Anything).Once().Return(testErr)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().Error(calbackErr)
	suite.ErrorIs(calbackErr, testErr)
}

func (suite *ydbPresenterTestSuite) TestParseError() {
	suite.pushMessage(`Not json`)

	var calbackErr error

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().Error(calbackErr)
}
