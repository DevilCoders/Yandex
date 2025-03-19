package handlers

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers/mocks"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type resharderMocks struct {
	messagesMocks

	metricsResolver        mocks.MetricsResolver
	billingAccountsGetter  mocks.BillingAccountsGetter
	identityResolver       mocks.IdentityResolver
	abcResolver            mocks.AbcResolver
	metricsPusher          mocks.MetricsPusher
	oversizedMessagePusher mocks.OversizedMessagePusher
	cumulativeCalculator   mocks.CumulativeCalculator
	duplicatesSeeker       mocks.DuplicatesSeeker
	e2ePusher              mocks.E2EPusher

	fabrics ResharderFabrics

	metricsSchema     string
	metricsSchemaTags string
	metricsUsageType  entities.UsageType
}

func (suite *resharderMocks) SetupTest() {
	suite.messagesMocks.SetupTest()

	suite.metricsSchema = "some-metrics-schema"
	suite.metricsSchemaTags = `{"cores":3}`
	suite.metricsUsageType = entities.DeltaUsage

	suite.oversizedMessagePusher = mocks.OversizedMessagePusher{}
	suite.metricsResolver = mocks.MetricsResolver{}
	suite.billingAccountsGetter = mocks.BillingAccountsGetter{}
	suite.identityResolver = mocks.IdentityResolver{}
	suite.abcResolver = mocks.AbcResolver{}
	suite.metricsPusher = mocks.MetricsPusher{}
	suite.cumulativeCalculator = mocks.CumulativeCalculator{}
	suite.duplicatesSeeker = mocks.DuplicatesSeeker{}
	suite.e2ePusher = mocks.E2EPusher{}

	suite.fabrics = ResharderFabrics{
		OversizedMessagePusherFabric: func() OversizedMessagePusher { return &suite.oversizedMessagePusher },
		MetricsResolverFabric:        func() MetricsResolver { return &suite.metricsResolver },
		BillingAccountsGetterFabric:  func() BillingAccountsGetter { return &suite.billingAccountsGetter },
		IdentityResolverFabric:       func() IdentityResolver { return &suite.identityResolver },
		AbcResolverFabric:            func() AbcResolver { return &suite.abcResolver },
		MetricsPusherFabric:          func() MetricsPusher { return &suite.metricsPusher },
		CumulativeCalculatorFabric:   func() CumulativeCalculator { return &suite.cumulativeCalculator },
		DuplicatesSeekerFabric:       func() DuplicatesSeeker { return &suite.duplicatesSeeker },
		E2EPusherFabric:              func() E2EPusher { return &suite.e2ePusher },
	}
}

var (
	anyCtx           = mock.MatchedBy(func(context.Context) bool { return true })
	anyTime          = mock.MatchedBy(func(time.Time) bool { return true })
	emptyStringsList = mock.MatchedBy(func(p []string) bool { return len(p) == 0 })
	emptyInt64List   = mock.MatchedBy(func(p []int64) bool { return len(p) == 0 })
)

