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

type NetworkConnection struct {
	ID           string         `db:"network_connection_id"`
	NetworkID    string         `db:"network_id"`
	ProjectID    string         `db:"project_id"`
	Provider     string         `db:"cloud_provider"`
	Region       string         `db:"region_id"`
	CreateTime   time.Time      `db:"create_time"`
	Description  sql.NullString `db:"description"`
	Status       string         `db:"status"`
	StatusReason sql.NullString `db:"status_reason"`
	Params       pgtype.JSONB   `db:"connection_params"`
}

func (n *NetworkConnection) ToInternal() (models.NetworkConnection, error) {
	out := models.NetworkConnection{
		ID:         n.ID,
		NetworkID:  n.NetworkID,
		ProjectID:  n.ProjectID,
		Region:     n.Region,
		Provider:   models.Provider(n.Provider),
		CreateTime: n.CreateTime,
		Status:     models.NetworkConnectionStatus(n.Status),
	}

	var err error
	out.Params, err = NetworkConnectionParamsToInternal(n.Params, out.Provider)
	if err != nil {
		return out, xerrors.Errorf("network connection params to internal: %w", err)
	}

	if n.Description.Valid {
		out.Description = n.Description.String
	}

	if n.StatusReason.Valid {
		out.StatusReason = n.StatusReason.String
	}

	return out, nil
}

func NetworkConnectionParamsToInternal(dbOML pgtype.JSONB, provider models.Provider) (models.NetworkConnectionParams, error) {
	var result models.NetworkConnectionParams
	switch provider {
	case models.ProviderAWS:
		result = aws.DefaultNetworkConnectionParams()
	}
	if err := json.Unmarshal(dbOML.Bytes, result); err != nil {
		return nil, err
	}
	return result, nil
}

func NetworkConnectionParamsToDB(params models.NetworkConnectionParams) (json.RawMessage, error) {
	return json.Marshal(params)
}
