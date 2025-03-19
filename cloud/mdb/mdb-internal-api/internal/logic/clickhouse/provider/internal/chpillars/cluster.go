package chpillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	clustersmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterCH struct {
	Data ClusterCHData `json:"data"`
}

func (c ClusterCH) Validate() error {
	return nil
}

var _ pillars.Marshaler = &ClusterCH{}
var _ clusters.Pillar = &ClusterCH{}

func DefaultUnmanagedPillar() json.RawMessage {
	data := map[string]interface{}{
		"enable_zk_tls": true,
	}
	res, err := json.Marshal(data)
	if err != nil {
		panic(err)
	}

	return res
}

func NewClusterCH() *ClusterCH {
	return &ClusterCH{
		Data: ClusterCHData{
			Unmanaged: DefaultUnmanagedPillar(),
		},
	}
}

func (c *ClusterCH) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal clickhouse cluster pillar: %w", err)
	}

	return raw, err
}

func (c *ClusterCH) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal clickhouse cluster pillar: %w", err)
	}

	return nil
}

type ClusterCHData struct {
	pillars.ClusterData

	Access                    ClusterCHAccess     `json:"access"`
	S3Bucket                  string              `json:"s3_bucket"`
	ClusterPrivateKey         pillars.CryptoKey   `json:"cluster_private_key"`
	Unmanaged                 json.RawMessage     `json:"unmanaged,omitempty"`
	MDBMetrics                *pillars.MDBMetrics `json:"mdb_metrics,omitempty"`
	UseYASMAgent              *bool               `json:"use_yasmagent,omitempty"`
	SuppressExternalYASMAgent bool                `json:"suppress_external_yasmagent,omitempty"`
	ShipLogs                  *bool               `json:"ship_logs,omitempty"`
	Billing                   *pillars.Billing    `json:"billing,omitempty"`
	MDBHealth                 *pillars.MDBHealth  `json:"mdb_health,omitempty"`
	Encryption                *EncryptionSettings `json:"encryption,omitempty"`
	AWS                       json.RawMessage     `json:"aws,omitempty"`
}

type ClusterCHAccess struct {
	UserNetIPV4CIDRs []string `json:"user_net_ipv4_cidrs,omitempty"`
	UserNetIPV6CIDRs []string `json:"user_net_ipv6_cidrs,omitempty"`
}

type EncryptionSettings struct {
	Enabled *bool           `json:"enabled,omitempty"`
	Key     json.RawMessage `json:"key,omitempty"`
}

func (c *ClusterCH) SetEncryption(es clustersmodels.Encryption) {
	c.Data.Encryption = &EncryptionSettings{
		Enabled: pillars.MapOptionalBoolToPtrBool(es.Enabled),
		Key:     es.Key,
	}
}