func (suite *resharderMocks) initMocks(quantity decimal.Decimal128, maxMessageOffset uint64) {
	scope := entities.ProcessingScope{
		SourceName:       "test/src",
		SourceType:       "logbroker-grpc",
		SourceID:         "test-source",
		StartTime:        getClock().Now(),
		Hostname:         "test-host",
		Pipeline:         "resharder",
		MinMessageOffset: 1,
		MaxMessageOffset: maxMessageOffset,
	}

	sku := entities.Sku{
		SkuID: "sku_id",
		SkuInfo: entities.SkuInfo{
			SkuUsageType: suite.metricsUsageType,
			PricingUnit:  "pricing_unit",
		},
		SkuUsageInfo: entities.SkuUsageInfo{
			Formula:   "usage.quantity",
			UsageUnit: "usage_unit",
		},
	}
	skuResolvingRule := entities.SkuResolveRules{
		Key: suite.metricsSchema,
		Rules: []entities.SkuResolve{
			{
				SkuID: "sku_id",
				Rules: []entities.MatchRulesByPath{{
					"tags.notexists": {Type: entities.ExistenceRule, Exists: false},
				}},
			},
		},
	}
	billingAccount := entities.BillingAccount{AccountID: "ba"}

	suite.duplicatesSeeker.On("FindDuplicates", anyCtx, scope, anyTime, mock.Anything).
		Return(nil, nil)
	suite.metricsResolver.On("GetMetricSchema", anyCtx, scope, suite.metricsSchema).
		Return(entities.MetricsSchema{Schema: suite.metricsSchema}, nil)
	suite.metricsResolver.On("GetSkuResolving", anyCtx, scope, []string{suite.metricsSchema}, emptyStringsList, emptyStringsList).
		Return([]entities.SkuResolveRules{skuResolvingRule}, nil, []entities.Sku{sku}, nil)
	suite.metricsResolver.On("ConvertQuantity", anyCtx, scope, anyTime, "usage_unit", "pricing_unit", quantity).
		Return(quantity, nil)
	suite.abcResolver.On("ResolveAbc", anyCtx, scope, emptyInt64List).
		Return(nil, nil)
	suite.identityResolver.On("GetFolders", anyCtx, scope, emptyStringsList).
		Return(nil, nil)
	suite.billingAccountsGetter.On("GetResourceBindings", anyCtx, scope, mock.Anything).
		Return(nil, nil)
	suite.billingAccountsGetter.On("GetCloudBindings", anyCtx, scope, mock.Anything).
		Return(nil, nil)
	suite.billingAccountsGetter.On("GetBillingAccounts", anyCtx, scope, []string{"ba"}).
		Return([]entities.BillingAccount{billingAccount}, nil)
	suite.metricsPusher.On("EnrichedMetricPartitions").
		Return(1)
	suite.metricsPusher.On("FlushEnriched", anyCtx).
		Return(nil)
	suite.metricsPusher.On("PushEnrichedMetricToPartition", anyCtx, scope, mock.Anything, 0).
		Return(nil)
	suite.e2ePusher.On("FlushE2EQuantityMetrics", anyCtx).Return(nil)
	suite.reporter.On("Consumed")
}

func (suite *resharderMocks) initCumulativeMocks(quantity, cumulativeQuantity decimal.Decimal128) {
	period1, _ := time.Parse(time.RFC3339, "2000-01-01T00:00:00+03:00")
	cumulativePeriod := mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period1) })
	scope := entities.ProcessingScope{
		SourceName:       "test/src",
		SourceType:       "logbroker-grpc",
		SourceID:         "test-source",
		StartTime:        getClock().Now(),
		Hostname:         "test-host",
		Pipeline:         "resharder",
		MinMessageOffset: 1,
		MaxMessageOffset: 1,
	}

	cumulativeSource := entities.CumulativeSource{
		ResourceID:      "resource_id",
		SkuID:           "sku_id",
		PricingQuantity: quantity,
		MetricOffset:    1,
	}
	cumulativeUsage := entities.CumulativeUsageLog{
		FirstPeriod:  true,
		ResourceID:   "resource_id",
		SkuID:        "sku_id",
		MetricOffset: 1,
		Delta:        cumulativeQuantity,
	}

	suite.cumulativeCalculator.On("CalculateCumulativeUsage", anyCtx, scope, cumulativePeriod, []entities.CumulativeSource{cumulativeSource}).
		Return(1, []entities.CumulativeUsageLog{cumulativeUsage}, nil)
	suite.metricsResolver.On("ConvertQuantity", anyCtx, scope, anyTime, "mock.Anything", "mock.Anything", cumulativeQuantity).
		Return(cumulativeQuantity, nil)
}

func (suite *resharderMocks) getLastPushedMetric() entities.EnrichedMetric {
	call := getLastMockCall("PushEnrichedMetricToPartition", suite.metricsPusher.Calls)
	if call.Method == "" {
		return entities.EnrichedMetric{}
	}
	return call.Arguments[2].(entities.EnrichedMetric)
}

func (suite *resharderMocks) assertMocks(t *testing.T) {
	suite.metricsResolver.AssertExpectations(t)
	suite.billingAccountsGetter.AssertExpectations(t)
	suite.identityResolver.AssertExpectations(t)
	suite.abcResolver.AssertExpectations(t)
	suite.metricsPusher.AssertExpectations(t)
	suite.oversizedMessagePusher.AssertExpectations(t)
	suite.cumulativeCalculator.AssertExpectations(t)
	suite.duplicatesSeeker.AssertExpectations(t)
}

func (h *commonResharderHandler) setConfig(config ResharderHandlerConfig) {
	h.config = config
}

