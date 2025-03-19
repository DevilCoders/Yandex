package actions

import (
	"context"
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
	"a.yandex-team.ru/cloud/billing/go/pkg/skuresolve"
)

var (
	ErrSchemaToSku   = errsentinel.New("schema to sku error")
	ErrProductToSku  = errsentinel.New("product to sku error")
	ErrSkuFormula    = errsentinel.New("sku formula error")
	ErrMetricCompute = errsentinel.New("metrics compute error")

	// https://st.yandex-team.ru/CLOUD-94625
	proratedSkuBlacklist = map[string]interface{}{
		"3ggtbq6i06bq934j7": true,
		"7ehak58goi5qvj5ru": true,
		"dvupr46hnpndng4l4": true,
		"fhft2jfnqob30a08e": true,
		"met5tu5uscoc0tlnb": true,
		"ni2kmom41tf568dql": true,
		"phc6idd6p47i8eqlp": true,
	}
)

func proratedBlacklisted(skuID string) bool {
	_, result := proratedSkuBlacklist[skuID[3:]]
	return result
}

type SkuMapper interface {
	// For now we want transaction. In future sku info may be moved to separate call and split schemas and products.
	GetSkuResolving(
		ctx context.Context, scope entities.ProcessingScope, schemas []string, productSchemas []string, explicitSkus []string,
	) (
		schemaToSku []entities.SkuResolveRules, productToSku []entities.SkuResolveRules,
		skus []entities.Sku, err error,
	)
}

type UnitsConverter interface {
	ConvertQuantity(
		ctx context.Context, scope entities.ProcessingScope, at time.Time, srcUnit, dstUnit string, quantity decimal.Decimal128,
	) (decimal.Decimal128, error)
}

type CumulativeCalculator interface {
	CalculateCumulativeUsage(
		ctx context.Context, scope entities.ProcessingScope, period entities.UsagePeriod, src []entities.CumulativeSource,
	) (diffCount int, log []entities.CumulativeUsageLog, err error)
}

func ResolveMetricsSku(
	ctx context.Context, scope entities.ProcessingScope, skus SkuMapper, converter UnitsConverter,
	metrics []entities.EnrichedMetric,
) (valid []entities.EnrichedMetric, unresolved []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, unresolved)
		}
	}()

	p := skuProcessor{
		ctx:       ctx,
		scope:     scope,
		mapper:    skus,
		converter: converter,
	}

	return p.resolve(ctx, metrics)
}

func CountCumulativeMonthlyUsage(
	ctx context.Context, scope entities.ProcessingScope, calc CumulativeCalculator,
	metrics []entities.EnrichedMetric,
) (valid []entities.EnrichedMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	p := cumulativeMonthlyProcessor{
		ctx:        ctx,
		scope:      scope,
		calculator: calc,
		prorated:   false,
	}
	return p.calculate(metrics)
}

func CountCumulativeMonthlyUsageProrated(
	ctx context.Context, scope entities.ProcessingScope, calc CumulativeCalculator,
	metrics []entities.EnrichedMetric,
) (valid []entities.EnrichedMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
	}()

	p := cumulativeMonthlyProcessor{
		ctx:        ctx,
		scope:      scope,
		calculator: calc,
		prorated:   true,
	}
	return p.calculate(metrics)
}

const (
	asIsUsageFormula = "usage.quantity"
)

type skuProcessor struct {
	enrichedCommon

	ctx       context.Context
	scope     entities.ProcessingScope
	mapper    SkuMapper
	converter UnitsConverter

	schemaMatcher interface {
		Match(string, skuresolve.MetricValueGetter) ([]string, error)
	}
	productMatcher interface {
		Match(string, skuresolve.MetricValueGetter) ([]string, error)
	}
}

