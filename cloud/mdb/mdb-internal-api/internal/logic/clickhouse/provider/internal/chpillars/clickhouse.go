package chpillars

import (
	"encoding/json"
	"reflect"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type SubClusterCH struct {
	Data SubClusterCHData `json:"data"`
}

var _ pillars.Marshaler = &SubClusterCH{}

func NewSubClusterCH() *SubClusterCH {
	return &SubClusterCH{
		Data: SubClusterCHData{
			ClickHouse: SubClusterCHServer{
				DBs:           []string{},
				Users:         map[string]User{},
				Shards:        map[string]Shard{},
				MLModels:      map[string]MLModel{},
				FormatSchemas: map[string]FormatSchema{},
				ZKUsers:       map[string]ZooKeeperCredentials{},
				SystemUsers:   map[string]SystemUserCredentials{},
				ShardGroups:   map[string]ShardGroup{},
			},
		},
	}
}

func NewSubClusterCHWithVersion(v string) *SubClusterCH {
	res := NewSubClusterCH()
	res.Data.ClickHouse.Version = v
	return res
}

func (p *SubClusterCH) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(p)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal clickhouse subcluster pillar: %w", err)
	}

	return raw, err
}

func (p *SubClusterCH) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, p); err != nil {
		return xerrors.Errorf("failed to unmarshal clickhouse subcluster pillar: %w", err)
	}

	if p.Data.ClickHouse.ShardGroups == nil {
		p.Data.ClickHouse.ShardGroups = map[string]ShardGroup{}
	}

	return nil
}

func (p *SubClusterCH) AddDatabase(db chmodels.DatabaseSpec) error {
	for _, name := range p.Data.ClickHouse.DBs {
		if name == db.Name {
			return semerr.AlreadyExistsf("database %q already exists", name)
		}
	}

	p.Data.ClickHouse.DBs = append(p.Data.ClickHouse.DBs, db.Name)
	return nil
}

func (p *SubClusterCH) DeleteDatabase(dbName string) error {
	dbIndex := -1
	for i, name := range p.Data.ClickHouse.DBs {
		if name == dbName {
			dbIndex = i
			break
		}
	}
	if dbIndex == -1 {
		return semerr.NotFoundf("database %q does not exist", dbName)
	}

	for _, v := range p.Data.ClickHouse.Users {
		delete(v.Databases, dbName)
	}

	p.Data.ClickHouse.DBs = append(p.Data.ClickHouse.DBs[:dbIndex], p.Data.ClickHouse.DBs[dbIndex+1:]...)

	return nil
}

type SubClusterCHData struct {
	Access           *AccessSettings        `json:"access,omitempty"`
	Backup           backups.BackupSchedule `json:"backup"`
	CHBackup         json.RawMessage        `json:"ch_backup,omitempty"`
	ClickHouse       SubClusterCHServer     `json:"clickhouse"`
	ServiceAccountID *string                `json:"service_account_id,omitempty"`
	CloudStorage     CloudStorage           `json:"cloud_storage"`
	UseCHBackup      *bool                  `json:"use_ch_backup,omitempty"`
	MonrunConfig     json.RawMessage        `json:"monrun,omitempty"`
	Unmanaged        json.RawMessage        `json:"unmanaged,omitempty"`
	TestingRepos     *bool                  `json:"testing_repos,omitempty"`
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
	// List of hosts with special network routing rules. Output traffic from ClickHouse hosts to these hosts
	// are routed through underlay network.
	UserHosts json.RawMessage `json:"user_hosts,omitempty"`
	// List of hosts to add to permissive firewall rules. Output traffic from ClickHouse hosts to these hosts
	// are allowed.
	ManagementHosts json.RawMessage `json:"management_hosts,omitempty"`
}

