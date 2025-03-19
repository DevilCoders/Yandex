package models

import (
	"database/sql"
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

type Request struct {
	ID                 int64
	Name               string
	ExtID              string `db:"request_ext_id"`
	Status             string
	Comment            string
	Author             string
	Type               string           `db:"request_type"`
	Extra              pgtype.JSONB     `db:"extra"`
	Fqdns              pgtype.TextArray `db:"fqdns"`
	CreatedAt          time.Time        `db:"created_at"`
	ResolvedAt         sql.NullTime     `db:"resolved_at"`
	CameBackAt         sql.NullTime     `db:"came_back_at"`
	ResolvedBy         sql.NullString   `db:"resolved_by"`
	AnalysedBy         sql.NullString   `db:"analysed_by"`
	ResolveExplanation string           `db:"resolve_explanation"`
	FailureType        string           `db:"failure_type"`
	ScenarioInfo       pgtype.JSONB     `db:"scenario_info"`
}

// ToInternal casts in-db Request model into internal one
func (task *Request) ToInternal() (models.ManagementRequest, error) {
	out := models.ManagementRequest{
		ID:                 task.ID,
		Name:               task.Name,
		ExtID:              task.ExtID,
		Status:             models.RequestStatus(task.Status),
		Comment:            task.Comment,
		Author:             models.Person(task.Author),
		Type:               task.Type,
		CreatedAt:          task.CreatedAt,
		ResolvedAt:         task.ResolvedAt.Time,
		CameBackAt:         task.CameBackAt.Time,
		ResolvedBy:         models.Person(task.ResolvedBy.String),
		AnalysedBy:         models.Person(task.AnalysedBy.String),
		ResolveExplanation: task.ResolveExplanation,
		FailureType:        task.FailureType,
	}
	if task.ResolvedBy.Valid {
		out.ResolvedBy = models.Person(task.ResolvedBy.String)
	} else {
		out.ResolvedBy = models.PersonUnknown
	}
	if task.AnalysedBy.Valid {
		out.AnalysedBy = models.Person(task.AnalysedBy.String)
	} else {
		out.AnalysedBy = models.PersonUnknown
	}
	if err := task.Fqdns.AssignTo(&out.Fqnds); err != nil {
		return models.ManagementRequest{}, err
	}

	if err := task.Extra.AssignTo(&out.Extra); err != nil {
		return out, err
	}

	if err := task.ScenarioInfo.AssignTo(&out.ScenarioInfo); err != nil {
		return out, err
	}

	return out, nil
}
