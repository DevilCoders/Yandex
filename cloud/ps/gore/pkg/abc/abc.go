package abc

import (
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
)

//go:generate ../../scripts/mockgen.sh Client

type Client interface {
	Authenticate(a auth.AuthType, t string) Client
	GetShifts(from, to time.Time, sid, schID uint64) (shs []Shift, err error)
}

type Person struct {
	Login string `json:"login"`
	UID   string `json:"uid"`
}

type Schedule struct {
	ID   uint64 `json:"id"`
	Name string `json:"name"`
}

type Shift struct {
	ID          int64    `json:"id"`
	Person      Person   `json:"person"`
	Schedule    Schedule `json:"schedule"`
	IsApproved  bool     `json:"is_approved"`
	FromWatcher bool     `json:"from_watcher"`
	IsPrimary   *bool    `json:"is_primary"`
	Start       string   `json:"start_datetime"`
	End         string   `json:"end_datetime"`
	Replaces    []Shift  `json:"replaces"`
}

type Response struct {
	Result []Shift `json:"results"`
}
