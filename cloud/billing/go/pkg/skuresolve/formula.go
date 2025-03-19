package skuresolve

import (
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/jmesengine"
)

// ApplyFormula calculates formula with metric values
func ApplyFormula(formula JMESPath, mg MetricValueGetter) (decimal.Decimal128, error) {
	ast, err := parseJMES(formula)
	if err != nil {
		return nanDecimal, fmt.Errorf("JMES parse error: %w", err)
	}

	if ast.hasStatic {
		return decimalFromJmesValue(ast.staticResult)
	}

	fv := mg.GetFullValue()
	ev, err := jmesEngine.Execute(ast.node, jmesengine.Value(fv))
	if err != nil {
		return nanDecimal, fmt.Errorf("formula execution error: %w", err)
	}
	return decimalFromJmesValue(ev)
}

func decimalFromJmesValue(ev jmesengine.ExecValue) (decimal.Decimal128, error) {
	val := ev.Value()
	result := fastjsonDecimal(val)
	if result.IsNan() {
		valStr := string(val.MarshalTo(nil))
		return nanDecimal, fmt.Errorf("invalid formula result %s", valStr)
	}
	return result, nil
}
