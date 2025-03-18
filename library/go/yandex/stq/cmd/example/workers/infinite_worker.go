package workers

import (
	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
	//"time"
)

type InfiniteWorker struct{}

func (w InfiniteWorker) ProcessRequest(_ worker.Request) error {
	//for {
	//	time.Sleep(time.Second)
	//}

	return nil
}
