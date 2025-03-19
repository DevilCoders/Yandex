package logic

import (
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/monitoring"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
	"a.yandex-team.ru/library/go/valid"
)

type DefaultResourcesRecord struct {
	ResourcePresetExtID string `json:"resource_preset_id" yaml:"resource_preset_id"`
	DiskTypeExtID       string `json:"disk_type_id" yaml:"disk_type_id"`
	DiskSize            int64  `json:"disk_size" yaml:"disk_size"`
	Generation          int64  `json:"generation" yaml:"generation"`
}

type DefaultResourcesConfig struct {
	ByClusterType map[string]map[string]DefaultResourcesRecord `json:"by_cluster_type" yaml:"by_cluster_type"`
}

type Config struct {
	Logs                         LogsConfig                   `json:"logs" yaml:"logs"`
	S3BucketPrefix               string                       `json:"s3_bucket_prefix" yaml:"s3_bucket_prefix"`
	PlatformsInfo                []consolemodels.Platform     `json:"platforms_info" yaml:"platforms_info"`
	VTypes                       map[environment.VType]string `json:"vtypes" yaml:"vtypes"`
	SaltEnvs                     SaltEnvsConfig               `json:"saltenvs" yaml:"saltenvs"`
	ZoneRenameMap                map[string]string            `json:"zone_rename_map" yaml:"zone_rename_map"`
	GenerationNames              map[int64]string             `json:"generation_names" yaml:"generation_names"`
	BackupPurgeDelay             time.Duration                `json:"backup_purge_delay" yaml:"backup_purge_delay"`
	DefaultResources             DefaultResourcesConfig       `json:"console_default_resources" yaml:"console_default_resources"`
	DefaultResourcesDefaultValue *DefaultResourcesRecord      `json:"console_default_resources_default_value" yaml:"console_default_resources_default_value"`
	DefaultCloudQuota            quota.Resources              `json:"cloud_default_quota" yaml:"cloud_default_quota"`
	ResourceValidation           ResourceValidationConfig     `json:"resource_validation" yaml:"resource_validation"`
	Console                      ConsoleConfig                `json:"console" yaml:"console"`
	Monitoring                   MonitoringConfig             `json:"monitoring" yaml:"monitoring"`
	ClickHouse                   CHConfig                     `json:"clickhouse" yaml:"clickhouse"`
	Kafka                        KafkaConfig                  `json:"kafka" yaml:"kafka"`
	Metastore                    MetastoreConfig              `json:"metastore" yaml:"metastore"`
	Elasticsearch                ElasticsearchConfig          `json:"elasticsearch" yaml:"elasticsearch"`
	SQLServer                    SQLServerConfig              `json:"sqlserver" yaml:"sqlserver"`
	E2E                          E2EConfig                    `json:"e2e" yaml:"e2e"`
	EnvironmentVType             environment.VType            `json:"environment_vtype" yaml:"environment_vtype"`
	Greenplum                    GreenplumConfig              `json:"greenplum" yaml:"greenplum"`
	ResourceModel                environment.ResourceModel    `json:"resource_model" yaml:"resource_model"`
	ServiceZK                    ServiceZKConfig              `json:"service_zk" yaml:"service_zk"`
	Flags                        FlagsConfig                  `json:"flags" yaml:"flags"`
	Airflow                      AirflowConfig                `json:"airlfow" yaml:"airflow"`
	ClusterStopSupported         bool                         `json:"cluster_stop_supported" yaml:"cluster_stop_supported"`
}

