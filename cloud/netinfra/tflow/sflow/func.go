package sflow

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/worker"
	"a.yandex-team.ru/library/go/core/log"
)

func CollectorFunc(j worker.Job, wp chan *worker.Worker, logger log.Logger) {
	w := <-wp
	w.JobChannel <- j
}

func SampleFunc(j worker.Job, out chan consumer.Message, logger log.Logger) {
	_, err := Decode(j.Payload.Raddr.IP, j.Payload.Body, false, out, logger)
	if err != nil {
		logger.Errorf("sflow worker func: %v", err)
	}
}

func EncapFunc(j worker.Job, out chan consumer.Message, logger log.Logger) {
	_, err := Decode(j.Payload.Raddr.IP, j.Payload.Body, true, out, logger)
	if err != nil {
		logger.Errorf("sflow worker func: %v", err)
	}
}
