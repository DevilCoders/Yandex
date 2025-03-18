package naming_test

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/yolint/internal/passes/naming"
)

func Test(t *testing.T) {
	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, naming.Analyzer, "github.com/golang/lint")
}
