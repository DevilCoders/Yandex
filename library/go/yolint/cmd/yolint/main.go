package main

import (
	"golang.org/x/tools/go/analysis/unitchecker"

	"a.yandex-team.ru/library/go/yolint/pkg/yolint"
)

func main() {
	analyzers := yolint.CollectAnalyzers()
	unitchecker.Main(analyzers...)
}
