package handlers

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/library/go/core/log"
)

type ResharderHandlerConfig struct {
	ChunkSize      int
	MetricLifetime time.Duration
	MetricGrace    time.Duration
}

type OversizedMessagePusher interface {
	actions.OversizedMessagePusher
}

type MetricsResolver interface {
	actions.SkuMapper
	actions.UnitsConverter
	actions.SchemaGetter
}

type BillingAccountsGetter interface {
	actions.BillingAccountsGetter
}

type IdentityResolver interface {
	actions.FoldersGetter
}

type AbcResolver interface {
	actions.AbcResolver
}

type MetricsPusher interface {
	actions.MetricsPusher
	actions.InvalidMetricsPusher
}

type CumulativeCalculator interface {
	actions.CumulativeCalculator
}

type DuplicatesSeeker interface {
	actions.DuplicatesSeeker
}

type E2EPusher interface {
	actions.E2EQuantityPusher
}

type ResharderFabrics struct {
	OversizedMessagePusherFabric func() OversizedMessagePusher
	MetricsResolverFabric        func() MetricsResolver
	BillingAccountsGetterFabric  func() BillingAccountsGetter
	IdentityResolverFabric       func() IdentityResolver
	AbcResolverFabric            func() AbcResolver
	MetricsPusherFabric          func() MetricsPusher
	CumulativeCalculatorFabric   func() CumulativeCalculator
	DuplicatesSeekerFabric       func() DuplicatesSeeker
	E2EPusherFabric              func() E2EPusher
}

type commonResharderHandler struct {
	ResharderFabrics
	commonHandler

	config ResharderHandlerConfig
}

type GeneralResharderHandler struct {
	commonResharderHandler
}

func NewGeneralResharderHandler(
	handlerName string, pipeline string, sourceName string, config ResharderHandlerConfig,
	fabrics ResharderFabrics,
) *GeneralResharderHandler {
	return &GeneralResharderHandler{
		commonResharderHandler: commonResharderHandler{
			ResharderFabrics: fabrics,
			commonHandler:    newCommonHandler(handlerName, sourceName, pipeline),
			config:           config,
		},
	}
}

func (h GeneralResharderHandler) Handle(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) {
	ctx, defFunc, scope := h.init(ctx, sourceID, messages)
	defer defFunc()

	if err := h.handle(ctx, scope, messages.Messages); err != nil {
		tooling.TraceTag(ctx, tracetag.Failed())
		tooling.TraceEvent(ctx, "errors resharding failed", log.Error(err))
		messages.Error(err)
		return
	}

	messages.Consumed()
}

func (h GeneralResharderHandler) handle(
	ctx context.Context, scope entities.ProcessingScope, messages []lbtypes.ReadMessage,
) (err error) {
	defer func() {
		err = tooling.PanicToError(err, recover())
	}()
	ps := generalResharderProcessor{
		ResharderFabrics: h.ResharderFabrics,
		config:           h.config,
		ctx:              ctx,
		scope:            scope,
	}
	parsed := ps.parseSourceMetrics(messages)
	tooling.IncomingMetricsCount(ctx, len(parsed))

	validated := ps.validateSourceMetrics(parsed)
	enriched := ps.enrichMetricsData(validated)

	ps.reportE2EQuantity(enriched)
	ps.pushInvalid()
	ps.push(enriched, h.clock.Now())
	if ps.err == nil {
		tooling.ProcessedMetricsCount(ctx, len(enriched))
	}

	return ps.err
}

type AggregatingResharderHandler struct {
	commonResharderHandler
}

func NewAggregatingResharderHandler(
	handlerName string, pipeline string, sourceName string, config ResharderHandlerConfig,
	fabrics ResharderFabrics,
) *AggregatingResharderHandler {
	return &AggregatingResharderHandler{
		commonResharderHandler: commonResharderHandler{
			ResharderFabrics: fabrics,
			commonHandler:    newCommonHandler(handlerName, sourceName, pipeline),
			config:           config,
		},
	}
}

func (h AggregatingResharderHandler) Handle(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) {
	ctx, defFunc, scope := h.init(ctx, sourceID, messages)
	defer defFunc()

	if err := h.handle(ctx, scope, messages.Messages); err != nil {
		tooling.TraceTag(ctx, tracetag.Failed())
		tooling.TraceEvent(ctx, "errors resharding failed", log.Error(err))
		messages.Error(err)
		return
	}

	messages.Consumed()
}

