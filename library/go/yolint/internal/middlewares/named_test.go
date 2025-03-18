package middlewares

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"
	"golang.org/x/tools/go/analysis/passes/nilness"
)

func TestNamed(t *testing.T) {
	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, Named(nilness.Analyzer), "named/nilness")
}
