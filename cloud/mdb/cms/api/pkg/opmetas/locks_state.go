package opmetas

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
)

type LocksStateMeta struct {
	Locks map[string]*lockcluster.State `json:"locks" yaml:"locks"`
}

func NewLocksStateMeta() *LocksStateMeta {
	return &LocksStateMeta{
		Locks: make(map[string]*lockcluster.State),
	}
}
