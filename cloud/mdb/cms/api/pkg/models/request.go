package models

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/settings"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Person can be a login of a real user or some stub with no real user, like "wall-e"
type Person string

type RequestStatus string

func (rs RequestStatus) IsGivenAway() bool {
	return rs == StatusOK || rs == StatusRejected
}

const (
	PersonUnknown Person = "unknown person"
)

const (
	StatusOK        RequestStatus = "ok"
	StatusInProcess RequestStatus = "in-process"
	StatusRejected  RequestStatus = "rejected"
)

// Request made by wall-e to maintain host machine
type ManagementRequest struct {
	ID                 int64
	Name               string // change-disk, reboot, etc
	ExtID              string
	Status             RequestStatus
	Comment            string
	Author             Person
	Type               string // manual/automated
	Extra              interface{}
	Fqnds              []string
	CreatedAt          time.Time
	ResolvedAt         time.Time
	CameBackAt         time.Time
	ResolvedBy         Person
	AnalysedBy         Person
	ResolveExplanation string
	FailureType        string
	ScenarioInfo       ScenarioInfo
}

type ScenarioInfo struct {
	ID   int64  `json:"id"`
	Type string `json:"type"`
}

func (r *ManagementRequest) MayBeTakenByWalle() bool {
	return r.Status == StatusOK
}

func (r *ManagementRequest) IsResolvedByAutoDuty() bool {
	return r.AnalysedBy == settings.CMSRobotLogin && r.ResolvedBy == settings.CMSRobotLogin
}

// FQDNs from request.
// Only one FQDN must be. It's a contract.
// https://wiki.yandex-team.ru/wall-e/guide/cms/#diagnosticsc
func (r *ManagementRequest) MustOneFQDN() string {
	if len(r.Fqnds) != 1 {
		panic(xerrors.Errorf("expect one fqdn. got %d", len(r.Fqnds)))
	}
	return r.Fqnds[0]
}

func ScenarioInfoFromAPI(info *models.ManagementRequestScenarioInfo) ScenarioInfo {
	if info == nil {
		return ScenarioInfo{}
	}
	return ScenarioInfo{
		ID:   info.ScenarioID,
		Type: info.ScenarioType,
	}
}
