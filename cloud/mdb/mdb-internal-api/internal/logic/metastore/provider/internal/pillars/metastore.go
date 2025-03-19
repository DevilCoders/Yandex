package pillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

type ClusterData struct {
	MDBMetrics              *pillars.MDBMetrics   `json:"mdb_metrics,omitempty"`
	MDBHealth               *pillars.MDBHealth    `json:"mdb_health,omitempty"`
	CloudType               environment.CloudType `json:"cloud_type,omitempty"`
	Version                 string                `json:"version,omitempty"`
	UserSubnetIDs           []string              `json:"user_subnet_ids,omitempty"`
	ServiceSubnetIDs        []string              `json:"service_subnet_ids,omitempty"`
	ZoneIDs                 []string              `json:"zone_ids,omitempty"`
	NetworkID               string                `json:"network_id,omitempty"`
	EndpointIP              string                `json:"endpoint_ip,omitempty"`
	MinServersPerZone       int64                 `json:"min_servers_per_zone,omitempty"`
	MaxServersPerZone       int64                 `json:"max_servers_per_zone,omitempty"`
	ServiceAccountID        string                `json:"service_account_id,omitempty"`
	NodeServiceAccountID    string                `json:"node_service_account_id,omitempty"`
	KubernetesClusterID     string                `json:"kubernetes_cluster_id,omitempty"`
	PostgresqlClusterID     string                `json:"postgresql_cluster_id,omitempty"`
	SecretsFolderID         string                `json:"secrets_folder_id,omitempty"`
	PostgresqlHostname      string                `json:"postgresql_hostname,omitempty"`
	KubernetesNamespaceName string                `json:"kubernetes_namespace_name,omitempty"`
	DBName                  string                `json:"db_name,omitempty"`
	DBUserName              string                `json:"db_user_name,omitempty"`
	DBUserPassword          pillars.CryptoKey     `json:"db_user_password,omitempty"`
}

func init() {
}

func NewCluster() *Cluster {
	var pillar Cluster
	return &pillar
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal metastore cluster pillar: %w", err)
	}
	return raw, err
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal metastore cluster pillar: %w", err)
	}
	return nil
}

func (c *Cluster) Validate() error {
	return nil
}

func (c *Cluster) SetVersion(targetVersion string) error {
	if c.Data.Version == "" {
		if targetVersion == "" {
			targetVersion = models.DefaultVersion
		}

		err := models.ValidateVersion(targetVersion)
		if err != nil {
			return err
		}
	}
	c.Data.Version = targetVersion
	return nil
}
