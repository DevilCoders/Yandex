package models

import (
	"database/sql"
	"encoding/json"
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Operation struct {
	ID          string         `db:"operation_id"`
	ProjectID   string         `db:"project_id"`
	Description sql.NullString `db:"description"`
	CreatedBy   string         `db:"created_by"`
	Metadata    pgtype.JSONB   `db:"metadata"`
	CreateTime  time.Time      `db:"create_time"`
	StartTime   sql.NullTime   `db:"start_time"`
	FinishTime  sql.NullTime   `db:"finish_time"`
	Status      string         `db:"status"`
	State       pgtype.JSONB   `db:"state"`
	Action      string         `db:"action"`
	Provider    string         `db:"cloud_provider"`
	Region      string         `db:"region"`
}

func (op *Operation) ToInternal() (models.Operation, error) {
	out := models.Operation{
		ID:        op.ID,
		ProjectID: op.ProjectID,
		Provider:  models.Provider(op.Provider),
		Region:    op.Region,
		Action:    models.OperationAction(op.Action),
	}

	var err error
	out.State, err = OperationStateToInternal(op.State, out.Action, out.Provider)
	if err != nil {
		return models.Operation{}, xerrors.Errorf("operation state to internal: %w", err)
	}

	out.Params, err = OperationParamsToInternal(op.Metadata, out.Action, out.Provider)
	if err != nil {
		return models.Operation{}, xerrors.Errorf("operation params to internal: %w", err)
	}

	if op.StartTime.Valid {
		out.StartTime = op.StartTime.Time
	}
	if op.FinishTime.Valid {
		out.FinishTime = op.FinishTime.Time
	}
	if op.Description.Valid {
		out.Description = op.Description.String
	}
	return out, nil
}

func OperationStateToInternal(dbOML pgtype.JSONB, action models.OperationAction, provider models.Provider) (models.OperationState, error) {
	var result models.OperationState
	switch provider {
	case models.ProviderAWS:
		switch action {
		case models.OperationActionCreateVPC:
			result = aws.DefaultCreateNetworkOperationState()
		case models.OperationActionDeleteVPC:
			result = aws.DefaultDeleteNetworkOperationState()
		case models.OperationActionCreateNetworkConnection:
			result = aws.DefaultCreateNetworkConnectionOperationState()
		case models.OperationActionDeleteNetworkConnection:
			result = aws.DefaultDeleteNetworkConnectionOperationState()
		case models.OperationActionImportVPC:
			result = aws.DefaultImportVPCOperationState()
		}
	}
	if result == nil {
		return nil, xerrors.New("can not determine state type")
	}
	if err := json.Unmarshal(dbOML.Bytes, result); err != nil {
		return result, err
	}
	return result, nil
}

func OperationStateToDB(state models.OperationState) (json.RawMessage, error) {
	return json.Marshal(state)
}

func OperationParamsToInternal(dbOML pgtype.JSONB, action models.OperationAction, provider models.Provider) (models.OperationParams, error) {
	var result models.OperationParams
	switch action {
	case models.OperationActionCreateVPC:
		result = models.DefaultCreateNetworkOperationParams()
	case models.OperationActionDeleteVPC:
		result = models.DefaultDeleteNetworkOperationParams()
	case models.OperationActionCreateNetworkConnection:
		result = models.DefaultCreateNetworkConnectionOperationParams()
	case models.OperationActionDeleteNetworkConnection:
		result = models.DefaultDeleteNetworkConnectionOperationParams()
	case models.OperationActionImportVPC:
		switch provider {
		case models.ProviderAWS:
			result = aws.DefaultImportVPCOperationParams()
		default:
			return nil, xerrors.Errorf("unknown provider %q", provider)
		}
	default:
		return nil, xerrors.Errorf("unknown action %q", action)
	}
	if err := json.Unmarshal(dbOML.Bytes, &result); err != nil {
		return result, err
	}
	return result, nil
}

func OperationParamsToDB(params models.OperationParams) (json.RawMessage, error) {
	return json.Marshal(params)
}
