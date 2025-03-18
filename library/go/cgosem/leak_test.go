package cgosem

import (
	"testing"
	"time"

	"a.yandex-team.ru/library/go/cgosem/dummy"
)

func spinC() {
	for {
		dummy.F()
	}
}

func spinSem() {
	for {
		func() {
			defer S.Acquire().Release()
			dummy.F()
		}()
	}
}

func TestLeak(t *testing.T) {
	t.SkipNow()

	for i := 0; i < 30; i++ {
		go spinC()
	}

	time.Sleep(time.Second * 5)
}

func TestLeakFix(t *testing.T) {
	t.SkipNow()

	for i := 0; i < 30; i++ {
		go spinSem()
	}

	time.Sleep(time.Second * 5)
}
