package ydb

import (
	"context"
	"encoding/json"
	"strings"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/meta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

func (s *MetaSession) GetSkuResolving(
	ctx context.Context, scope entities.ProcessingScope, schemas []string, productSchemas []string, explicitSkus []string,
) (
	schemaToSku []entities.SkuResolveRules, productToSku []entities.SkuResolveRules,
	skus []entities.Sku, err error,
) {
	var (
		dbSchResolve []meta.SkuBySchemaResolveRow
		dbPrdResolve []meta.SkuByProductResolveRow
		dbSkus       []meta.SkuInfoRow
	)
	{
		retryCtx := tooling.StartRetry(ctx)
		err = s.retryRead(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)

			tx, err := s.adapter.db.BeginTxx(retryCtx, readCommitted())
			if err != nil {
				return err
			}
			defer func() {
				autoTx(tx, resErr)
			}()

			queries := meta.New(tx, s.adapter.queryParams)
			dbSchResolve, dbPrdResolve, dbSkus, err = queries.GetSkuResolvingBySchema(retryCtx, schemas, productSchemas, explicitSkus)
			return err
		})
	}
	if err != nil {
		return
	}

	skuIDs := make(map[string]struct{}, len(dbSkus))
	for _, s := range dbSkus {
		skuIDs[s.ID] = struct{}{}

		es := entities.Sku{}
		es.SkuID = s.ID
		es.SkuName = s.Name
		es.PricingUnit = s.PricingUnit
		es.SkuUsageType = decodeUsageType(s.UsageType)
		es.Formula = entities.JMESPath(s.Formula)
		es.UsageUnit = s.UsageUnit
		skus = append(skus, es)
	}

	skuResolveMap := make(map[string][]entities.SkuResolve)
	for _, r := range dbSchResolve {
		if _, ok := skuIDs[r.SkuID]; !ok {
			continue
		}

		var sr entities.SkuResolve
		sr, err = decodeSkuResolve(r.SkuID, r.ResolvingRules, string(r.ResolvingPolicy))
		if err != nil {
			err = ErrResolveRulesValue.Wrap(err)
		}
		skuResolveMap[r.Schema] = append(skuResolveMap[r.Schema], sr)
	}
	for k, v := range skuResolveMap {
		schemaToSku = append(schemaToSku, entities.SkuResolveRules{Key: k, Rules: v})
		delete(skuResolveMap, k)
	}

	for _, r := range dbPrdResolve {
		if _, ok := skuIDs[r.SkuID]; !ok {
			continue
		}

		var sr entities.SkuResolve
		sr, err = decodeSkuResolve(r.SkuID, r.ResolvingRules, string(r.CheckFormula))
		if err != nil {
			err = ErrResolveRulesValue.Wrap(err)
		}
		skuResolveMap[r.ProductID] = append(skuResolveMap[r.ProductID], sr)
	}
	for k, v := range skuResolveMap {
		productToSku = append(productToSku, entities.SkuResolveRules{Key: k, Rules: v})
		delete(skuResolveMap, k)
	}

	return
}

func decodeUsageType(value string) entities.UsageType {
	switch value {
	case "delta":
		return entities.DeltaUsage
	case "cumulative":
		return entities.CumulativeUsage
	case "gauge":
		return entities.GaugeUsage
	}
	return entities.UnknownUsage
}

func decodeSkuResolve(sku string, rules qtool.JSONAnything, policy string) (result entities.SkuResolve, err error) {
	var dbRules matchRules
	if err = json.Unmarshal([]byte(rules), &dbRules); err != nil {
		return
	}

	for _, r := range dbRules {
		result.Rules = append(result.Rules, decodeMatchRule(r))
	}

	result.SkuID = sku
	result.Policy = entities.JMESPath(policy)
	return
}

func decodeMatchRule(r matchRuleByPath) (result entities.MatchRulesByPath) {
	result = make(entities.MatchRulesByPath, len(r.Options))

	for path, match := range r.Options {
		if match.Exists.parsed {
			result[path] = entities.MatchRule{Type: entities.ExistenceRule, Exists: match.Exists.value}
			continue
		}
		result[path] = entities.MatchRule{
			Type: entities.ValueRule,
			Value: entities.ValueMatch{
				Null:        match.Value.null,
				EmptyString: match.Value.emptyString,
				String:      match.Value.str,
				Number:      match.Value.number,
				True:        match.Value.isTrue,
				False:       match.Value.isFalse,
			},
		}
	}
	return
}

type matchRules []matchRuleByPath

func (r *matchRules) UnmarshalJSON(data []byte) error {
	if len(data) < 2 || data[0] != '[' {
		return nil
	}

	return json.Unmarshal(data, (*[]matchRuleByPath)(r))
}

type matchRuleByPath struct {
	Options map[string]matcher
}

type matcher struct {
	Value  matcherValue
	Exists optionalBool
}

type optionalBool struct {
	value  bool
	parsed bool
}

func (b *optionalBool) UnmarshalJSON(data []byte) error {
	b.parsed = true
	return json.Unmarshal(data, &b.value)
}

type matcherValue struct {
	null        bool
	emptyString bool
	str         string
	number      decimal.Decimal128
	isTrue      bool
	isFalse     bool
}

func (v *matcherValue) UnmarshalJSON(data []byte) (err error) {
	str := string(data)
	switch {
	case str == "null":
		v.null = true
	case strings.HasPrefix(str, `"`):
		if v.number.UnmarshalJSON(data) == nil {
			return
		}
		v.number = decimal.Decimal128{}
		if err = json.Unmarshal(data, &v.str); err != nil {
			return
		}
		v.emptyString = v.str == ""
	case str == "true":
		v.isTrue = true
	case str == "false":
		v.isFalse = true
	default:
		return v.number.UnmarshalJSON(data)
	}
	return nil
}