func (h *commonResharderHandler) mockClock() {
	h.clock = getClock()
}

type generalResharderTestSuite struct {
	suite.Suite
	resharderMocks
	metricsMaker

	handler *GeneralResharderHandler
}

func TestGeneralResharder(t *testing.T) {
	suite.Run(t, new(generalResharderTestSuite))
}

func (suite *generalResharderTestSuite) SetupTest() {
	suite.resharderMocks.SetupTest()
	suite.handler = NewGeneralResharderHandler("general", "resharder", "test/src", ResharderHandlerConfig{}, suite.fabrics)
	suite.handler.mockClock()

	now := getClock().Now()
	suite.pushMessage(suite.formatSourceMessage(suite.metricsSchema, now.Add(time.Second*10), now.Add(time.Second*20), 10, suite.metricsSchemaTags))
}

func (suite *generalResharderTestSuite) TestResharding() {
	var callbackErr error

	suite.handler.setConfig(ResharderHandlerConfig{ChunkSize: 0})

	quantity := intDec(10)
	suite.initMocks(quantity, 1)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().NoError(callbackErr)

	actualMetric := suite.getLastPushedMetric()

	start := time.Date(2000, time.January, 1, 0, 0, 10, 0, time.UTC).Local()
	finish := time.Date(2000, time.January, 1, 0, 0, 20, 0, time.UTC).Local()
	wantEnriched := suite.makeCommonEnriched(
		suite.metricsSchema, 0, quantity, start, finish,
		actualMetric.MessageWriteTime, entities.DeltaUsage, suite.metricsSchemaTags,
	)

	suite.EqualValues(wantEnriched, actualMetric)
	suite.assertMocks(suite.T())
}

func (suite *generalResharderTestSuite) TestErrorOversizedMessage() {
	var callbackErr error

	suite.handler.setConfig(ResharderHandlerConfig{ChunkSize: 1})

	suite.oversizedMessagePusher.On("PushOversizedMessage", mock.Anything, mock.Anything, mock.Anything).
		Return(nil)
	suite.oversizedMessagePusher.On("FlushOversized", mock.Anything).
		Return(nil)
	suite.reporter.On("Consumed")

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().NoError(callbackErr)

	pushCall := getLastMockCall("PushOversizedMessage", suite.oversizedMessagePusher.Calls)
	suite.Require().Equal("PushOversizedMessage", pushCall.Method)

	wantIncorrectRawMessage := entities.IncorrectRawMessage{
		Reason:           entities.FailedByTooBigChunk,
		ReasonComment:    "",
		RawMetric:        pushCall.Arguments[2].(entities.IncorrectRawMessage).RawMetric,
		UploadTime:       pushCall.Arguments[2].(entities.IncorrectRawMessage).UploadTime,
		MessageWriteTime: pushCall.Arguments[2].(entities.IncorrectRawMessage).MessageWriteTime,
		MessageOffset:    1,
	}

	suite.EqualValues(wantIncorrectRawMessage, pushCall.Arguments[2])

	suite.assertMocks(suite.T())
}

type storageAggregatingResharderTestSuite struct {
	suite.Suite
	resharderMocks
	metricsMaker

	handler *AggregatingResharderHandler
}

func TestStorageAggregatingResharder(t *testing.T) {
	suite.Run(t, new(storageAggregatingResharderTestSuite))
}

func (suite *storageAggregatingResharderTestSuite) SetupTest() {
	suite.resharderMocks.SetupTest()
	suite.metricsSchema = "s3.api.v1"
	suite.handler = NewAggregatingResharderHandler("general", "resharder", "test/src", ResharderHandlerConfig{}, suite.fabrics)
	suite.handler.mockClock()
}

