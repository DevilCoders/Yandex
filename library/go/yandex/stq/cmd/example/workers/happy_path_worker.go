package workers

import (
	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
)

type HappyPathWorker struct{}

func (w HappyPathWorker) ProcessRequest(request worker.Request) error {
	return nil
}