func (as *AccessSettings) toModel() clusters.Access {
	if as == nil {
		return clusters.Access{}
	}

	return clusters.Access{
		DataLens:       pillars.MapPtrBoolToOptionalBool(as.DataLens),
		WebSQL:         pillars.MapPtrBoolToOptionalBool(as.WebSQL),
		Metrica:        pillars.MapPtrBoolToOptionalBool(as.Metrika),
		Serverless:     pillars.MapPtrBoolToOptionalBool(as.Serverless),
		DataTransfer:   pillars.MapPtrBoolToOptionalBool(as.DataTransfer),
		YandexQuery:    pillars.MapPtrBoolToOptionalBool(as.YandexQuery),
		Ipv4CidrBlocks: as.Ipv4CidrBlocks,
		Ipv6CidrBlocks: as.Ipv6CidrBlocks,
	}
}

func (p *SubClusterCH) SetAccess(as clusters.Access) {
	if as.DataLens.Valid || as.Metrica.Valid || as.Serverless.Valid || as.WebSQL.Valid ||
		as.DataTransfer.Valid || as.YandexQuery.Valid ||
		as.Ipv4CidrBlocks != nil || as.Ipv6CidrBlocks != nil {
		p.Data.Access = &AccessSettings{
			DataLens:       pillars.MapOptionalBoolToPtrBool(as.DataLens),
			WebSQL:         pillars.MapOptionalBoolToPtrBool(as.WebSQL),
			Metrika:        pillars.MapOptionalBoolToPtrBool(as.Metrica),
			Serverless:     pillars.MapOptionalBoolToPtrBool(as.Serverless),
			DataTransfer:   pillars.MapOptionalBoolToPtrBool(as.DataTransfer),
			YandexQuery:    pillars.MapOptionalBoolToPtrBool(as.YandexQuery),
			Ipv4CidrBlocks: as.Ipv4CidrBlocks,
			Ipv6CidrBlocks: as.Ipv6CidrBlocks,
		}
	}
}

type SubClusterCHServer struct {
	Users                  map[string]User         `json:"users"`
	Config                 ClickHouseConfig        `json:"config"`
	Shards                 map[string]Shard        `json:"shards"`
	DBs                    []string                `json:"databases"`
	MLModels               map[string]MLModel      `json:"models"`
	Version                string                  `json:"ch_version"`
	InterserverCredentials InterserverCredentials  `json:"interserver_credentials"`
	ShardGroups            map[string]ShardGroup   `json:"shard_groups,omitempty"`
	MysqlProtocol          *bool                   `json:"mysql_protocol"`
	PostgresqlProtocol     *bool                   `json:"postgresql_protocol"`
	EmbeddedKeeper         *bool                   `json:"embedded_keeper,omitempty"`
	SQLUserManagement      *bool                   `json:"sql_user_management"`
	SQLDatabaseManagement  *bool                   `json:"sql_database_management"`
	AdminPassword          *AdminPassword          `json:"admin_password,omitempty"`
	AllowTLS11             *bool                   `json:"allow_tlsv1_1,omitempty"`
	FormatSchemas          map[string]FormatSchema `json:"format_schemas"`
	UserManagementV2       *bool                   `json:"user_management_v2,omitempty"`

	// It is used only in early created clusters.
	// Later we will migrate to KeeperHosts field and remove it.
	ZKHosts []string `json:"zk_hosts,omitempty"`

	// Zookeeper or Clickhouse Keeper map with `hostname -> server_id`
	KeeperHosts map[string]int `json:"keeper_hosts,omitempty"`

	ZKUsers     map[string]ZooKeeperCredentials  `json:"zk_users,omitempty"`
	SystemUsers map[string]SystemUserCredentials `json:"system_users,omitempty"`
	Profiles    json.RawMessage                  `json:"profiles,omitempty"`
	ClusterName *string                          `json:"cluster_name,omitempty"`
	Unmanaged   json.RawMessage                  `json:"unmanaged,omitempty"`
}

