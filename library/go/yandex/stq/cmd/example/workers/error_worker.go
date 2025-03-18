package workers

import (
	"fmt"

	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
)

type ErrorWorker struct{}

func (w ErrorWorker) ProcessRequest(_ worker.Request) error {
	return fmt.Errorf("error worker")
}
