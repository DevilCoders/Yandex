//go:build !arcadia
// +build !arcadia

package yatest

import (
	"a.yandex-team.ru/library/go/yatool"
)

func doInit() {
	isRunningUnderGoTest = true

	arcadiaSourceRoot, err := yatool.ArcadiaRoot()
	if err != nil {
		panic(err)
	}
	context.Runtime.SourceRoot = arcadiaSourceRoot
}
