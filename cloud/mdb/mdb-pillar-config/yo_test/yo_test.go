package yotest

import (
	"testing"

	yo_fix "a.yandex-team.ru/library/go/test/checks/yofix"
)

func TestMDBPillarConfig(t *testing.T) {
	yo_fix.Run(t, "g:mdb")
}
