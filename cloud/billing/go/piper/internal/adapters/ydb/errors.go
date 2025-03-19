package ydb

import "a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"

var (
	ErrTransaction   = errsentinel.New("database transaction error")
	ErrSchemeSession = errsentinel.New("scheme session error")

	ErrOversizedPush        = errsentinel.New("oversized push error")
	ErrInvalidPush          = errsentinel.New("invalid push error")
	ErrSchemasRefresh       = errsentinel.New("schemas refresh error")
	ErrSchemasExpired       = errsentinel.New("schemas data expired")
	ErrSchemaData           = errsentinel.New("schema data parse error")
	ErrUnitsRefresh         = errsentinel.New("units refresh error")
	ErrUnitsExpired         = errsentinel.New("units data expired")
	ErrUnitsConversionRule  = errsentinel.New("incorrect units conversion rule")
	ErrCurrency             = errsentinel.New("currency error")
	ErrResolveRulesValue    = errsentinel.New("resolve rules parse error")
	ErrResourceBindingType  = errsentinel.New("incorrect resource binding type")
	ErrCumulativeInit       = errsentinel.New("cumulative init error")
	ErrCumulativePeriodType = errsentinel.New("unsupported cumulative period type")

	ErrUnitRuleNotFound = errsentinel.New("no such unit conversion rule")

	ErrUniqPartitionsRefresh = errsentinel.New("uniq partitions refresh error")
	ErrUniqPartitionsExpired = errsentinel.New("uniq partitions expired")
)
