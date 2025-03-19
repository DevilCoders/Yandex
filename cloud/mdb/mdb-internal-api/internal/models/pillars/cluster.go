package pillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterPillar struct {
	Data ClusterData `json:"data"`
}

var _ Marshaler = &ClusterPillar{}

func (c *ClusterPillar) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal cluster pillar: %w", err)
	}

	return raw, err
}

func (c *ClusterPillar) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal cluster pillar: %w", err)
	}

	return nil
}

type ClusterData struct {
	CloudType environment.CloudType `json:"cloud_type,omitempty"`
	RegionID  string                `json:"region_id,omitempty"`
}