type AdminPassword struct {
	Hash     pillars.CryptoKey `json:"hash"`
	Password pillars.CryptoKey `json:"password"`
}

type InterserverCredentials struct {
	User     string            `json:"user"`
	Password pillars.CryptoKey `json:"password"`
}

type ZooKeeperCredentials struct {
	Password pillars.CryptoKey `json:"password"`
}

type SystemUserCredentials struct {
	Hash     pillars.CryptoKey `json:"hash"`
	Password pillars.CryptoKey `json:"password"`
}

type CloudStorage struct {
	Enabled  *bool                 `json:"enabled,omitempty"`
	S3       *CloudStorageS3       `json:"s3,omitempty"`
	Settings *CloudStorageSettings `json:"settings,omitempty"`
}

type CloudStorageS3 struct {
	Bucket          string             `json:"bucket"`
	Scheme          string             `json:"scheme,omitempty"`
	Endpoint        string             `json:"endpoint,omitempty"`
	AccessKeyID     *pillars.CryptoKey `json:"access_key_id,omitempty"`
	AccessSecretKey *pillars.CryptoKey `json:"access_secret_key,omitempty"`
}

type CloudStorageSettings struct {
	ConnectTimeoutMS       *int64   `json:"connect_timeout_ms,omitempty"`
	RequestTimeoutMS       *int64   `json:"request_timeout_ms,omitempty"`
	MetadataPath           *string  `json:"metadata_path,omitempty"`
	CacheEnabled           *bool    `json:"cache_enabled,omitempty"`
	MinMultiPartUploadSize *int64   `json:"min_multi_part_upload_size,omitempty"`
	MinBytesForSeek        *int64   `json:"min_bytes_for_seek,omitempty"`
	SkipAccessCheck        *bool    `json:"skip_access_check,omitempty"`
	MaxConnections         *int64   `json:"max_connections,omitempty"`
	SendMetadata           *bool    `json:"send_metadata,omitempty"`
	ThreadPoolSize         *int64   `json:"thread_pool_size,omitempty"`
	ListObjectKeysSize     *int64   `json:"list_object_keys_size,omitempty"`
	DataCacheEnabled       *bool    `json:"data_cache_enabled,omitempty"`
	DataCacheMaxSize       *int64   `json:"data_cache_max_size,omitempty"`
	MoveFactor             *float64 `json:"move_factor,omitempty"`
}

type MLModel struct {
	Type chmodels.MLModelType `json:"type"`
	URI  string               `json:"uri"`
}

type FormatSchema struct {
	Type chmodels.FormatSchemaType `json:"type"`
	URI  string                    `json:"uri"`
}

func (p *SubClusterCH) AddMLModel(name string, modelType chmodels.MLModelType, uri string) error {
	for modelName := range p.Data.ClickHouse.MLModels {
		if name == modelName {
			return semerr.AlreadyExistsf("ML model %q already exists", name)
		}
	}

	model := MLModel{Type: modelType, URI: uri}
	p.Data.ClickHouse.MLModels[name] = model
	return nil
}

func (p *SubClusterCH) UpdateMLModel(name, uri string) error {
	model, ok := p.Data.ClickHouse.MLModels[name]
	if !ok {
		return semerr.NotFoundf("ML model %q not found", name)
	}

	model.URI = uri
	p.Data.ClickHouse.MLModels[name] = model
	return nil
}

func (p *SubClusterCH) DeleteMLModel(name string) error {
	_, ok := p.Data.ClickHouse.MLModels[name]
	if !ok {
		return semerr.NotFoundf("ML model %q not found", name)
	}

	delete(p.Data.ClickHouse.MLModels, name)
	return nil
}

func (p *SubClusterCH) AddFormatSchema(name string, schemaType chmodels.FormatSchemaType, uri string) error {
	for formatSchemaName := range p.Data.ClickHouse.FormatSchemas {
		if name == formatSchemaName {
			return semerr.AlreadyExistsf("format schema %q already exists", name)
		}
	}

	schema := FormatSchema{Type: schemaType, URI: uri}
	p.Data.ClickHouse.FormatSchemas[name] = schema
	return nil
}

