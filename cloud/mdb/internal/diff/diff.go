package diff

import (
	"reflect"

	"github.com/google/go-cmp/cmp"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const ignoreFlag = "**IGNORE**"

func Full(expected interface{}, actual interface{}) error {
	opt := cmp.FilterValues(func(a, b interface{}) bool {
		s, ok := a.(string)
		s2, ok2 := b.(string)
		return (ok && s == ignoreFlag) || (ok2 && s2 == ignoreFlag)
	}, cmp.Ignore())
	diff := cmp.Diff(expected, actual, opt)
	if diff != "" {
		return xerrors.Errorf("expected %q, actual %q, Diff: %s", expected, actual, diff)
	}

	return nil
}

func OptionalKeys(expected map[string]interface{}, actual map[string]interface{}) error {
	for expectedKey, expectedValue := range expected {
		actualValue, ok := actual[expectedKey]
		if !ok {
			return xerrors.Errorf("key %q is missing from actual value %s", expectedKey, actual)
		}

		if err := Full(expectedValue, actualValue); err != nil {
			return xerrors.Errorf("key %q: %w", expectedKey, err)
		}
	}

	return nil
}

func OptionalKeysNested(expected map[string]interface{}, actual map[string]interface{}) error {
	for expectedKey, expectedValue := range expected {
		actualValue, ok := actual[expectedKey]
		if !ok {
			return xerrors.Errorf("key %q is missing from actual value %s", expectedKey, actual)
		}

		isActualMap := reflect.ValueOf(actualValue).Kind() == reflect.Map
		isExpectedMap := reflect.ValueOf(expectedValue).Kind() == reflect.Map
		if isActualMap != isExpectedMap {
			return xerrors.Errorf("key %q: expected %q, actual %q", expectedKey, expectedValue, actualValue)
		}
		if isActualMap {
			if err := OptionalKeysNested(expectedValue.(map[string]interface{}), actualValue.(map[string]interface{})); err != nil {
				return xerrors.Errorf("key %q: %w", expectedKey, err)
			}
		} else {
			if err := Full(expectedValue, actualValue); err != nil {
				return xerrors.Errorf("key %q: %w", expectedKey, err)
			}
		}
	}

	return nil
}
