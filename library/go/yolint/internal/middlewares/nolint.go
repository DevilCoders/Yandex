package middlewares

import (
	"fmt"

	"golang.org/x/tools/go/analysis"

	"a.yandex-team.ru/library/go/yolint/internal/passes/nolint"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	nolintDoc = `if you believe this report is false positive, please silence it with //nolint:%s comment`
)

// Nolint adds linting disabling capability to analyzer
func Nolint(analyzer *analysis.Analyzer) *analysis.Analyzer {
	nolintAnalyzer := *analyzer
	nolintAnalyzer.Requires = append(analyzer.Requires, nolint.Analyzer)

	nolintAnalyzer.Run = func(pass *analysis.Pass) (interface{}, error) {
		localPass := *pass

		// gather nolint nodes
		nolintNodes := lintutils.ResultOf(&localPass, nolint.Name).(*nolint.Index).ForLinter(analyzer.Name)

		// swap report func
		localPass.Report = func(d analysis.Diagnostic) {
			if dn, ok := lintutils.NodeOfReport(&localPass, d); ok && nolintNodes.Excluded(dn) {
				return
			}

			pass.Report(d)

			d.Message = fmt.Sprintf(nolintDoc, analyzer.Name)
			pass.Report(d)
		}

		return analyzer.Run(&localPass)
	}

	return &nolintAnalyzer
}
