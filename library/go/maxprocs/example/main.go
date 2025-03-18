package main

import (
	"fmt"
	"runtime"

	"a.yandex-team.ru/library/go/maxprocs"
)

func main() {
	maxprocs.AdjustAuto()
	fmt.Printf("GOMAXPROCS is: %d\n", runtime.GOMAXPROCS(0))
}
