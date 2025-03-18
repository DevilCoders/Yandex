package internal

import "a.yandex-team.ru/library/go/yandex/stq/pkg/worker"

type StqRunnerConnector interface {
	GetRequest() (worker.Request, error)
	SendResponse(worker.Response) error
}