func (p *SubClusterCH) UpdateFormatSchema(name, uri string) error {
	schema, ok := p.Data.ClickHouse.FormatSchemas[name]
	if !ok {
		return semerr.NotFoundf("format schema %q not found", name)
	}

	schema.URI = uri
	p.Data.ClickHouse.FormatSchemas[name] = schema
	return nil
}

func (p *SubClusterCH) DeleteFormatSchema(name string) error {
	_, ok := p.Data.ClickHouse.FormatSchemas[name]
	if !ok {
		return semerr.NotFoundf("format schema %q not found", name)
	}

	delete(p.Data.ClickHouse.FormatSchemas, name)
	return nil
}

type ShardGroup struct {
	Description string   `json:"description"`
	ShardNames  []string `json:"shard_names"`
}

func (p *SubClusterCH) AddShardGroup(group chmodels.ShardGroup) error {
	_, ok := p.Data.ClickHouse.ShardGroups[group.Name]
	if ok {
		return semerr.AlreadyExistsf("shard group %q already exists", group.Name)
	}

	groupPillar := ShardGroup{Description: group.Description, ShardNames: group.ShardNames}
	p.Data.ClickHouse.ShardGroups[group.Name] = groupPillar
	return nil
}

func (p *SubClusterCH) UpdateShardGroup(update chmodels.UpdateShardGroupArgs) error {
	group, ok := p.Data.ClickHouse.ShardGroups[update.Name]
	if !ok {
		return semerr.NotFoundf("shard group %q not found", update.Name)
	}

	isChanged := false

	if update.Description.Valid && group.Description != update.Description.String {
		isChanged = true
		group.Description = update.Description.String
	}

	if update.ShardNames.Valid && !reflect.DeepEqual(group.ShardNames, update.ShardNames.Strings) {
		isChanged = true
		group.ShardNames = update.ShardNames.Strings
	}

	if !isChanged {
		return semerr.FailedPrecondition("nothing to update")
	}

	groupPillar := ShardGroup{Description: group.Description, ShardNames: group.ShardNames}
	p.Data.ClickHouse.ShardGroups[update.Name] = groupPillar
	return nil
}

func (p *SubClusterCH) AddShardInGroup(groupName, shardName string) error {
	group, ok := p.Data.ClickHouse.ShardGroups[groupName]
	if !ok {
		return semerr.NotFoundf("shard group %q not found", groupName)
	}

	for _, shard := range group.ShardNames {
		if shardName == shard {
			return semerr.AlreadyExistsf("shard %q already added in group %q", shardName, groupName)
		}
	}

	group.ShardNames = append(group.ShardNames, shardName)

	p.Data.ClickHouse.ShardGroups[groupName] = group
	return nil
}

func (p *SubClusterCH) DeleteShardGroup(name string) error {
	_, ok := p.Data.ClickHouse.ShardGroups[name]
	if !ok {
		return semerr.NotFoundf("shard group %q not found", name)
	}

	delete(p.Data.ClickHouse.ShardGroups, name)
	return nil
}

type Shard struct {
	Weight int64 `json:"weight"`
}

func (p *SubClusterCH) AddShard(id string, weight int64) error {
	_, ok := p.Data.ClickHouse.Shards[id]
	if ok {
		return semerr.AlreadyExistsf("shard %q already exists", id)
	}

	p.Data.ClickHouse.Shards[id] = Shard{Weight: weight}
	return nil
}

func (p *SubClusterCH) DeleteShard(id string) {
	_, ok := p.Data.ClickHouse.Shards[id]
	if ok {
		delete(p.Data.ClickHouse.Shards, id)
	}
}

