package importcheck_test

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/yolint/internal/passes/importcheck"
)

func Test(t *testing.T) {
	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, importcheck.Analyzer, "a.yandex-team.ru/lintexamples")
}
