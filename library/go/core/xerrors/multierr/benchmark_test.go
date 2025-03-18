package multierr

import (
	"errors"
	"testing"

	"a.yandex-team.ru/library/go/core/xerrors/benchxerrors"
)

func BenchmarkAppend(b *testing.B) {
	var (
		err1 = errors.New("foo")
		err2 = errors.New("bar")
	)

	benchxerrors.RunPerMode(b, func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = Append(err1, err2)
		}
	})
}

func BenchmarkCombine(b *testing.B) {
	var (
		err1 = errors.New("foo")
		err2 = errors.New("bar")
		err3 = errors.New("baz")
	)

	benchxerrors.RunPerMode(b, func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = Combine(err1, err2, err3)
		}
	})

}