func (p *SubClusterCH) DeleteShardFromAllGroups(name string) error {
	for groupName, group := range p.Data.ClickHouse.ShardGroups {
		for i, n := range group.ShardNames {
			if n == name {
				if len(group.ShardNames) == 1 {
					return semerr.FailedPreconditionf("last shard in shard group %q cannot be removed", groupName)
				}

				p.Data.ClickHouse.ShardGroups[groupName] = ShardGroup{
					Description: group.Description,
					ShardNames:  append(group.ShardNames[:i], group.ShardNames[i+1:]...),
				}
			}
		}
	}

	return nil
}

func (p SubClusterCH) SQLUserManagement() bool {
	return p.Data.ClickHouse.SQLUserManagement != nil && *p.Data.ClickHouse.SQLUserManagement
}

func (p *SubClusterCH) SetSQLUserManagement(v optional.Bool) {
	val := v.Valid && v.Bool
	p.Data.ClickHouse.SQLUserManagement = &val
}

func (p SubClusterCH) SQLDatabaseManagement() bool {
	return p.Data.ClickHouse.SQLDatabaseManagement != nil && *p.Data.ClickHouse.SQLDatabaseManagement
}

func (p *SubClusterCH) SetSQLDatabaseManagement(v optional.Bool) {
	val := v.Valid && v.Bool
	p.Data.ClickHouse.SQLDatabaseManagement = &val
}

func (p SubClusterCH) CloudStorageEnabled() bool {
	return p.Data.CloudStorage.Enabled != nil && *p.Data.CloudStorage.Enabled
}

func (p SubClusterCH) CloudStorageBucket() *string {
	if !p.CloudStorageEnabled() || p.Data.CloudStorage.S3 == nil {
		return nil
	}

	return &p.Data.CloudStorage.S3.Bucket
}

func (p SubClusterCH) CloudStorageMoveFactor() *float64 {
	if !p.CloudStorageEnabled() || p.Data.CloudStorage.Settings == nil || p.Data.CloudStorage.Settings.MoveFactor == nil {
		return nil
	}

	return p.Data.CloudStorage.Settings.MoveFactor
}

func (p SubClusterCH) CloudStorageDataCacheEnabled() *bool {
	if !p.CloudStorageEnabled() || p.Data.CloudStorage.Settings == nil || p.Data.CloudStorage.Settings.DataCacheEnabled == nil {
		return nil
	}

	return p.Data.CloudStorage.Settings.DataCacheEnabled
}

func (p SubClusterCH) CloudStorageDataCacheMaxSize() *int64 {
	if !p.CloudStorageEnabled() || p.Data.CloudStorage.Settings == nil || p.Data.CloudStorage.Settings.DataCacheMaxSize == nil {
		return nil
	}

	return p.Data.CloudStorage.Settings.DataCacheMaxSize
}

func (p SubClusterCH) MySQLProtocol() bool {
	return p.Data.ClickHouse.MysqlProtocol != nil && *p.Data.ClickHouse.MysqlProtocol
}

func (p SubClusterCH) PostgreSQLProtocol() bool {
	return p.Data.ClickHouse.PostgresqlProtocol != nil && *p.Data.ClickHouse.PostgresqlProtocol
}

func (p SubClusterCH) EmbeddedKeeper() bool {
	return p.Data.ClickHouse.EmbeddedKeeper != nil && *p.Data.ClickHouse.EmbeddedKeeper
}

func (p *SubClusterCH) SetEmbeddedKeeper(keeper bool) {
	p.Data.ClickHouse.EmbeddedKeeper = &keeper
}

func (p *SubClusterCH) SetMySQLProtocol(enabled optional.Bool) {
	p.Data.ClickHouse.MysqlProtocol = pillars.MapOptionalBoolToPtrBool(enabled)
}

func (p *SubClusterCH) SetPostgreSQLProtocol(enabled optional.Bool) {
	p.Data.ClickHouse.PostgresqlProtocol = pillars.MapOptionalBoolToPtrBool(enabled)
}