func DefaultConfig() Config {
	return Config{
		Logs:           DefaultLogsConfig(),
		S3BucketPrefix: "yandexcloud-dbaas-",
		PlatformsInfo: []consolemodels.Platform{
			{
				ID:          "mdb-v1",
				Description: "Intel Broadwell",
				Generation:  1,
			},
			{
				ID:          "mdb-v2",
				Description: "Intel Cascade Lake",
				Generation:  2,
			},
			{
				ID:          "mdb-v3",
				Description: "Intel Ice Lake",
				Generation:  3,
			},
		},
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "df.cloud.yandex.net",
			environment.VTypePorto:   "db.yandex.net",
			environment.VTypeAWS:     "yadc.io",
		},
		ZoneRenameMap: map[string]string{
			"ru-central1-a": "rc1a",
			"ru-central1-b": "rc1b",
			"ru-central1-c": "rc1c",
			"eu-central-1a": "ec1a",
			"eu-central-1b": "ec1b",
			"eu-central-1c": "ec1c",
			"us-east-2a":    "use2a",
			"us-east-2b":    "use2b",
			"us-east-2c":    "use2c",
		},
		GenerationNames: map[int64]string{
			1: "Haswell",
			2: "Broadwell",
			3: "Cascade Lake",
		},
		DefaultResources: DefaultResourcesConfig{
			ByClusterType: make(map[string]map[string]DefaultResourcesRecord),
		},
		BackupPurgeDelay:   time.Hour * 24 * 7,
		DefaultCloudQuota:  DefaultCloudQuota(),
		ResourceValidation: DefaultResourceValidationConfig(),
		ClickHouse:         DefaultCHConfig(),
		SQLServer:          DefaultSQLServerConfig(),
		Greenplum:          DefaultGreenplumConfig(),
		Elasticsearch:      DefaultElasticSearchConfig(),
		ResourceModel:      environment.ResourceModelYandex,
		Flags:              DefaultFlagsConfig(),
	}
}

func (cfg *Config) S3BucketName(cid string) string {
	return cfg.S3BucketPrefix + cid
}

func (cfg *Config) GetDomain(vtype environment.VType) (string, error) {
	res, ok := cfg.VTypes[vtype]
	if !ok {
		return "", xerrors.Errorf("unknown vtype: %q", vtype)
	}
	return res, nil
}

func (cfg E2EConfig) IsClusterE2E(clusterName, folderExtID string) bool {
	return cfg.ClusterName == clusterName && cfg.FolderID == folderExtID
}

type SaltEnvsConfig struct {
	Production environment.SaltEnv `json:"production" yaml:"production"`
	Prestable  environment.SaltEnv `json:"prestable" yaml:"prestable"`
}

var _ valid.Validator = &SaltEnvsConfig{}

func (cfg SaltEnvsConfig) Validate(_ *valid.ValidationCtx) (bool, error) {
	if _, err := environment.ParseSaltEnv(string(cfg.Production)); err != nil {
		return true, xerrors.Errorf("saltenvs config: production: %w", err)
	}

	if _, err := environment.ParseSaltEnv(string(cfg.Prestable)); err != nil {
		return true, xerrors.Errorf("saltenvs config: prestable: %w", err)
	}

	return true, nil
}

func (cfg *Config) GetGenerationName(generation int64) (string, error) {
	res, ok := cfg.GenerationNames[generation]
	if !ok {
		return "", xerrors.Errorf("unknown generation: %d", generation)
	}
	return res, nil
}

func (cfg *Config) GetDefaultResources(clusterType clusters.Type, role hosts.Role) (DefaultResourcesRecord, error) {
	roles, ok := cfg.DefaultResources.ByClusterType[clusterType.Stringified()]
	if !ok {
		if cfg.DefaultResourcesDefaultValue != nil {
			return *cfg.DefaultResourcesDefaultValue, nil
		}
		return DefaultResourcesRecord{}, xerrors.Errorf("unknown default resources for cluster type : %s", clusterType)
	}
	res, ok := roles[role.Stringified()]
	if !ok {
		if cfg.DefaultResourcesDefaultValue != nil {
			return *cfg.DefaultResourcesDefaultValue, nil
		}
		return DefaultResourcesRecord{}, xerrors.Errorf("unknown default resources for cluster type and role : %s %s", clusterType, role)
	}
	return res, nil
}

func (cfg *Config) MapZoneName(zone string) string {
	res, ok := cfg.ZoneRenameMap[zone]
	if !ok {
		return zone
	}
	return res
}

type LogsConfig struct {
	Retry          retry.Config  `json:"retry" yaml:"retry"`
	BatchSize      int64         `json:"batch_size" yaml:"batch_size"`
	TailWaitPeriod time.Duration `json:"tail_wait_period" yaml:"tail_wait_period"`
}

func DefaultLogsConfig() LogsConfig {
	return LogsConfig{
		Retry:          retry.DefaultConfig(),
		BatchSize:      250,
		TailWaitPeriod: time.Second * 5,
	}
}

type KafkaConfig struct {
	ClustersPrefix string   `json:"clusters_prefix" yaml:"clusters_prefix"`
	TasksPrefix    string   `json:"tasks_prefix" yaml:"tasks_prefix"`
	ZooKeeperZones []string `json:"zk_zones" yaml:"zk_zones"`
	SyncTopics     bool     `json:"sync_topics" yaml:"sync_topics"`
}

