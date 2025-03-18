package printf

import (
	"fmt"
	"strings"

	"golang.org/x/tools/go/analysis/passes/printf"
)

var (
	Analyzer = printf.Analyzer

	additionalFuncs = []string{
		// Logger
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Tracef",
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Debugf",
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Infof",
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Warnf",
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Errorf",
		"(a.yandex-team.ru/library/go/core/log.loggerFmt).Fatalf",
	}
)

func init() {
	err := Analyzer.Flags.Set("funcs", strings.Join(additionalFuncs, ","))
	if err != nil {
		panic(fmt.Errorf("failed to configure printf functions: %w", err))
	}
}
