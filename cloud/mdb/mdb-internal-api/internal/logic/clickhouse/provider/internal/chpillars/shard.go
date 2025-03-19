package chpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ShardCH struct {
	Data ShardCHData `json:"data"`
}

func (s ShardCH) Validate() error {
	return nil
}

var _ pillars.Marshaler = &ShardCH{}

func (s *ShardCH) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(s)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal clickhouse shard pillar: %w", err)
	}

	return raw, err
}

func (s *ShardCH) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, s); err != nil {
		return xerrors.Errorf("failed to unmarshal clickhouse shard pillar: %w", err)
	}

	return nil
}

type ShardCHData struct {
	ClickHouse ShardCHServer `json:"clickhouse"`
}

type ShardCHServer struct {
	Config ClickHouseConfig `json:"config"`
}
