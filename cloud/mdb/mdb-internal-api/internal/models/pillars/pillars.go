package pillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type CryptoKey struct {
	Data              string `json:"data"`
	EncryptionVersion int64  `json:"encryption_version"`
}

type MDBMetrics struct {
	Enabled *bool `json:"enabled,omitempty"`
}

func NewDisabledMDBMetrics() *MDBMetrics {
	return &MDBMetrics{Enabled: new(bool)}
}

type Billing struct {
	ShipLogs   *bool `json:"ship_logs,omitempty"`
	UseCloudLB *bool `json:"use_cloud_logbroker,omitempty"`
}

func NewDisabledBilling() *Billing {
	return &Billing{ShipLogs: new(bool)}
}

type MDBHealth struct {
	Nonaggregatable bool `json:"nonaggregatable,omitempty"`
}

type AccessSettings struct {
	DataLens       *bool                `json:"data_lens,omitempty"`
	Metrika        *bool                `json:"metrika,omitempty"`
	Serverless     *bool                `json:"serverless,omitempty"`
	WebSQL         *bool                `json:"web_sql,omitempty"`
	DataTransfer   *bool                `json:"data_transfer,omitempty"`
	YandexQuery    *bool                `json:"yandex_query,omitempty"`
	Ipv4CidrBlocks []clusters.CidrBlock `json:"ipv4_cidr_blocks,omitempty"`
	Ipv6CidrBlocks []clusters.CidrBlock `json:"ipv6_cidr_blocks,omitempty"`
}

func NewMDBHealthWithDisabledAggregate() *MDBHealth {
	return &MDBHealth{Nonaggregatable: true}
}

// Marshaler is used for marshaling and unmarshaling of pillars to raw form that can be stored somewhere.
// Every pillar must implement this interface in order to be able to use common interfaces for loading and
// storing pillars.
type Marshaler interface {
	MarshalPillar() (json.RawMessage, error)
	UnmarshalPillar(raw json.RawMessage) error
}
