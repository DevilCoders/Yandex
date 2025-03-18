package imports

import (
	"errors"
)

const (
	DefaultLocalPrefix = "a.yandex-team.ru"
)

func Process(src []byte, opts ...Option) ([]byte, error) {
	f := formatter{
		localPrefix:  DefaultLocalPrefix,
		removeUnused: false,
		processGen:   true,
	}

	for _, opt := range opts {
		opt(&f)
	}

	if f.removeUnused && f.resolver == nil {
		return nil, errors.New("resolver (WithImportsResolver) required")
	}

	return f.Process(src)
}