type MetastoreConfig struct {
	ClustersPrefix                    string   `json:"clusters_prefix" yaml:"clusters_prefix"`
	TasksPrefix                       string   `json:"tasks_prefix" yaml:"tasks_prefix"`
	KubernetesClusterID               string   `json:"kubernetes_cluster_id" yaml:"kubernetes_cluster_id"`
	PostgresqlClusterID               string   `json:"postgresql_cluster_id" yaml:"postgresql_cluster_id"`
	KubernetesClusterServiceAccountID string   `json:"kubernetes_cluster_service_account_id" yaml:"kubernetes_cluster_service_account_id"`
	KubernetesNodeServiceAccountID    string   `json:"kubernetes_node_service_account_id" yaml:"kubernetes_node_service_account_id"`
	PostgresqlHostname                string   `json:"postgresql_hostname" yaml:"postgresql_hostname"`
	ServiceSubnetIDs                  []string `json:"service_subnet_ids" yaml:"service_subnet_ids"`
}

type ElasticsearchConfig struct {
	TasksPrefix            string    `json:"tasks_prefix" yaml:"tasks_prefix"`
	Versions               []Version `json:"versions" yaml:"versions"`
	AllowedEditions        []string  `json:"allowed_editions" yaml:"allowed_editions"`
	EnableAutoBackups      bool      `json:"enable_auto_backups" yaml:"enable_auto_backups"`
	ExtensionPrevalidation bool      `json:"extension_prevalidation" yaml:"extension_prevalidation"`
	ExtensionAllowedDomain string    `json:"extension_allowed_domain" yaml:"extension_allowed_domain"`
}

type SQLServerConfig struct {
	TasksPrefix    string                 `json:"tasks_prefix" yaml:"tasks_prefix"`
	BackupSchedule bmodels.BackupSchedule `json:"backup_schedule" yaml:"backup_schedule"`
	ProductIDs     SQLServerProductIDs    `json:"product_ids" yaml:"product_ids"`
}

type SQLServerProductIDs struct {
	Standard   string `json:"standard" yaml:"standard"`
	Enterprise string `json:"enterprise" yaml:"enterprise"`
}

type GreenplumConfig struct {
	TasksPrefix    string                 `json:"tasks_prefix" yaml:"tasks_prefix"`
	BackupSchedule bmodels.BackupSchedule `json:"backup_schedule" yaml:"backup_schedule"`
}

type E2EConfig struct {
	ClusterName string `json:"cluster_name" yaml:"cluster_name"`
	FolderID    string `json:"folder_id" yaml:"folder_id"`
}

type CHBackupConfig struct {
	BackupSchedule              bmodels.BackupSchedule `json:"backup_schedule" yaml:"backup_schedule"`
	MinimalBackupTimeLimitHours int                    `json:"minimal_backup_time_limit_hours" yaml:"minimal_backup_time_limit_hours"`
}

type CHConfig struct {
	Backup                   CHBackupConfig                `json:"backup" yaml:"backup"`
	Versions                 []Version                     `json:"versions" yaml:"versions"`
	MinimalZkResources       []ZkResource                  `json:"minimal_zk_resources" yaml:"minimal_zk_resources"`
	ExternalURIValidation    ExternalURIValidationSettings `json:"external_uri_validation" yaml:"external_uri_validation"`
	ShardCountLimit          int64                         `json:"shard_count_limit" yaml:"shard_count_limit"`
	ZooKeeperHostCount       int                           `json:"zk_host_count" yaml:"zk_host_count"`
	ClustersPrefix           string                        `json:"clusters_prefix" yaml:"clusters_prefix"`
	TasksPrefix              string                        `json:"tasks_prefix" yaml:"tasks_prefix"`
	CloudStorageBucketPrefix string                        `json:"cloud_storage_bucket_prefix" yaml:"cloud_storage_bucket_prefix"`
}

func (cfg *CHConfig) CloudStorageBucketName(cid string) string {
	return cfg.CloudStorageBucketPrefix + cid
}