func (h AggregatingResharderHandler) handle(
	ctx context.Context, scope entities.ProcessingScope, messages []lbtypes.ReadMessage,
) (err error) {
	defer func() {
		err = tooling.PanicToError(err, recover())
	}()
	ps := generalResharderProcessor{
		ResharderFabrics: h.ResharderFabrics,
		config:           h.config,
		ctx:              ctx,
		scope:            scope,
	}
	parsed := ps.parseSourceMetrics(messages)
	tooling.IncomingMetricsCount(ctx, len(parsed))

	validated := ps.validateSourceMetrics(parsed)
	aggregated := ps.aggregateMetrics(validated)
	enriched := ps.enrichMetricsData(aggregated)

	ps.reportE2EQuantity(enriched)
	ps.pushInvalid()
	ps.push(enriched, h.clock.Now())
	if ps.err == nil {
		tooling.ProcessedMetricsCount(ctx, len(enriched))
	}

	return ps.err
}

type CumulativeResharderHandler struct {
	commonResharderHandler
	prorated bool
}

func NewCumulativeResharderHandler(
	handlerName string, pipeline string, sourceName string, config ResharderHandlerConfig,
	fabrics ResharderFabrics,
) *CumulativeResharderHandler {
	return &CumulativeResharderHandler{
		commonResharderHandler: commonResharderHandler{
			ResharderFabrics: fabrics,
			commonHandler:    newCommonHandler(handlerName, sourceName, pipeline),
			config:           config,
		},
		prorated: false,
	}
}

func NewCumulativeProratedResharderHandler(
	handlerName string, pipeline string, sourceName string, config ResharderHandlerConfig,
	fabrics ResharderFabrics,
) *CumulativeResharderHandler {
	return &CumulativeResharderHandler{
		commonResharderHandler: commonResharderHandler{
			ResharderFabrics: fabrics,
			commonHandler:    newCommonHandler(handlerName, sourceName, pipeline),
			config:           config,
		},
		prorated: true,
	}
}

func (h CumulativeResharderHandler) Handle(ctx context.Context, sourceID lbtypes.SourceID, messages *lbtypes.Messages) {
	ctx, defFunc, scope := h.init(ctx, sourceID, messages)
	defer defFunc()

	if err := h.handle(ctx, scope, messages.Messages); err != nil {
		tooling.TraceTag(ctx, tracetag.Failed())
		tooling.TraceEvent(ctx, "errors resharding failed", log.Error(err))
		messages.Error(err)
		return
	}

	messages.Consumed()
}

func (h CumulativeResharderHandler) handle(
	ctx context.Context, scope entities.ProcessingScope, messages []lbtypes.ReadMessage,
) (err error) {
	defer func() {
		err = tooling.PanicToError(err, recover())
	}()
	ps := generalResharderProcessor{
		ResharderFabrics: h.ResharderFabrics,
		config:           h.config,
		ctx:              ctx,
		scope:            scope,
	}
	parsed := ps.parseSourceMetrics(messages)
	tooling.IncomingMetricsCount(ctx, len(parsed))

	validated := ps.validateSourceMetrics(parsed)
	enriched := ps.enrichMetricsData(validated)
	if !h.prorated {
		enriched = ps.processCumulativeMetrics(enriched)
	} else {
		enriched = ps.processCumulativeProratedMetrics(enriched)
	}

	ps.reportE2EQuantity(enriched)
	ps.pushInvalid()
	ps.push(enriched, h.clock.Now())
	if ps.err == nil {
		tooling.ProcessedMetricsCount(ctx, len(enriched))
	}

	return ps.err
}

type generalResharderProcessor struct {
	ResharderFabrics
	sourceMetricsParser

	config ResharderHandlerConfig

	ctx   context.Context
	scope entities.ProcessingScope

	err error

	invalid [][]entities.InvalidMetric
}

func (p *generalResharderProcessor) parseSourceMetrics(messages []lbtypes.ReadMessage) []entities.SourceMetric {
	valid, oversized := filterMessagesBySize(p.config.ChunkSize, messages)

	if len(oversized) > 0 {
		oversizedMetricsPusher := p.OversizedMessagePusherFabric()
		invalidOversizedMetrics := readMessagesToInvalidMetrics(oversized)
		p.err = actions.PushOversizedMessages(p.ctx, p.scope, oversizedMetricsPusher, invalidOversizedMetrics)
		if p.err != nil {
			return nil
		}
	}

	parsed, invalidMetrics := p.parseMessages(valid)
	tooling.InvalidMetrics(p.ctx, invalidMetrics)
	p.addInvalid(invalidMetrics)
	return parsed
}

