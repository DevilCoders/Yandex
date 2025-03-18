package printf_test

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/yolint/internal/passes/printf"
)

func Test(t *testing.T) {
	patterns := []string{
		"a",
		"b",
		"logger",
		"nofmt",
	}

	testdata := analysistest.TestData()
	_ = printf.Analyzer.Flags.Set("funcs", "Warn,Warnf")
	analysistest.Run(t, testdata, printf.Analyzer, patterns...)
}
