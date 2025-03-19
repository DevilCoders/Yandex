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

type Network struct {
	ID                string         `db:"network_id"`
	ProjectID         string         `db:"project_id"`
	Provider          string         `db:"cloud_provider"`
	Region            string         `db:"region_id"`
	CreateTime        time.Time      `db:"create_time"`
	Name              string         `db:"name"`
	Description       sql.NullString `db:"description"`
	IPv4              string         `db:"ipv4_cidr_block"`
	IPv6              string         `db:"ipv6_cidr_block"`
	Status            string         `db:"status"`
	StatusReason      sql.NullString `db:"status_reason"`
	ExternalResources pgtype.JSONB   `db:"external_resources"`
}

func (n *Network) ToInternal() (models.Network, error) {
	out := models.Network{
		ID:         n.ID,
		ProjectID:  n.ProjectID,
		Provider:   models.Provider(n.Provider),
		Region:     n.Region,
		Name:       n.Name,
		CreateTime: n.CreateTime,
		IPv4:       n.IPv4,
		IPv6:       n.IPv6,
		Status:     models.NetworkStatus(n.Status),
	}

	var err error
	out.ExternalResources, err = ExternalResourcesToInternal(n.ExternalResources, out.Provider)
	if err != nil {
		return models.Network{}, xerrors.Errorf("external resources to internal: %w", err)
	}

	if n.Description.Valid {
		out.Description = n.Description.String
	}

	if n.StatusReason.Valid {
		out.StatusReason = n.StatusReason.String
	}

	return out, nil
}

func ExternalResourcesToInternal(dbOML pgtype.JSONB, provider models.Provider) (models.ExternalResources, error) {
	var result models.ExternalResources
	switch provider {
	case models.ProviderAWS:
		result = aws.DefaultNetworkExternalResources()
	}
	if err := json.Unmarshal(dbOML.Bytes, result); err != nil {
		return result, err
	}
	return result, nil
}

func ExternalResourcesToDB(state models.ExternalResources) (json.RawMessage, error) {
	return json.Marshal(state)
}
