package rule

import (
	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Rule is a validation function for a single value
type Rule = func(value *inspection.Inspected) error
