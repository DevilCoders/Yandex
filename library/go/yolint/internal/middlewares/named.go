package middlewares

import (
	"fmt"

	"golang.org/x/tools/go/analysis"
)

// NamedAnalyzers prepends analyzer name to their diagnostic message
func NamedAnalyzers(analyzers ...*analysis.Analyzer) []*analysis.Analyzer {
	out := make([]*analysis.Analyzer, len(analyzers))
	for i, analyzer := range analyzers {
		out[i] = Named(analyzer)
	}
	return out
}

// Named prepends analyzer name to its diagnostic message
func Named(analyzer *analysis.Analyzer) *analysis.Analyzer {
	localAnalyzer := *analyzer

	localAnalyzer.Run = func(pass *analysis.Pass) (interface{}, error) {
		localPass := *pass

		// swap report func
		localPass.Report = func(d analysis.Diagnostic) {
			if analyzer.Name != "" {
				d.Message = fmt.Sprintf("%s: %s", analyzer.Name, d.Message)
			}
			pass.Report(d)
		}

		return analyzer.Run(&localPass)
	}

	return &localAnalyzer
}
