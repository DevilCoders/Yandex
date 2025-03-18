package valid

import (
	"a.yandex-team.ru/library/go/valid/v2/inspection"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

// ValueRule is an association between target value and validation rules
type ValueRule struct {
	value *inspection.Inspected
	rules []rule.Rule
}

// Value returns new ValueRule that passes single value through given rules
func Value(target interface{}, rules ...rule.Rule) ValueRule {
	return ValueRule{
		value: inspection.Inspect(target),
		rules: rules,
	}
}

// Validate runs all rules against stored value.
// It is always returns Errors error
func (v ValueRule) Validate() error {
	var errs rule.Errors

	if v.value.Validate != nil {
		err := v.value.Validate()
		errs = append(errs, unwrapErrors(err)...)
	}

	for _, r := range v.rules {
		err := r(v.value)
		errs = append(errs, unwrapErrors(err)...)
	}

	if len(errs) == 0 {
		return nil
	}
	return errs
}

// unwrapErrors flattens multidimensional errors slice
func unwrapErrors(err error) []error {
	if err == nil {
		return nil
	}

	var res []error
	if multierr, ok := err.(interface{ Errors() []error }); ok {
		for _, err := range multierr.Errors() {
			res = append(res, unwrapErrors(err)...)
		}
	} else {
		res = append(res, err)
	}

	return res
}
