package metrics

import "a.yandex-team.ru/library/go/core/metrics/nop"

func CreateEmptyRegistry() Registry {
	return new(nop.Registry)
}