func (p *SubClusterCH) SetEmbeddedKeeperHosts(newHosts []hosts.Host) bool {
	newHostFQDNs := hosts.GetFQDNs(newHosts)

	keeperHosts := []string{}
	for keeperHost := range p.Data.ClickHouse.KeeperHosts {
		keeperHosts = append(keeperHosts, keeperHost)
	}

	intersection := slices.IntersectStrings(keeperHosts, newHostFQDNs)
	if len(intersection) == len(newHostFQDNs) {
		return false
	}

	maxServerID := 0
	for _, serverID := range p.Data.ClickHouse.KeeperHosts {
		if serverID > maxServerID {
			maxServerID = serverID
		}
	}
	newServerID := maxServerID + 1

	newKeeperHosts := map[string]int{}
	for _, hostFQDN := range newHostFQDNs {
		serverID, ok := p.Data.ClickHouse.KeeperHosts[hostFQDN]
		if !ok {
			newKeeperHosts[hostFQDN] = newServerID
			newServerID++

			continue
		}

		newKeeperHosts[hostFQDN] = serverID
	}

	p.Data.ClickHouse.KeeperHosts = newKeeperHosts
	return true
}

func (p *SubClusterCH) SetEmbeddedZKHosts(newHosts []hosts.Host) bool {
	newHostFQDNs := hosts.GetFQDNs(newHosts)
	intersection := slices.IntersectStrings(p.Data.ClickHouse.ZKHosts, newHostFQDNs)
	if len(intersection) == len(newHostFQDNs) {
		return false
	}

	sort.Strings(newHostFQDNs)
	p.Data.ClickHouse.ZKHosts = newHostFQDNs
	return true
}

func (p *SubClusterCH) SetServiceAccountID(serviceAccountID string) {
	if serviceAccountID != "" {
		p.Data.ServiceAccountID = &serviceAccountID
	}
}

func (p *SubClusterCH) SetClickHouseClusterName(name string) {
	if name != "" {
		p.Data.ClickHouse.ClusterName = &name
	}
}

func (p *SubClusterCH) EnableCloudStorage(enable bool, cid string, cfg logic.CHConfig) {
	p.Data.CloudStorage.Enabled = &enable

	if !enable {
		return
	}

	p.Data.CloudStorage.S3 = &CloudStorageS3{
		Bucket: cfg.CloudStorageBucketName(cid),
	}
}

func (p SubClusterCH) ToClusterConfig() chmodels.ClusterConfig {
	res := chmodels.ClusterConfig{
		Version:               chmodels.CutVersionToMajor(p.Data.ClickHouse.Version),
		BackupWindowStart:     p.Data.Backup.Start,
		Access:                p.Data.Access.toModel(),
		CloudStorageEnabled:   p.CloudStorageEnabled(),
		SQLDatabaseManagement: p.SQLDatabaseManagement(),
		SQLUserManagement:     p.SQLUserManagement(),
		EmbeddedKeeper:        p.EmbeddedKeeper(),
		MySQLProtocol:         p.MySQLProtocol(),
		PostgreSQLProtocol:    p.PostgreSQLProtocol(),
		CloudStorageConfig: chmodels.CloudStorageConfig{
			DataCacheEnabled: pillars.MapPtrBoolToOptionalBool(p.CloudStorageDataCacheEnabled()),
			DataCacheMaxSize: pillars.MapPtrInt64ToOptionalInt64(p.CloudStorageDataCacheMaxSize()),
			MoveFactor:       pillars.MapPtrFloat64ToOptionalFloat64(p.CloudStorageMoveFactor()),
		},
	}

	return res
}

func (p SubClusterCH) ServiceAccountID() optional.String {
	return pillars.MapPtrStringToOptionalString(p.Data.ServiceAccountID)
}