func DefaultCHConfig() CHConfig {
	return CHConfig{
		Backup: CHBackupConfig{
			BackupSchedule: bmodels.BackupSchedule{
				Sleep: 7200,
				Start: bmodels.BackupWindowStart{
					Hours:   22,
					Minutes: 15,
					Seconds: 30,
					Nanos:   100,
				},
				UseBackupService: false,
			},
			MinimalBackupTimeLimitHours: 5,
		},
		Versions:           DefaultClickhouseVersionsConfig(),
		MinimalZkResources: DefaultClickhouseZkResourcesConfig(),
		ExternalURIValidation: ExternalURIValidationSettings{
			Regexp: "https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+",
		},
		ShardCountLimit:          50,
		ZooKeeperHostCount:       3,
		CloudStorageBucketPrefix: "cloud-storage-",
	}
}

func DefaultElasticSearchConfig() ElasticsearchConfig {
	return ElasticsearchConfig{
		Versions: DefaultElasticSearchVersionsConfig(),
	}
}

func DefaultElasticSearchVersionsConfig() []Version {
	return []Version{
		{
			Name:        "7.17",
			ID:          "7.17.0",
			Default:     true,
			UpdatableTo: []string{},
		},
		{
			Name: "7.16",
			ID:   "7.16.2",
			UpdatableTo: []string{
				"7.17",
			},
		},
		{
			Name: "7.15",
			ID:   "7.15.2",
			UpdatableTo: []string{
				"7.17",
				"7.16",
			},
		},
		{
			Name: "7.14",
			ID:   "7.14.2",
			UpdatableTo: []string{
				"7.17",
				"7.16",
				"7.15",
			},
		},
		{
			Name: "7.13",
			ID:   "7.13.4",
			UpdatableTo: []string{
				"7.17",
				"7.16",
				"7.15",
				"7.14",
			},
		},
		{
			Name: "7.12",
			ID:   "7.12.1",
			UpdatableTo: []string{
				"7.17",
				"7.16",
				"7.15",
				"7.14",
				"7.13",
			},
		},
		{
			Name: "7.11",
			ID:   "7.11.2",
			UpdatableTo: []string{
				"7.17",
				"7.16",
				"7.15",
				"7.14",
				"7.13",
				"7.12",
			},
		},
		{
			Name: "7.10",
			ID:   "7.10.2",
			UpdatableTo: []string{
				"7.17",
				"7.16",
				"7.15",
				"7.14",
				"7.13",
				"7.12",
				"7.11",
			},
		},
	}
}

func DefaultSQLServerConfig() SQLServerConfig {
	return SQLServerConfig{
		BackupSchedule: bmodels.BackupSchedule{
			Sleep: 7200,
			Start: bmodels.BackupWindowStart{
				Hours:   22,
				Minutes: 15,
				Seconds: 30,
				Nanos:   100,
			},
			UseBackupService: false,
		},
		ProductIDs: SQLServerProductIDs{
			Standard:   "dqnversiomdbstdmssql",
			Enterprise: "dqnversiomdbentmssql",
		},
	}
}

func DefaultGreenplumConfig() GreenplumConfig {
	return GreenplumConfig{
		BackupSchedule: bmodels.BackupSchedule{
			Sleep: 7200,
			Start: bmodels.BackupWindowStart{
				Hours:   22,
				Minutes: 15,
				Seconds: 30,
				Nanos:   100,
			},
			UseBackupService: false,
		},
	}
}

func DefaultCloudQuota() quota.Resources {
	clusters := int64(2)
	hosts := int64(3)
	return quota.Resources{
		CPU:      float64(clusters * hosts * 16),
		Memory:   clusters * hosts * 64 * 1024 * 1024 * 1024,
		SSDSpace: clusters * hosts * 800 * 1024 * 1024 * 1024,
		HDDSpace: clusters * hosts * 800 * 1024 * 1024 * 1024,
		Clusters: clusters,
	}
}

type ResourceValidationConfig struct {
	DecommissionedResourcePresets []string `json:"decommissioned_resource_presets" yaml:"decommissioned_resource_presets"`
	DecommissionedZones           []string `json:"decommissioned_zones" yaml:"decommissioned_zones"`
	MinimalDiskUnit               int64    `json:"minimal_disk_unit" yaml:"minimal_disk_unit"`
}

func DefaultResourceValidationConfig() ResourceValidationConfig {
	return ResourceValidationConfig{
		MinimalDiskUnit: 1,
	}
}

func (cfg ResourceValidationConfig) IsDecommissionedResourcePresets(flavor string) bool {
	return slices.ContainsString(cfg.DecommissionedResourcePresets, flavor)
}