func (p *generalResharderProcessor) validateSourceMetrics(metrics []entities.SourceMetric) []entities.SourceMetric {
	if p.err != nil || len(metrics) == 0 {
		return nil
	}
	var (
		validMetrics   []entities.SourceMetric
		invalidMetrics []entities.InvalidMetric
	)

	if validMetrics, invalidMetrics, p.err = actions.ValidateMetricsWriteTime(p.ctx, p.scope, p.config.MetricLifetime, metrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)

	metricsResolver := p.MetricsResolverFabric()
	if validMetrics, invalidMetrics, p.err = actions.ValidateMetricsSchema(p.ctx, p.scope, metricsResolver, validMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)

	if validMetrics, invalidMetrics, p.err = actions.ValidateMetricsGrace(p.ctx, p.scope, p.config.MetricGrace, validMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)

	if validMetrics, invalidMetrics, p.err = actions.ValidateMetricsQuantity(p.ctx, p.scope, validMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)

	if validMetrics, invalidMetrics, p.err = actions.ValidateMetricsUnique(p.ctx, p.scope, p.DuplicatesSeekerFabric(), validMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)
	return validMetrics
}

func (p *generalResharderProcessor) aggregateMetrics(metrics []entities.SourceMetric) []entities.SourceMetric {
	if p.err != nil || len(metrics) == 0 {
		return nil
	}
	return actions.Aggregate(p.ctx, metrics)
}

func (p *generalResharderProcessor) processCumulativeMetrics(metrics []entities.EnrichedMetric) []entities.EnrichedMetric {
	if p.err != nil || len(metrics) == 0 {
		return nil
	}
	var enrichedMetrics []entities.EnrichedMetric
	cumulativeCalculator := p.CumulativeCalculatorFabric()
	if enrichedMetrics, p.err = actions.CountCumulativeMonthlyUsage(p.ctx, p.scope, cumulativeCalculator, metrics); p.err != nil {
		return nil
	}
	return enrichedMetrics
}

func (p *generalResharderProcessor) processCumulativeProratedMetrics(metrics []entities.EnrichedMetric) []entities.EnrichedMetric {
	if p.err != nil || len(metrics) == 0 {
		return nil
	}
	var enrichedMetrics []entities.EnrichedMetric
	cumulativeCalculator := p.CumulativeCalculatorFabric()
	if enrichedMetrics, p.err = actions.CountCumulativeMonthlyUsageProrated(p.ctx, p.scope, cumulativeCalculator, metrics); p.err != nil {
		return nil
	}
	return enrichedMetrics
}

func (p *generalResharderProcessor) enrichMetricsData(metrics []entities.SourceMetric) []entities.EnrichedMetric {
	if p.err != nil || len(metrics) == 0 {
		return nil
	}
	var (
		enrichedMetrics []entities.EnrichedMetric
		invalidMetrics  []entities.InvalidMetric
	)
	if enrichedMetrics, p.err = actions.StartEnrichment(p.ctx, p.scope, metrics); p.err != nil {
		return nil
	}

	metricsResolver := p.MetricsResolverFabric()
	if enrichedMetrics, invalidMetrics, p.err = actions.ResolveMetricsSku(p.ctx, p.scope, metricsResolver, metricsResolver, enrichedMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)

	baGetter := p.BillingAccountsGetterFabric()
	identityResolver := p.IdentityResolverFabric()
	abcResolver := p.AbcResolverFabric()
	if enrichedMetrics, invalidMetrics, p.err = actions.ResolveMetricsIdentity(p.ctx, p.scope, baGetter, identityResolver, abcResolver, enrichedMetrics); p.err != nil {
		return nil
	}
	p.addInvalid(invalidMetrics)
	return enrichedMetrics
}

func (p *generalResharderProcessor) reportE2EQuantity(metrics []entities.EnrichedMetric) {
	if p.err != nil || len(metrics) == 0 {
		return
	}
	p.err = actions.ReportE2EQuantity(p.ctx, p.scope, p.E2EPusherFabric(), metrics)
}

func (p *generalResharderProcessor) pushInvalid() {
	if p.err != nil || len(p.invalid) == 0 {
		return
	}
	metricsPusher := p.MetricsPusherFabric()
	p.err = actions.PushInvalidMetrics(p.ctx, p.scope, metricsPusher, p.flattenInvalid())
}

func (p *generalResharderProcessor) push(metrics []entities.EnrichedMetric, now time.Time) {
	if p.err != nil || len(metrics) == 0 {
		return
	}
	if p.err = actions.ReportLag(p.ctx, p.scope, now, true, metrics); p.err != nil {
		return
	}
	metricsPusher := p.MetricsPusherFabric()
	p.err = actions.PushEnrichedMetrics(p.ctx, p.scope, metricsPusher, metrics)
}

func (p *generalResharderProcessor) addInvalid(metrics []entities.InvalidMetric) {
	if len(metrics) > 0 {
		p.invalid = append(p.invalid, metrics)
	}
}

func (p *generalResharderProcessor) flattenInvalid() []entities.InvalidMetric {
	totalLen := 0
	for _, metrics := range p.invalid {
		totalLen += len(metrics)
	}
	if totalLen == 0 {
		return nil
	}
	result := make([]entities.InvalidMetric, 0, totalLen)
	for _, metrics := range p.invalid {
		result = append(result, metrics...)
	}
	return result
}