func (suite *storageAggregatingResharderTestSuite) TestResharding() {
	now := getClock().Now()
	firstMetricTime := now.Add(-59 * time.Minute)
	secondMetricTime := now.Add(-1 * time.Minute)
	suite.Require().EqualValues(firstMetricTime.Truncate(time.Hour), secondMetricTime.Truncate(time.Hour))

	metricsSchemaTags1 := `{"storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":5}`
	metric1 := suite.formatSourceMessage(suite.metricsSchema, firstMetricTime, firstMetricTime, 10, metricsSchemaTags1)
	metricsSchemaTags2 := `{"storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":10}`
	metric2 := suite.formatSourceMessage(suite.metricsSchema, secondMetricTime, secondMetricTime, 15, metricsSchemaTags2)

	expectedSchemaTags := `{"storage_class":"cold","method":"PUT","status_code":"200","net_type":"ingress","transferred":15}`

	suite.pushMessage(metric1, metric2)

	quantity := intDec(25)
	suite.initMocks(quantity, 1)

	suite.handler.setConfig(ResharderHandlerConfig{ChunkSize: 0, MetricLifetime: 12 * time.Hour})
	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)

	actualMetric := suite.getLastPushedMetric()
	suite.Require().NotEmpty(actualMetric)

	expectedUsageTime := firstMetricTime.Truncate(time.Hour).Add(59 * time.Minute).Local()
	expectedEnriched := suite.makeCommonEnriched(
		suite.metricsSchema, 0, quantity, expectedUsageTime, expectedUsageTime,
		actualMetric.MessageWriteTime, suite.metricsUsageType, expectedSchemaTags,
	)
	suite.EqualValues(expectedEnriched, actualMetric)

	suite.assertMocks(suite.T())
}

type cumulativeResharderTestSuite struct {
	suite.Suite
	resharderMocks
	metricsMaker

	new        func(fabrics ResharderFabrics) *CumulativeResharderHandler
	prorated   bool
	quantities []decimal.Decimal128
	handler    *CumulativeResharderHandler
}

func TestCumulativeResharder(t *testing.T) {
	suite.Run(t, &cumulativeResharderTestSuite{
		new: func(f ResharderFabrics) *CumulativeResharderHandler {
			return NewCumulativeResharderHandler("cumulative", "resharder", "test/src", ResharderHandlerConfig{}, f)
		},
		prorated: false,
	})
}

func TestCumulativeResharderProrated(t *testing.T) {
	suite.Run(t, &cumulativeResharderTestSuite{
		new: func(f ResharderFabrics) *CumulativeResharderHandler {
			return NewCumulativeProratedResharderHandler("cumulative-prorated", "resharder", "test/src", ResharderHandlerConfig{}, f)
		},
		prorated: true,
	})
}

func (suite *cumulativeResharderTestSuite) SetupTest() {
	suite.resharderMocks.SetupTest()
	suite.metricsUsageType = entities.CumulativeUsage

	suite.handler = suite.new(suite.fabrics)
	suite.handler.mockClock()
	now := getClock().Now()
	suite.pushMessage(
		suite.formatSourceMessage(suite.metricsSchema, now.Add(time.Second*5), now.Add(time.Second*10), 10, suite.metricsSchemaTags),
		suite.formatSourceMessage(suite.metricsSchema, now.Add(time.Second*10), now.Add(time.Second*20), 15, suite.metricsSchemaTags),
	)
}

func (suite *cumulativeResharderTestSuite) TestResharding() {
	var callbackErr error
	suite.handler.setConfig(ResharderHandlerConfig{ChunkSize: 0})

	quantity2 := intDec(15)
	pricingQuantity := intDec(10)
	proratedPricingQuantity := strDec("9.959606481481481")

	suite.initMocks(quantity2, 1)
	suite.initCumulativeMocks(quantity2, pricingQuantity)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().NoError(callbackErr)

	actualMetric := suite.getLastPushedMetric()
	suite.Require().NotEmpty(actualMetric)

	start := time.Date(2000, time.January, 1, 0, 0, 10, 0, time.UTC).Local()
	finish := time.Date(2000, time.January, 1, 0, 0, 20, 0, time.UTC).Local()
	wantEnriched := suite.makeCommonEnriched(
		suite.metricsSchema, 1, quantity2, start, finish,
		actualMetric.MessageWriteTime, entities.CumulativeUsage, suite.metricsSchemaTags,
	)
	wantEnriched.PricingQuantity = pricingQuantity
	if suite.prorated {
		wantEnriched.PricingQuantity = proratedPricingQuantity
	}

	suite.EqualValues(wantEnriched, actualMetric)
	suite.assertMocks(suite.T())
}

type cumulativeResharderScaleUpTestSuite struct {
	suite.Suite
	resharderMocks
	metricsMaker

	new      func(fabrics ResharderFabrics) *CumulativeResharderHandler
	prorated bool
	handler  *CumulativeResharderHandler
	now      time.Time
}

