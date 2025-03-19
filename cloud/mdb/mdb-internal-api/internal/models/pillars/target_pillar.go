package pillars

import (
	"encoding/json"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// TargetPillar contains a copy of pillar of the cluster to be restored
// For historical reasons it should be wrapped into {"data":{"restore-from-pillar-data":...}}
type TargetPillar struct {
	Data struct {
		RestoreFromPillarData interface{} `json:"restore-from-pillar-data"`
	} `json:"data"`
}

func MakeTargetPillar(data interface{}) *TargetPillar {
	tp := new(TargetPillar)
	tp.Data.RestoreFromPillarData = data
	return tp
}

func (p *TargetPillar) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal target pillar: %w", err)
	}

	return raw, err
}

func (p *TargetPillar) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal target pillar: %w", err)
	}

	return nil
}
