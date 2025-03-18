package errcheck_test

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/yolint/internal/passes/errcheck"
)

func Test(t *testing.T) {
	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, errcheck.Analyzer, "a", "github.com/kisielk/errcheck")
}
