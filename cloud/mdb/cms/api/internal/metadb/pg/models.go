package pg

import (
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
)

type operation struct {
	OperationID string       `db:"operation_id"`
	TargetID    string       `db:"target_id"`
	ClusterID   string       `db:"cid"`
	ClusterType string       `db:"cluster_type"`
	Environment string       `db:"env"`
	Type        string       `db:"operation_type"`
	CreatedBy   string       `db:"created_by"`
	CreatedAt   time.Time    `db:"created_at"`
	StartedAt   sql.NullTime `db:"started_at"`
	ModifiedAt  time.Time    `db:"modified_at"`
	Status      string       `db:"status"`
}

func operationFromDB(op operation) (models.Operation, error) {
	res := models.Operation{
		OperationID: op.OperationID,
		TargetID:    op.TargetID,
		ClusterID:   op.ClusterID,
		ClusterType: op.ClusterType,
		Environment: op.Environment,
		Type:        op.Type,
		CreatedBy:   op.CreatedBy,
		CreatedAt:   op.CreatedAt,
		ModifiedAt:  op.ModifiedAt,
		Status:      op.Status,
	}

	if op.StartedAt.Valid {
		res.StartedAt = op.StartedAt.Time
	}

	return res, nil
}
