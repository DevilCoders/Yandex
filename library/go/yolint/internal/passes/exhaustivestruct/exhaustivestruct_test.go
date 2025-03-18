package exhaustivestruct_test

import (
	"os"
	"path/filepath"
	"testing"

	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/yolint/internal/passes/exhaustivestruct"
)

func TestAll(t *testing.T) {
	wd, err := os.Getwd()
	if err != nil {
		t.Fatalf("Failed to get wd: %s", err)
	}

	testdata := filepath.Join(wd, "testdata")
	analysistest.Run(t, testdata, exhaustivestruct.Analyzer,
		"exstr", "exstr/inner", "exstr/ignored", "exstr/skipped")
}
