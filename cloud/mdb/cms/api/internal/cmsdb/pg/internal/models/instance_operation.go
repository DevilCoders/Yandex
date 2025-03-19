package models

import (
	"encoding/json"
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

type InstanceOperation struct {
	ID                string           `db:"operation_id"`
	Type              string           `db:"operation_type"`
	Status            string           `db:"status"`
	Comment           string           `db:"comment"`
	Author            string           `db:"author"`
	InstanceID        string           `db:"instance_id"`
	CreatedAt         time.Time        `db:"created_at"`
	ModifiedAt        time.Time        `db:"modified_at"`
	Explanation       string           `db:"explanation"`
	Log               string           `db:"operation_log"`
	State             pgtype.JSONB     `db:"operation_state"`
	ExecutedStepNames pgtype.TextArray `db:"executed_step_names"`
}

func (op *InstanceOperation) ToInternal(_ log.Logger) (models.ManagementInstanceOperation, error) {
	state, err := OperationStateToInternal(op.State)
	if err != nil {
		return models.ManagementInstanceOperation{}, err
	}

	out := models.ManagementInstanceOperation{
		ID:          op.ID,
		Type:        models.InstanceOperationType(op.Type),
		Status:      models.InstanceOperationStatus(op.Status),
		Comment:     op.Comment,
		Author:      models.Person(op.Author),
		InstanceID:  op.InstanceID,
		CreatedAt:   op.CreatedAt,
		ModifiedAt:  op.ModifiedAt,
		Explanation: op.Explanation,
		Log:         op.Log,
		State:       state,
	}
	err = op.ExecutedStepNames.AssignTo(&out.ExecutedStepNames)
	if err != nil {
		return models.ManagementInstanceOperation{}, err
	}

	return out, nil
}

func OperationStateToInternal(dbOML pgtype.JSONB) (*models.OperationState, error) {
	result := models.DefaultOperationState()
	if err := json.Unmarshal(dbOML.Bytes, &result); err != nil {
		return result, err
	}
	return result, nil
}

func OperationStateToDB(state *models.OperationState) (json.RawMessage, error) {
	return json.Marshal(state)
}
