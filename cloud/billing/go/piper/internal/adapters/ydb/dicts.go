package ydb

import (
	"context"
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/meta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/ctxtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

func (s *MetaSession) GetMetricSchema(
	ctx context.Context, scope entities.ProcessingScope, schemaName string,
) (schema entities.MetricsSchema, err error) {
	if len(s.schemas.tags) == 0 { // use consistent schemas information in session
		s.schemas, err = s.adapter.schemas.getData(ctxtool.WithGlobalCancel(ctx, s.adapter.runCtx), s.adapter.db, s.adapter.queryParams)
		if errors.Is(err, ErrSchemasExpired) {
			return
		}
		if errors.Is(err, ErrSchemasRefresh) { // skip refresh errors during some grace time
			tooling.Logger(ctx).Error("schemas refresh temporary error", logf.Error(err))
			err = nil
		}
	}
	if err != nil {
		return
	}
	idx, ok := s.schemas.namesIndex[schemaName]
	if !ok {
		// Empty schema if not found
		return
	}

	schema.Schema = schemaName
	schema.RequiredTags = s.schemas.tags[idx].Required
	return
}

func (s *MetaSession) ConvertQuantity(
	ctx context.Context, scope entities.ProcessingScope, at time.Time, srcUnit, dstUnit string, quantity decimal.Decimal128,
) (result decimal.Decimal128, err error) {
	// TODO: shortcut for srcUnit==dstUnit can be placed here, but this can change some behavior against python impl
	if len(s.units.units) == 0 {
		s.units, err = s.adapter.units.getData(ctxtool.WithGlobalCancel(ctx, s.adapter.runCtx), s.adapter.db, s.adapter.queryParams, false)
		if errors.Is(err, ErrUnitsExpired) {
			return
		}
		if errors.Is(err, ErrUnitsRefresh) { // skip refresh errors during some grace time
			tooling.Logger(ctx).Error("units refresh temporary error", logf.Error(err))
			err = nil
		}
	}
	if err != nil {
		return
	}
	idx, ok := s.units.unitsIndex[keyPair{srcUnit, dstUnit}]
	if !ok && srcUnit == dstUnit {
		return quantity, nil
	}
	if !ok {
		s.units, err = s.adapter.units.getData(ctxtool.WithGlobalCancel(ctx, s.adapter.runCtx), s.adapter.db, s.adapter.queryParams, true)
		if err != nil {
			return
		}
		idx, ok = s.units.unitsIndex[keyPair{srcUnit, dstUnit}]
	}
	if !ok {
		err = ErrUnitRuleNotFound.Wrap(fmt.Errorf("%s to %s", srcUnit, dstUnit))
		return
	}

	rule := s.units.units[idx]
	result, err = s.convertQuantity(ctx, quantity, rule, at)
	return
}

const (
	defaultConversionRule       = "default"
	monthConversionRule         = "calendar_month"
	multiCurrencyConversionRule = "multi_currency"
)

func (s *MetaSession) convertQuantity(
	ctx context.Context, value decimal.Decimal128, rule meta.UnitRow, at time.Time,
) (decimal.Decimal128, error) {
	if rule.SrcUnit == rule.DstUnit {
		return value, nil
	}

	switch rule.Type {
	case defaultConversionRule:
	case monthConversionRule:
		value = value.Mul(s.monthCorrection(tooling.LocalTime(ctx, at)))
	case multiCurrencyConversionRule:
		coeff, err := s.monetaryCoefficient(rule.SrcUnit, rule.DstUnit, at)
		if err != nil {
			return nanDecimal, err
		}
		value = value.Mul(coeff)
	default:
		return nanDecimal, ErrUnitsConversionRule.Wrap(fmt.Errorf("type=%s", rule.Type))
	}

	factor := decimal.Must(decimal.FromInt64(int64(rule.Factor)))
	if rule.Reverse {
		return value.Mul(factor), nil
	}
	return value.Div(factor), nil
}

func (s *MetaSession) monthCorrection(locAt time.Time) decimal.Decimal128 {
	curMon := timetool.MonthStart(locAt)
	nextMon := curMon.AddDate(0, 1, 0)

	sec := int64(nextMon.Sub(curMon).Seconds())
	dSec := decimal.Must(decimal.FromInt64(sec))

	return secInBaseMonth.Div(dSec)
}

func (s *MetaSession) monetaryCoefficient(srcUnit, dstUnit string, at time.Time) (result decimal.Decimal128, err error) {
	result = nanDecimal

	srcCurr := unitToCurrency(srcUnit)
	if srcCurr == "" {
		err = ErrCurrency.Wrap(fmt.Errorf("unknown %s", srcUnit))
		return
	}
	dstCurr := unitToCurrency(dstUnit)
	if dstCurr == "" {
		err = ErrCurrency.Wrap(fmt.Errorf("unknown %s", dstUnit))
		return
	}
	if srcCurr == dstCurr {
		err = ErrCurrency.Wrap(fmt.Errorf("can not convert from %s to %s", srcCurr, dstCurr))
		return
	}

	rates := s.units.conversions[keyPair{srcCurr, dstCurr}]

	rateIdx := -1 // if rates count will be increased change to sort.Search
	for i, r := range rates {
		if time.Time(r.EffectiveTime).After(at) {
			break
		}
		rateIdx = i
	}
	if rateIdx < 0 {
		err = ErrCurrency.Wrap(fmt.Errorf("no conversion rate from %s to %s", srcCurr, dstCurr))
	}

	result = decimal.Decimal128(rates[rateIdx].Multiplier)
	return
}

const (
	rubleCode = "rub"
	kopekCode = "kop"
	usdCode   = "usd"
	centCode  = "cent"
	tengeCode = "kzt"
	tiynCode  = "tiyn"
)

func unitToCurrency(unit string) string {
	switch unit {
	case rubleCode, kopekCode:
		return rubleCode
	case usdCode, centCode:
		return usdCode
	case tengeCode, tiynCode:
		return tengeCode
	}
	return ""
}
