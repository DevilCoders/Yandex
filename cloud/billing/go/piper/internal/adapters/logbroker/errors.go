package logbroker

import "a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"

var (
	ErrWrite           = errsentinel.New("Write error")
	ErrNoSuchPartition = errsentinel.New("No such partition for topic")

	ErrRateLimited = errsentinel.New("Rate limited")
)
