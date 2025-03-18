package middlewares

import (
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"
	"golang.org/x/tools/go/analysis/passes/nilness"
)

func TestNoGen(t *testing.T) {
	testdata := analysistest.TestData()
	analysistest.Run(t, testdata, Nogen(nilness.Analyzer), "nogen/nilness")
}