func (cfg ResourceValidationConfig) IsDecommissionedZone(zoneID string) bool {
	return slices.ContainsString(cfg.DecommissionedZones, zoneID)
}

type Version struct {
	ID          string   `json:"version" yaml:"version"`
	Name        string   `json:"name" yaml:"name"`
	Deprecated  bool     `json:"deprecated" yaml:"deprecated"`
	Default     bool     `json:"default" yaml:"default"`
	UpdatableTo []string `json:"updatable_to" yaml:"updatable_to"`
}

func DefaultClickhouseVersionsConfig() []Version {
	return []Version{
		{
			ID:         "21.1.9.41",
			Name:       "21.1",
			Deprecated: true,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.2.10.48",
			Name:       "21.2",
			Deprecated: false,
			UpdatableTo: []string{
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.3.8.76",
			Name:       "21.3 LTS",
			Deprecated: false,
			Default:    true,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.4.5.46",
			Name:       "21.4",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.7.2.7",
			Name:       "21.7",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.8.15.7",
			Name:       "21.8 LTS",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.9.5.16",
			Name:       "21.9",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.10.2.15",
			Name:       "21.10",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "21.11.5.33",
			Name:       "21.11",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"22.1.4.30",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "22.1.4.30",
			Name:       "22.1",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.3.2.2",
				"22.5.1.2079",
			},
		},
		{
			ID:         "22.3.2.2",
			Name:       "22.3 LTS",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.5.1.2079",
			},
		},
		{
			ID:         "22.5.1.2079",
			Name:       "22.5",
			Deprecated: false,
			UpdatableTo: []string{
				"21.2.10.48",
				"21.3.8.76",
				"21.4.5.46",
				"21.7.2.7",
				"21.8.15.7",
				"21.9.5.16",
				"21.10.2.15",
				"21.11.5.33",
				"22.1.4.30",
				"22.3.2.2",
			},
		},
	}
}

type ZkResource struct {
	ChSubclusterCPU float64
	ZKHostCPU       float64
}

type ExternalURIValidationSettings struct {
	Regexp        string             `json:"regexp" yaml:"regexp"`
	Message       string             `json:"message" yaml:"message"`
	UseHTTPClient bool               `json:"use_http_client" yaml:"use_http_client"`
	TLS           httputil.TLSConfig `json:"tls" yaml:"tls"`
}

func DefaultClickhouseZkResourcesConfig() []ZkResource {
	return []ZkResource{
		{
			ChSubclusterCPU: 16,
			ZKHostCPU:       2,
		},
		{
			ChSubclusterCPU: 48,
			ZKHostCPU:       4,
		},
	}
}

type MonitoringConfig struct {
	Charts map[clusters.Type][]MonitoringChartConfig `json:"charts" yaml:"charts"`
}

type MonitoringChartConfig struct {
	Name        string `json:"name" yaml:"name"`
	Description string `json:"description" yaml:"description"`
	Link        string `json:"link" yaml:"link"`
}

func (mc MonitoringConfig) New(typ clusters.Type, cid, folderExtID, consoleURI string) (monitoring.Monitoring, error) {
	charts, ok := mc.Charts[typ]
	if !ok {
		return monitoring.Monitoring{}, xerrors.Errorf("no charts for cluster type %q", typ)
	}

	r := strings.NewReplacer("{cid}", cid, "{folderExtID}", folderExtID, "{console}", consoleURI)
	res := make([]monitoring.Chart, 0, len(charts))
	for _, c := range charts {
		chart := monitoring.Chart(c)
		chart.Link = r.Replace(c.Link)
		res = append(res, chart)
	}

	return monitoring.Monitoring{Charts: res}, nil
}

type ConsoleConfig struct {
	URI string `json:"uri" yaml:"uri"`
}

type ServiceZKConfig struct {
	Hosts map[environment.SaltEnv][]string `json:"hosts" yaml:"hosts"`
}

type FlagsConfig struct {
	AllowMoveBetweenClouds bool `json:"allow_move_between_clouds" yaml:"allow_move_between_clouds"`
}

func DefaultFlagsConfig() FlagsConfig {
	return FlagsConfig{
		AllowMoveBetweenClouds: false,
	}
}

type AirflowConfig struct {
	KubernetesClusterID string `json:"kubernetes_cluster_id" yaml:"kubernetes_cluster_id"`
	PostgresqlClusterID string `json:"postgresql_cluster_id" yaml:"postgresql_cluster_id"`
}