func (sp *skuProcessor) resolve(
	ctx context.Context, metrics []entities.EnrichedMetric,
) ([]entities.EnrichedMetric, []entities.InvalidMetric, error) {
	logger := tooling.Logger(ctx)

	simpleSchemas, productSchemas, explicit := sp.resolvingParams(metrics)
	schemaToSku, productToSku, skus, err := sp.mapper.GetSkuResolving(
		sp.ctx, sp.scope, simpleSchemas, productSchemas, explicit,
	)
	if err != nil {
		return nil, nil, err
	}
	if err = sp.buildMatchers(schemaToSku, productToSku); err != nil {
		return nil, nil, err
	}

	skuMap := make(map[string]int)
	for i := range skus {
		skuID := skus[i].SkuID
		if _, ok := skuMap[skuID]; ok {
			panic("duplicated sku")
		}
		skuMap[skuID] = i
	}

	var resolvedMetrics []entities.EnrichedMetric
	var unresolvedMetrics []entities.InvalidMetric

	for _, m := range metrics {
		mg := &metricValueGetter{metric: &m.SourceMetric}
		metricSkus, err := sp.matchSchemaSku(m, mg)
		if err != nil {
			return nil, nil, ErrSchemaToSku.Wrap(err)
		}
		notResolved := len(metricSkus) == 0
		for _, skuID := range metricSkus {
			skuIDx, ok := skuMap[skuID]
			if !ok {
				panic("inconsistent sku mapping")
			}
			mm, err := sp.copyWithSku(m, mg, skus[skuIDx])
			switch {
			case err == nil:
				resolvedMetrics = append(resolvedMetrics, mm)
			case errors.Is(err, ErrMetricCompute):
				im := sp.makeIncorrectMetric(
					m, entities.FailedBySkuResolving, fmt.Sprintf("SKU %s forumla compute error", skus[skuIDx].SkuName),
				)
				unresolvedMetrics = append(unresolvedMetrics, im)
				logger.Warn("Metric compute error", logf.Error(err), logf.Value(skus[skuIDx].SkuName))
			default:
				return nil, nil, err
			}
		}
		productsMap, err := sp.matchProductSku(m, mg)
		if err != nil {
			return nil, nil, ErrProductToSku.Wrap(err)
		}
		for product, prodSkus := range productsMap {
			notResolved = false
			for _, skuID := range prodSkus {
				skuIDx, ok := skuMap[skuID]
				if !ok {
					panic("inconsistent product sku mapping")
				}
				mm, err := sp.copyWithSku(m, mg, skus[skuIDx])
				switch {
				case err == nil:
					sp.setProduct(&mm, product)
					resolvedMetrics = append(resolvedMetrics, mm)
				case errors.Is(err, ErrMetricCompute):
					im := sp.makeIncorrectMetric(
						m, entities.FailedBySkuResolving, fmt.Sprintf("SKU %s formula error for product %s", skus[skuIDx].SkuName, product),
					)
					unresolvedMetrics = append(unresolvedMetrics, im)
					logger.Warn("Metric compute error", logf.Error(err), logf.Value(skus[skuIDx].SkuName))
				default:
					return nil, nil, err
				}
			}
		}
		if notResolved {
			im := sp.makeIncorrectMetric(
				m, entities.FailedBySkuResolving, fmt.Sprintf("Can not resolve sku for schema %s", m.Schema),
			)
			unresolvedMetrics = append(unresolvedMetrics, im)
		}
		mg.reset()
	}
	return resolvedMetrics, unresolvedMetrics, nil
}

func (sp *skuProcessor) resolvingParams(metrics []entities.EnrichedMetric) (simple, product, skus []string) {
	mark := struct{}{}
	processed := make(map[string]struct{})
	seenSkus := map[string]struct{}{"": mark}
	for _, m := range metrics {
		if _, ok := seenSkus[m.SkuID]; !ok {
			skus = append(skus, m.SkuID)
			seenSkus[m.SkuID] = mark
			// Maybe should continue here
		}

		if _, ok := processed[m.Schema]; ok {
			continue
		}
		processed[m.Schema] = mark

		simple = append(simple, m.Schema)
		if sp.hasProduct(m.Schema) {
			product = append(product, m.Schema)
		}
	}
	return
}

func (skuProcessor) hasProduct(schema string) bool {
	return schema == "compute.vm.generic.v1" // Single schema for now
}

func (sp *skuProcessor) buildMatchers(schemaToSku, productToSku []entities.SkuResolveRules) error {
	schemas := skuresolve.NewMatcher()
	for _, r := range schemaToSku {
		resolvers, err := sp.buildResolvers(r.Rules)
		if err != nil {
			return ErrSchemaToSku.Wrap(err)
		}
		schemas.AddResolvers(r.Key, resolvers...)
	}
	sp.schemaMatcher = schemas

	products := skuresolve.NewMatcher()
	for _, r := range productToSku {
		resolvers, err := sp.buildResolvers(r.Rules)
		if err != nil {
			return ErrProductToSku.Wrap(err)
		}
		products.AddResolvers(r.Key, resolvers...)
	}
	sp.productMatcher = products
	return nil
}

