package rpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Host struct {
	Data HostData `json:"data"`
}

var _ pillars.Marshaler = &Host{}

func (c *Host) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal redis host pillar: %w", err)
	}

	return raw, err
}

func (c *Host) UnmarshalPillar(raw json.RawMessage) error {
	if len(raw) == 0 {
		return nil
	}
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal redis host pillar: %w", err)
	}
	return nil
}

type HostData struct {
	Redis HostDataRedis `json:"redis"`
}

type HostDataRedis struct {
	Config HostConfig `json:"config"`
}

type HostConfig struct {
	ReplicaPriority optional.Int64 `json:"replica-priority,omitempty" name:"replica-priority"`
}

func (c *Host) SetReplicaPriority(priority optional.Int64) {
	if priority.Valid {
		c.Data.Redis.Config.ReplicaPriority = priority
	} else {
		c.Data.Redis.Config.ReplicaPriority = optional.NewInt64(rmodels.DefaultReplicaPriority)
	}
}