func TestCumulativeResharderScaleUp(t *testing.T) {
	suite.Run(t, &cumulativeResharderScaleUpTestSuite{
		new: func(f ResharderFabrics) *CumulativeResharderHandler {
			return NewCumulativeResharderHandler("cumulative", "resharder", "test/src", ResharderHandlerConfig{}, f)
		},
		prorated: false,
	})
}

func TestCumulativeResharderProratedScaleUp(t *testing.T) {
	suite.Run(t, &cumulativeResharderScaleUpTestSuite{
		new: func(f ResharderFabrics) *CumulativeResharderHandler {
			return NewCumulativeProratedResharderHandler("cumulative-prorated", "resharder", "test/src", ResharderHandlerConfig{}, f)
		},
		prorated: true,
	})
}

func (suite *cumulativeResharderScaleUpTestSuite) SetupTest() {
	suite.resharderMocks.SetupTest()
	suite.metricsUsageType = entities.CumulativeUsage

	suite.handler = suite.new(suite.fabrics)
	suite.handler.mockClock()
	suite.now = getClock().Now()
}

func (suite *cumulativeResharderScaleUpTestSuite) TestReshardingScaleUp() {
	var callbackErr error
	suite.handler.setConfig(ResharderHandlerConfig{ChunkSize: 0})
	period, _ := time.Parse(time.RFC3339, "2000-01-01T00:00:00+03:00")
	cumulativePeriod := mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period) })

	cases := []struct {
		reason           string
		quantity         int
		delta            decimal.Decimal128
		proratedQuantity decimal.Decimal128
		offset           uint64
	}{
		{"first metric", 15, intDec(15), strDec("14.939409722222222"), 1},
		{"scale up", 18, intDec(3), strDec("2.987881944444444"), 1},
	}
	for _, c := range cases {
		suite.Run(c.reason, func() {
			suite.resetMessages()
			suite.initMocks(intDec(c.quantity), c.offset)
			scope := entities.ProcessingScope{
				SourceName:       "test/src",
				SourceType:       "logbroker-grpc",
				SourceID:         "test-source",
				StartTime:        getClock().Now(),
				Hostname:         "test-host",
				Pipeline:         "resharder",
				MinMessageOffset: 1,
				MaxMessageOffset: 1,
			}
			cumulativeSource := entities.CumulativeSource{
				ResourceID:      "resource_id",
				SkuID:           "sku_id",
				PricingQuantity: intDec(c.quantity),
				MetricOffset:    c.offset,
			}
			cumulativeUsage := entities.CumulativeUsageLog{
				FirstPeriod:  true,
				ResourceID:   "resource_id",
				SkuID:        "sku_id",
				MetricOffset: c.offset,
				Delta:        c.delta,
			}
			suite.cumulativeCalculator.On("CalculateCumulativeUsage", anyCtx, scope, cumulativePeriod, []entities.CumulativeSource{cumulativeSource}).
				Return(1, []entities.CumulativeUsageLog{cumulativeUsage}, nil)

			from := time.Second * time.Duration(int(c.offset)*10)
			to := time.Second * time.Duration((int(c.offset)+1)*10)
			suite.pushMessage(
				suite.formatSourceMessage(suite.metricsSchema, suite.now.Add(from), suite.now.Add(to), c.quantity, suite.metricsSchemaTags),
			)

			suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
			suite.Require().NoError(callbackErr)

			actualMetric := suite.getLastPushedMetric()
			suite.Require().NotEmpty(actualMetric)

			start := time.Date(2000, time.January, 1, 0, 0, int(c.offset)*10, 0, time.UTC).Local()
			finish := time.Date(2000, time.January, 1, 0, 0, (int(c.offset)+1)*10, 0, time.UTC).Local()
			wantEnriched := suite.makeCommonEnriched(
				suite.metricsSchema, int(c.offset)-1, intDec(c.quantity), start, finish,
				actualMetric.MessageWriteTime, entities.CumulativeUsage, suite.metricsSchemaTags,
			)
			wantEnriched.PricingQuantity = c.delta
			wantEnriched.MessageOffset = c.offset
			if suite.prorated {
				wantEnriched.PricingQuantity = c.proratedQuantity
			}

			suite.EqualValues(wantEnriched, actualMetric)
		})
	}
}