func (skuProcessor) buildResolvers(resolves []entities.SkuResolve) (result []skuresolve.Resolver, err error) {
	for _, r := range resolves {
		if len(r.Rules) == 0 {
			resolver, err := skuresolve.NewPolicyResolver(r.SkuID, skuresolve.JMESPath(r.Policy))
			if err != nil {
				return nil, err
			}
			result = append(result, resolver)
			continue
		}
		matchers := []skuresolve.PathsMatcher{}
		for _, rule := range r.Rules {
			matcher := skuresolve.PathsMatcher{}
			for path, prule := range rule {
				if prule.Type == entities.ExistenceRule {
					matcher[path] = skuresolve.NewExistsMatcher(prule.Exists)
					continue
				}
				switch {
				case prule.Value.Null:
					matcher[path] = skuresolve.NewNullMatcher()
				case prule.Value.EmptyString || prule.Value.String != "":
					matcher[path] = skuresolve.NewStringMatcher(prule.Value.String)
				case prule.Value.True:
					matcher[path] = skuresolve.NewBoolMatcher(true)
				case prule.Value.False:
					matcher[path] = skuresolve.NewBoolMatcher(false)
				default:
					matcher[path] = skuresolve.NewNumberMatcher(prule.Value.Number)
				}
			}
			matchers = append(matchers, matcher)
		}
		resolver, err := skuresolve.NewRulesResolver(r.SkuID, matchers...)
		if err != nil {
			return nil, err
		}
		result = append(result, resolver)
	}
	return result, nil
}

func (sp *skuProcessor) matchSchemaSku(m entities.EnrichedMetric, getter *metricValueGetter) ([]string, error) {
	if m.SkuID != "" {
		return []string{m.SkuID}, nil
	}
	return sp.schemaMatcher.Match(m.Schema, getter)
}

func (sp *skuProcessor) matchProductSku(m entities.EnrichedMetric, getter *metricValueGetter) (map[string][]string, error) {
	if len(m.Products) == 0 {
		return nil, nil
	}
	result := make(map[string][]string)

	for _, product := range m.Products {
		skus, err := sp.productMatcher.Match(product, getter)
		if err != nil {
			return nil, err
		}
		result[product] = skus
	}
	return result, nil
}

func (sp *skuProcessor) copyWithSku(
	m entities.EnrichedMetric, getter *metricValueGetter, sku entities.Sku,
) (entities.EnrichedMetric, error) {
	mm := m.Clone()
	mm.SkuID = sku.SkuID
	mm.SkuInfo = sku.SkuInfo
	err := sp.countUsage(&mm, getter, sku)
	return mm, err
}

func (skuProcessor) setProduct(m *entities.EnrichedMetric, product string) {
	if m.TagsOverride == nil {
		m.TagsOverride = make(map[string]string)
	}
	m.TagsOverride["product_version_id"] = product
	m.Products = []string{product}
}

func (sp *skuProcessor) countUsage(
	m *entities.EnrichedMetric, getter *metricValueGetter, sku entities.Sku,
) (err error) {
	quantity := m.Usage.Quantity
	if sku.Formula != asIsUsageFormula { // shortcut
		if quantity, err = skuresolve.ApplyFormula(skuresolve.JMESPath(sku.Formula), getter); err != nil {
			return ErrMetricCompute.Wrap(err)
		}
	}

	pricingQty, err := sp.converter.ConvertQuantity(
		sp.ctx, sp.scope, m.Usage.UsageTime(), sku.UsageUnit, sku.PricingUnit, quantity,
	)
	if err != nil {
		return err
	}
	m.PricingQuantity = pricingQty
	return nil
}

type cumulativeMonthlyProcessor struct {
	enrichedCommon

	ctx        context.Context
	scope      entities.ProcessingScope
	calculator CumulativeCalculator
	prorated   bool
}

