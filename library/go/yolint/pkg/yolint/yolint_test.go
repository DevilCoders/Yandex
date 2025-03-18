package yolint

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/tools/go/analysis"
)

func TestYolint(t *testing.T) {
	analyzers := CollectAnalyzers()
	assert.NoError(t, analysis.Validate(analyzers))
}
