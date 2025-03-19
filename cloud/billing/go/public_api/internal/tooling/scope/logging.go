package scope

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
)

func Logger(ctx context.Context) log.Logger {
	return getCurrent(ctx).Logger()
}