func (cp *cumulativeMonthlyProcessor) calculate(
	metrics []entities.EnrichedMetric,
) ([]entities.EnrichedMetric, error) {
	calculatePeriods := make(map[entities.UsagePeriod][]*entities.EnrichedMetric)

	for i := range metrics {
		m := &metrics[i]

		if m.SkuUsageType != entities.CumulativeUsage {
			continue
		}

		p := cp.usagePeriod(tooling.LocalTime(cp.ctx, m.Usage.UsageTime()))
		calculatePeriods[p] = append(calculatePeriods[p], m)
	}

	for period, periodMetrics := range calculatePeriods {
		if err := cp.calculatePeriod(period, periodMetrics); err != nil {
			return nil, err
		}
	}

	return metrics, nil
}

type cumulativeKey struct {
	resourceID string
	skuID      string
}

func (cp *cumulativeMonthlyProcessor) calculatePeriod(
	period entities.UsagePeriod, metrics []*entities.EnrichedMetric,
) error {
	// NOTE:
	// -------------------------------
	// * In:   cores: 1, 3, 5, 5, 10 ;
	// * Out:  cores: 1, 2, 2, 0, 5 ;
	// -------------------------------

	resourceLargest := make(map[cumulativeKey]*entities.EnrichedMetric)
	for _, m := range metrics {
		key := cumulativeKey{resourceID: m.ResourceID, skuID: m.SkuID}

		cur, ok := resourceLargest[key]
		switch { // largest and earliest
		case !ok:
			resourceLargest[key] = m
		case cur.PricingQuantity.Cmp(m.PricingQuantity) < 0:
			resourceLargest[key] = m
		case cur.PricingQuantity.Cmp(m.PricingQuantity) == 0 && cur.Usage.Finish.After(m.Usage.Finish):
			resourceLargest[key] = m
		}
	}
	source := make([]entities.CumulativeSource, 0, len(resourceLargest))
	for _, m := range resourceLargest {
		source = append(source, entities.CumulativeSource{
			ResourceID:      m.ResourceID,
			SkuID:           m.SkuID,
			PricingQuantity: m.PricingQuantity,
			MetricOffset:    m.MessageOffset,
		})
	}

	log, err := cp.getUsage(period, source)
	if err != nil {
		return err
	}

	for _, m := range metrics {
		key := cumulativeKey{resourceID: m.ResourceID, skuID: m.SkuID}
		if resourceLargest[key] != m {
			m.PricingQuantity = decimal.Decimal128{}
			continue
		}
		if l, ok := log[key]; ok && l.MetricOffset == m.MessageOffset {
			switch {
			case !l.FirstPeriod, !cp.prorated, proratedBlacklisted(l.SkuID):
				m.PricingQuantity = l.Delta
			default:
				m.PricingQuantity = cp.proratedQuantity(tooling.LocalTime(cp.ctx, m.Usage.UsageTime()), l)
			}

			continue
		}
		m.PricingQuantity = decimal.Decimal128{}
	}
	return nil
}

func (cp *cumulativeMonthlyProcessor) getUsage(
	period entities.UsagePeriod, source []entities.CumulativeSource,
) (map[cumulativeKey]entities.CumulativeUsageLog, error) {
	count, log, err := cp.calculator.CalculateCumulativeUsage(cp.ctx, cp.scope, period, source)
	if err != nil {
		return nil, err
	}
	tooling.CumulativeDiffSize(cp.ctx, count)

	result := make(map[cumulativeKey]entities.CumulativeUsageLog, len(log))
	for _, l := range log {
		key := cumulativeKey{resourceID: l.ResourceID, skuID: l.SkuID}
		result[key] = l
	}
	return result, nil
}

func (cumulativeMonthlyProcessor) usagePeriod(locT time.Time) entities.UsagePeriod {
	return entities.UsagePeriod{
		PeriodType: entities.MonthlyUsage,
		Period:     billMonthStartTime(locT),
	}
}

func (cp *cumulativeMonthlyProcessor) proratedQuantity(locAt time.Time, l entities.CumulativeUsageLog) decimal.Decimal128 {
	curMonth := billMonthStartTime(locAt)
	nextMonth := nextBillMonthStartTime(locAt)
	duration := decimal.Must(decimal.FromInt64(int64(nextMonth.Sub(curMonth).Seconds())))
	consumed := decimal.Must(decimal.FromInt64(int64(nextMonth.Sub(locAt).Seconds())))
	return l.Delta.Mul(consumed).Div(duration)
}
