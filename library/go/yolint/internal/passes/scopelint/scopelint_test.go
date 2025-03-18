package scopelint

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/test/yatest"
	"a.yandex-team.ru/library/go/yolint/internal/passes/errcheck"
	"a.yandex-team.ru/library/go/yolint/internal/passes/printf"
)

func Test(t *testing.T) {
	// register available analyzers
	Register(errcheck.Analyzer, printf.Analyzer)

	// TODO(prime@): provide helpers accepting relative path.
	flagConfigPath = yatest.SourcePath("library/go/yolint/internal/passes/scopelint/testdata/config.yml")

	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, Analyzer,
		"a.yandex-team.ru/library/go/somepkg",
		"a.yandex-team.ru/library/go/somepkg/subpkg",
		"a.yandex-team.ru/junk/imperator/otherpkg",
	)
}
