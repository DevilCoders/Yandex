package ospillars

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Cluster struct {
	Data ClusterData `json:"data"`
}

var _ pillars.Marshaler = &Cluster{}

func NewCluster() *Cluster {
	pillar := &Cluster{}
	pillar.Data.OpenSearch.Users = make(map[string]UserData)
	return pillar
}

func (p *Cluster) Validate() error {
	return nil
}

func (p *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal opensearch cluster pillar: %w", err)
	}

	return raw, err
}

func (p *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal opensearch cluster pillar: %w", err)
	}
	return nil
}

func (p *Cluster) AddUser(u UserData) error {
	_, ok := p.Data.OpenSearch.Users[u.Name]
	if ok {
		return semerr.AlreadyExistsf("user %q already exists", u.Name)
	}
	p.Data.OpenSearch.Users[u.Name] = u
	return nil
}

func (p *Cluster) GetUser(name string) (UserData, bool) {
	u, ok := p.Data.OpenSearch.Users[name]
	return u, ok
}

func (p *Cluster) SetUser(u UserData) {
	p.Data.OpenSearch.Users[u.Name] = u
}

func (p *Cluster) DeleteUser(name string) error {
	_, ok := p.Data.OpenSearch.Users[name]
	if !ok {
		return semerr.NotFoundf("user %q not found", name)
	}
	delete(p.Data.OpenSearch.Users, name)
	return nil
}

func (p *Cluster) SetPlugins(plugins ...string) error {
	if err := osmodels.ValidatePlugins(plugins); err != nil {
		return err
	}
	p.Data.OpenSearch.Plugins = plugins
	return nil
}

func (p *Cluster) Version() osmodels.Version {
	return p.Data.OpenSearch.Version
}

func (p *Cluster) SetAuthProviders(ap *osmodels.AuthProviders) error {
	return p.Data.OpenSearch.Auth.setAuthProviders(ap)
}

func (p *Cluster) AuthProviders() (*osmodels.AuthProviders, error) {
	return p.Data.OpenSearch.Auth.authProviders()
}

func (p *Cluster) Extensions() *osmodels.Extensions {
	return osmodels.NewExtensions(p.Data.OpenSearch.Extensions)
}

func (p *Cluster) SetExtensions(e *osmodels.Extensions) {
	p.Data.OpenSearch.Extensions = e.GetList()
}

func (p *Cluster) SetAccess(a osmodels.OptionalAccess) {
	if a.Valid && a.Value.DataTransfer.Valid {
		p.Data.Access = &pillars.AccessSettings{
			DataTransfer: &a.Value.DataTransfer.Bool,
		}
	}
}

type ClusterData struct {
	ClusterPrivateKey         pillars.CryptoKey       `json:"cluster_private_key"`
	S3                        json.RawMessage         `json:"s3,omitempty"`
	S3Bucket                  string                  `json:"s3_bucket,omitempty"`
	S3Buckets                 map[string]string       `json:"s3_buckets,omitempty"`
	ServiceAccountID          string                  `json:"service_account_id,omitempty"`
	OpenSearch                CommonClusterData       `json:"opensearch"`
	MDBMetrics                *pillars.MDBMetrics     `json:"mdb_metrics,omitempty"`
	UseYASMAgent              *bool                   `json:"use_yasmagent,omitempty"`
	SuppressExternalYASMAgent bool                    `json:"suppress_external_yasmagent,omitempty"`
	ShipLogs                  *bool                   `json:"ship_logs,omitempty"`
	Billing                   *pillars.Billing        `json:"billing,omitempty"`
	MDBHealth                 *pillars.MDBHealth      `json:"mdb_health,omitempty"`
	Access                    *pillars.AccessSettings `json:"access,omitempty"`
}

type DataNodeSubCluster struct {
	Data DataNodeSubClusterData `json:"data"`
}

var _ pillars.Marshaler = &DataNodeSubCluster{}

func NewDataNodeSubCluster() *DataNodeSubCluster {
	pillar := &DataNodeSubCluster{}
	pillar.Data.OpenSearch.IsData = true
	pillar.Data.OpenSearch.Dashboards.Enabled = true
	return pillar
}

func (p *DataNodeSubCluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal opensearch data node pillar: %w", err)
	}

	return raw, err
}

