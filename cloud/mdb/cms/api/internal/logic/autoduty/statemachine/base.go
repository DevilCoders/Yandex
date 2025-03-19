package statemachine

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/library/go/core/log"
)

type StateMachine struct {
	log   log.Logger
	cmsdb cmsdb.Client
}

func NewStateMachine(log log.Logger, cmsdb cmsdb.Client) StateMachine {
	return StateMachine{
		log:   log,
		cmsdb: cmsdb,
	}
}
