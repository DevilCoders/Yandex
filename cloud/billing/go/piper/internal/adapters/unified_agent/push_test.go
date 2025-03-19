package unifiedagent

import (
	"encoding/json"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	unifiedagent "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

func strToDec(s string) decimal.Decimal128 {
	result, _ := decimal.FromString(s)
	return result
}

type pushSuite struct {
	clientMockedSuite
	sess *Session

	metric entities.E2EMetric
}

func TestFolder(t *testing.T) {
	suite.Run(t, new(pushSuite))
}

func (suite *pushSuite) SetupTest() {
	suite.clientMockedSuite.SetupTest()
	adapter, _ := New(suite.ctx, suite.mock)
	suite.sess = adapter.Session()

	suite.metric = entities.E2EMetric{
		SkuInfo:   entities.SkuInfo{SkuName: "test-sku"},
		Label:     "test-label",
		Value:     strToDec("10.12345"),
		UsageTime: time.Now(),
	}
}

func (suite *pushSuite) TestResolve() {
	expectedPricingSolomonMetric := unifiedagent.SolomonMetric{
		Labels: map[string]string{
			"sensor":   "sku_pricing_qty",
			"sku_name": "test-sku",
			"label":    "test-label",
		},
		Value:     json.Number("10.12345"),
		Timestamp: suite.metric.UsageTime.Unix(),
	}
	expectedUsageSolomonMetric := unifiedagent.SolomonMetric{
		Labels: map[string]string{
			"sensor":   "sku_usage_qty",
			"sku_name": "test-sku",
			"label":    "test-label",
		},
		Value:     json.Number("10.12345"),
		Timestamp: suite.metric.UsageTime.Unix(),
	}

	suite.mock.On("PushMetrics", mock.Anything, expectedPricingSolomonMetric, expectedUsageSolomonMetric).Return(nil)

	err := suite.sess.PushE2EPricingQuantityMetric(suite.ctx, entities.ProcessingScope{}, suite.metric)
	suite.Require().NoError(err)

	err = suite.sess.PushE2EUsageQuantityMetric(suite.ctx, entities.ProcessingScope{}, suite.metric)
	suite.Require().NoError(err)

	err = suite.sess.FlushE2EQuantityMetrics(suite.ctx)
	suite.Require().NoError(err)
	suite.mock.AssertExpectations(suite.T())
}