func (p *DataNodeSubCluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal opensearch data node pillar: %w", err)
	}

	return nil
}

func (p *DataNodeSubCluster) UpdateConfig(config osmodels.DataNodeConfig) bool {
	updated := false
	pillarConfig := &p.Data.OpenSearch.Config.DataNode
	if config.FielddataCacheSize.Valid {
		if !pillarConfig.FielddataCacheSize.Valid ||
			pillarConfig.FielddataCacheSize != config.FielddataCacheSize {
			pillarConfig.FielddataCacheSize = config.FielddataCacheSize
			updated = true
		}
	}
	if config.MaxClauseCount.Valid {
		if !pillarConfig.MaxClauseCount.Valid ||
			pillarConfig.MaxClauseCount != config.MaxClauseCount {
			pillarConfig.MaxClauseCount = config.MaxClauseCount
			updated = true
		}
	}
	if config.ReindexRemoteWhitelist.Valid {
		if !pillarConfig.ReindexRemoteWhitelist.Valid ||
			pillarConfig.ReindexRemoteWhitelist != config.ReindexRemoteWhitelist {
			pillarConfig.ReindexRemoteWhitelist = config.ReindexRemoteWhitelist
			updated = true
		}
	}
	if config.ReindexSSLCAPath.Valid {
		if !pillarConfig.ReindexSSLCAPath.Valid ||
			pillarConfig.ReindexSSLCAPath != config.ReindexSSLCAPath {
			pillarConfig.ReindexSSLCAPath = config.ReindexSSLCAPath
			updated = true
		}
	}
	return updated
}

type MasterNodeSubCluster struct {
	Data MasterNodeSubClusterData `json:"data"`
}

var _ pillars.Marshaler = &MasterNodeSubCluster{}

func NewMasterNodeSubCluster() *MasterNodeSubCluster {
	pillar := &MasterNodeSubCluster{}
	pillar.Data.OpenSearch.IsMaster = true
	return pillar
}

func (p *MasterNodeSubCluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal opensearch master node subcluster pillar: %w", err)
	}

	return raw, err
}

func (p *MasterNodeSubCluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, &p); err != nil {
		return xerrors.Errorf("failed to unmarshal opensearch master node subcluster pillar: %w", err)
	}

	return nil
}

type CommonClusterData struct {
	Version osmodels.Version    `json:"version"`
	Users   map[string]UserData `json:"users"`
	Plugins []string            `json:"plugins,omitempty"`
	Config  struct {
		Common  json.RawMessage `json:"common,omitempty"` // used for adding any configuration params to cluster adhoc
		Logging json.RawMessage `json:"logging,omitempty"`
	} `json:"config"`
	Dashboards  Dashboards `json:"dashboards"`
	Auth        Auth       `json:"auth"`
	AutoBackups struct {
		Enabled bool `json:"enabled"`
	} `json:"auto_backups"`
	Extensions []osmodels.Extension `json:"extensions,omitempty"`
}

type DataNodeSubClusterData struct {
	OpenSearch DataNodeOpenSearch `json:"opensearch"`
}

type MasterNodeSubClusterData struct {
	OpenSearch MasterNodeOpenSearch `json:"opensearch"`
}

type DataNodeOpenSearch struct {
	Config     DataNodeConfig `json:"config"`
	Dashboards Dashboards     `json:"dashboards"`
	IsData     bool           `json:"is_data"`
	IsMaster   bool           `json:"is_master"`
}

type MasterNodeOpenSearch struct {
	Config     MasterNodeConfig `json:"config"`
	Dashboards Dashboards       `json:"dashboards"`
	IsData     bool             `json:"is_data"`
	IsMaster   bool             `json:"is_master"`
}

type DataNodeConfig struct {
	DataNode osmodels.DataNodeConfig `json:"data_node,omitempty"`
}

type MasterNodeConfig struct {
	DataNode MasterNodeConfigData `json:"master_node,omitempty"`
}

type MasterNodeConfigData struct{}

type UserData struct {
	Name     string            `json:"name"`
	Password pillars.CryptoKey `json:"password"`
	Internal bool              `json:"internal,omitempty"`
	Roles    []string          `json:"roles,omitempty"`
}
