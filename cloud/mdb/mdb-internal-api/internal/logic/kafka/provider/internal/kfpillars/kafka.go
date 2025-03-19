package kfpillars

import (
	"encoding/json"
	"math"
	"reflect"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	DefaultSegmentBytes = int64(1024 * 1024 * 1024)
)

type Cluster struct {
	Data           ClusterData `json:"data"`
	skipValidation bool

	requiredDiskSizeBeforeUpdate int64
}

type ClusterData struct {
	Kafka                     KafkaData             `json:"kafka"`
	ZooKeeper                 ZooKeeperData         `json:"zk"`
	ClusterPrivateKey         pillars.CryptoKey     `json:"cluster_private_key"`
	MDBMetrics                *pillars.MDBMetrics   `json:"mdb_metrics,omitempty"`
	UseYASMAgent              *bool                 `json:"use_yasmagent,omitempty"`
	SuppressExternalYASMAgent bool                  `json:"suppress_external_yasmagent,omitempty"`
	ShipLogs                  *bool                 `json:"ship_logs,omitempty"`
	Billing                   *pillars.Billing      `json:"billing,omitempty"`
	MDBHealth                 *pillars.MDBHealth    `json:"mdb_health,omitempty"`
	CloudType                 environment.CloudType `json:"cloud_type,omitempty"`
	RegionID                  string                `json:"region_id,omitempty"`
	Access                    AccessSettings        `json:"access,omitempty"`
	Encryption                EncryptionSettings    `json:"encryption,omitempty"`
	AWS                       json.RawMessage       `json:"aws,omitempty"`
}

type AccessSettings struct {
	DataTransfer     *bool                `json:"data_transfer,omitempty"`
	Ipv4CidrBlocks   []clusters.CidrBlock `json:"ipv4_cidr_blocks,omitempty"`
	Ipv6CidrBlocks   []clusters.CidrBlock `json:"ipv6_cidr_blocks,omitempty"`
	UserNetIPV4CIDRs []string             `json:"user_net_ipv4_cidrs,omitempty"`
	UserNetIPV6CIDRs []string             `json:"user_net_ipv6_cidrs,omitempty"`
}

func (as *AccessSettings) ToModel() clusters.Access {
	if as == nil {
		return clusters.Access{}
	}

	return clusters.Access{
		DataTransfer:   pillars.MapPtrBoolToOptionalBool(as.DataTransfer),
		Ipv4CidrBlocks: as.Ipv4CidrBlocks,
		Ipv6CidrBlocks: as.Ipv6CidrBlocks,
	}
}

type EncryptionSettings struct {
	Enabled *bool           `json:"enabled,omitempty"`
	Key     json.RawMessage `json:"key,omitempty"`
}

func (es *EncryptionSettings) ToModel() clusters.Encryption {
	if es == nil {
		return clusters.Encryption{}
	}

	return clusters.Encryption{
		Enabled: pillars.MapPtrBoolToOptionalBool(es.Enabled),
		Key:     es.Key,
	}
}

type KafkaConfig struct {
	CompressionType             string   `json:"compression.type,omitempty"`
	LogFlushIntervalMessages    *int64   `json:"log.flush.interval.messages,omitempty"`
	LogFlushIntervalMs          *int64   `json:"log.flush.interval.ms,omitempty"`
	LogFlushSchedulerIntervalMs *int64   `json:"log.flush.scheduler.interval.ms,omitempty"`
	LogRetentionBytes           *int64   `json:"log.retention.bytes,omitempty"`
	LogRetentionHours           *int64   `json:"log.retention.hours,omitempty"`
	LogRetentionMinutes         *int64   `json:"log.retention.minutes,omitempty"`
	LogRetentionMs              *int64   `json:"log.retention.ms,omitempty"`
	LogSegmentBytes             *int64   `json:"log.segment.bytes,omitempty"`
	LogPreallocate              *bool    `json:"log.preallocate,omitempty"`
	SocketSendBufferBytes       *int64   `json:"socket.send.buffer.bytes,omitempty"`
	SocketReceiveBufferBytes    *int64   `json:"socket.receive.buffer.bytes,omitempty"`
	AutoCreateTopicsEnable      *bool    `json:"auto.create.topics.enable,omitempty"`
	NumPartitions               *int64   `json:"num.partitions,omitempty"`
	DefaultReplicationFactor    *int64   `json:"default.replication.factor,omitempty"`
	MessageMaxBytes             *int64   `json:"message.max.bytes,omitempty"`
	ReplicaFetchMaxBytes        *int64   `json:"replica.fetch.max.bytes,omitempty"`
	SslCipherSuites             []string `json:"ssl.cipher.suites,omitempty"`
	OffsetsRetentionMinutes     *int64   `json:"offsets.retention.minutes,omitempty"`
}

type KafkaData struct {
	AdminPassword              pillars.CryptoKey           `json:"admin_password"`
	MonitorPassword            pillars.CryptoKey           `json:"monitor_password,omitempty"`
	Topics                     map[string]TopicData        `json:"topics"`
	DeletedTopics              map[string]DeletedTopic     `json:"deleted_topics"`
	Users                      map[string]UserData         `json:"users"`
	DeletedUsers               map[string]DeletedUser      `json:"deleted_users"`
	Nodes                      map[string]KafkaNodeData    `json:"nodes"`
	Connectors                 map[string]ConnectorData    `json:"connectors"`
	DeletedConnectors          map[string]DeletedConnector `json:"deleted_connectors"`
	MainBrokerID               int                         `json:"main_broker_id"`
	BrokersCount               int64                       `json:"brokers_count"`
	AssignPublicIP             bool                        `json:"assign_public_ip"`
	ZoneID                     []string                    `json:"zone_id"`
	Version                    string                      `json:"version"`
	PackageVersion             string                      `json:"package_version,omitempty"`
	Config                     KafkaConfig                 `json:"config,omitempty"`
	HasZkSubcluster            bool                        `json:"has_zk_subcluster,omitempty"`
	UnmanagedTopics            bool                        `json:"unmanaged_topics,omitempty"`
	SyncTopics                 bool                        `json:"sync_topics,omitempty"`
	ConnectEnabled             bool                        `json:"connect_enabled,omitempty"`
	DiskSizeValidationDisabled bool                        `json:"disk_size_validation_disabled,omitempty"`
	SchemaRegistry             bool                        `json:"schema_registry_enabled,omitempty"`
	AclsViaPy4j                bool                        `json:"acls_via_py4j,omitempty"`
	UsePlainSasl               bool                        `json:"use_plain_sasl,omitempty"`
	// Global emergency flag to disable custom authorizer for all clusters
	UseCustomAuthorizerForCluster bool     `json:"use_custom_authorizer_for_cluster,omitempty"`
	KnownTopicConfigProperties    []string `json:"known_topic_config_properties,omitempty"`
	InterBrokerProtocolVersion    string   `json:"inter_broker_protocol_version,omitempty"`
	// Generated automatically admin user for connection string
	AdminUserName string `json:"admin_username,omitempty"`
	Resources     models.ClusterResources
}

type TopicConfig struct {
	Version            string `json:"-"`
	CleanupPolicy      string `json:"cleanup.policy,omitempty"`
	CompressionType    string `json:"compression.type,omitempty"`
	DeleteRetentionMs  *int64 `json:"delete.retention.ms,omitempty"`
	FileDeleteDelayMs  *int64 `json:"file.delete.delay.ms,omitempty"`
	FlushMessages      *int64 `json:"flush.messages,omitempty"`
	FlushMs            *int64 `json:"flush.ms,omitempty"`
	MinCompactionLagMs *int64 `json:"min.compaction.lag.ms,omitempty"`
	RetentionMs        *int64 `json:"retention.ms,omitempty"`
	RetentionBytes     *int64 `json:"retention.bytes,omitempty"`
	MaxMessageBytes    *int64 `json:"max.message.bytes,omitempty"`
	MinInsyncReplicas  *int64 `json:"min.insync.replicas,omitempty"`
	SegmentBytes       *int64 `json:"segment.bytes,omitempty"`
	Preallocate        *bool  `json:"preallocate,omitempty"`
}

var KnownTopicConfigProperties []string

func init() {
	topicConfigType := reflect.TypeOf(TopicConfig{})
	for i := 0; i < topicConfigType.NumField(); i++ {
		if v, ok := topicConfigType.Field(i).Tag.Lookup("json"); ok {
			if v == "-" {
				continue
			}
			jsonName := strings.Split(v, ",")[0]
			KnownTopicConfigProperties = append(KnownTopicConfigProperties, jsonName)
		}
	}
	sort.Strings(KnownTopicConfigProperties)
}

type TopicData struct {
	Name              string      `json:"name"`
	Partitions        int64       `json:"partitions"`
	ReplicationFactor int64       `json:"replication_factor"`
	Config            TopicConfig `json:"config,omitempty"`
}

type DeletedTopic struct {
	Name      string    `json:"name"`
	DeletedAt time.Time `json:"deleted_at"`
}

type UserData struct {
	Name        string            `json:"name"`
	Password    pillars.CryptoKey `json:"password"`
	Permissions []PermissionsData `json:"permissions"`
}

type DeletedUser struct {
	Name      string    `json:"name"`
	DeletedAt time.Time `json:"deleted_at"`
}

type KafkaNodeData struct {
	ID          int    `json:"id"`
	FQDN        string `json:"fqdn"`
	PrivateFQDN string `json:"private_fqdn"`
	Rack        string `json:"rack"`
}

type PermissionsData struct {
	TopicName string `json:"topic_name"`
	Role      string `json:"role"`
	Group     string `json:"group"`
	Host      string `json:"host"`
}

type ZooKeeperData struct {
	Version   string `json:"version,omitempty"`
	Resources models.ClusterResources
	Nodes     map[string]int  `json:"nodes"`
	Config    ZooKeeperConfig `json:"config"`
	// Name from salt state
	DisableAppArmor bool `json:"apparmor_disabled,omitempty"`
	// Optional SCRAM auth mechaism
	ScramAuthEnabled   bool               `json:"scram_auth_enabled,omitempty"`
	ScramAdminPassword *pillars.CryptoKey `json:"scram_admin_password,omitempty"`
}

type ZooKeeperConfig struct {
	DataDir string `json:"dataDir,omitempty"`
}

type ConnectorData struct {
	Type                  string                `json:"type"`
	Name                  string                `json:"name"`
	TasksMax              int64                 `json:"tasks_max,omitempty"`
	Properties            map[string]string     `json:"properties,omitempty"`
	MirrorMakerConfig     MirrorMakerConfig     `json:"mirrormaker,omitempty"`
	S3SinkConnectorConfig S3SinkConnectorConfig `json:"s3_sink,omitempty"`
}

type DeletedConnector struct {
	Name      string    `json:"name"`
	DeletedAt time.Time `json:"deleted_at"`
}

type MirrorMakerConfig struct {
	SourceCluster     ClusterConnection `json:"source"`
	TargetCluster     ClusterConnection `json:"target"`
	Topics            string            `json:"topics"`
	ReplicationFactor int64             `json:"replication_factor"`
}

type ClusterConnection struct {
	Alias                     string             `json:"alias"`
	Type                      string             `json:"type"`
	BootstrapServers          string             `json:"bootstrap_servers"`
	SaslUsername              string             `json:"sasl_username"`
	SaslPassword              *pillars.CryptoKey `json:"sasl_password,omitempty"`
	SaslMechanism             string             `json:"sasl_mechanism,omitempty"`
	SecurityProtocol          string             `json:"security_protocol,omitempty"`
	SslTruststoreCertificates string             `json:"ssl_truststore_certificates,omitempty"`
}

type S3Connection struct {
	Type            string             `json:"type"`
	AccessKeyID     string             `json:"access_key_id"`
	SecretAccessKey *pillars.CryptoKey `json:"secret_access_key"`
	BucketName      string             `json:"bucket_name"`
	Endpoint        string             `json:"endpoint"`
	Region          string             `json:"region,omitempty"`
}

type S3SinkConnectorConfig struct {
	S3Connection        S3Connection `json:"s3_connection"`
	Topics              string       `json:"topics"`
	FileCompressionType string       `json:"file_compression_type,omitempty"`
	FileMaxRecords      int64        `json:"file_max_records,omitempty"`
}

func NewCluster() *Cluster {
	var pillar Cluster
	pillar.Data.Kafka.Topics = make(map[string]TopicData)
	pillar.Data.Kafka.DeletedTopics = make(map[string]DeletedTopic)
	pillar.Data.Kafka.Users = make(map[string]UserData)
	pillar.Data.Kafka.DeletedUsers = make(map[string]DeletedUser)
	pillar.Data.Kafka.Nodes = make(map[string]KafkaNodeData)
	pillar.Data.Kafka.Connectors = make(map[string]ConnectorData)
	pillar.Data.Kafka.DeletedConnectors = make(map[string]DeletedConnector)
	pillar.Data.ZooKeeper.Nodes = make(map[string]int)
	pillar.Data.Kafka.UseCustomAuthorizerForCluster = true
	pillar.Data.Kafka.KnownTopicConfigProperties = KnownTopicConfigProperties
	pillar.Data.Kafka.MainBrokerID = 1
	pillar.Data.Kafka.AclsViaPy4j = true
	pillar.Data.Kafka.UsePlainSasl = true
	return &pillar
}

func (c *Cluster) SetSkipValidation(skip bool) {
	c.skipValidation = skip
}

func (c *Cluster) MarshalPillar() (json.RawMessage, error) {
	raw, err := json.Marshal(c)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal kafka cluster pillar: %w", err)
	}
	return raw, err
}

func (c *Cluster) UnmarshalPillar(raw json.RawMessage) error {
	if err := json.Unmarshal(raw, c); err != nil {
		return xerrors.Errorf("failed to unmarshal kafka cluster pillar: %w", err)
	}
	if c.Data.ZooKeeper.Config.DataDir == "" {
		c.Data.ZooKeeper.Config.DataDir = "/data/zookeper"
	}
	for _, topic := range c.Data.Kafka.Topics {
		topic.Config.Version = c.Data.Kafka.Version
	}
	if c.Data.Kafka.DeletedTopics == nil {
		c.Data.Kafka.DeletedTopics = make(map[string]DeletedTopic)
	}
	if c.Data.Kafka.DeletedUsers == nil {
		c.Data.Kafka.DeletedUsers = make(map[string]DeletedUser)
	}
	if c.Data.Kafka.DeletedConnectors == nil {
		c.Data.Kafka.DeletedConnectors = make(map[string]DeletedConnector)
	}
	c.Data.Kafka.KnownTopicConfigProperties = KnownTopicConfigProperties
	c.requiredDiskSizeBeforeUpdate = c.requiredDiskSize()
	return nil
}

func (c *Cluster) BrokersQuantity() int64 {
	return c.Data.Kafka.BrokersCount * int64(len(c.Data.Kafka.ZoneID))
}

func (c *Cluster) SetVersion(targetVersion string) error {
	if c.Data.Kafka.Version == "" {
		if targetVersion == "" {
			targetVersion = kfmodels.DefaultVersion
		}

		err := kfmodels.ValidateVersion(targetVersion)
		if err != nil {
			return err
		}
		c.Data.Kafka.InterBrokerProtocolVersion = targetVersion
	} else {
		err := kfmodels.ValidateUpgrade(c.Data.Kafka.Version, targetVersion)
		if err != nil {
			return err
		}

		// Set "inter.broker.protocol.version" if it wasn't set previously.
		// For 2.1 use 2.1-IV2 for compatibility with current config values.
		// For 2.6 and 2.8 just use version itself because current config values are 2.6-IV0 and 2.8-IV0,
		// and I believe they are equivalent.
		if c.Data.Kafka.InterBrokerProtocolVersion == "" {
			c.Data.Kafka.InterBrokerProtocolVersion = c.Data.Kafka.Version
			if c.Data.Kafka.Version == "2.1" {
				c.Data.Kafka.InterBrokerProtocolVersion = "2.1-IV2"
			}
		}
	}

	c.Data.Kafka.Version = targetVersion

	version, err := kfmodels.FindVersion(targetVersion)
	if err != nil {
		// actually this path is impossible because targetVersion is already validated
		return xerrors.Errorf("unknown target version %q: %w", targetVersion, err)
	}
	c.Data.Kafka.PackageVersion = version.PackageVersion

	return nil
}

func (c *Cluster) AddTopic(t TopicData) error {
	_, ok := c.Data.Kafka.Topics[t.Name]
	if ok {
		return semerr.AlreadyExistsf("topic %q already exists", t.Name)
	}
	nameNormalized := strings.ReplaceAll(t.Name, ".", "_")
	for _, topic := range c.Data.Kafka.Topics {
		if nameNormalized == strings.ReplaceAll(topic.Name, ".", "_") {
			return semerr.InvalidInputf("name %q colliding with %q", t.Name, topic.Name)
		}
	}

	c.SetTopic(t)
	return nil
}

func (c *Cluster) GetTopic(name string) (TopicData, bool) {
	u, ok := c.Data.Kafka.Topics[name]
	return u, ok
}

func (c *Cluster) SetTopic(t TopicData) {
	c.Data.Kafka.Topics[t.Name] = t
	delete(c.Data.Kafka.DeletedTopics, t.Name)
}

func (c *Cluster) DeleteTopic(name string) error {
	_, ok := c.Data.Kafka.Topics[name]
	if !ok {
		return semerr.NotFoundf("topic %q not found", name)
	}
	delete(c.Data.Kafka.Topics, name)
	c.Data.Kafka.DeletedTopics[name] = DeletedTopic{Name: name, DeletedAt: time.Now()}
	return nil
}

func (c *Cluster) AddUser(u UserData) error {
	_, ok := c.Data.Kafka.Users[u.Name]
	if ok {
		return semerr.AlreadyExistsf("user %q already exists", u.Name)
	}
	c.Data.Kafka.Users[u.Name] = u
	delete(c.Data.Kafka.DeletedUsers, u.Name)
	return nil
}

func (c *Cluster) GetUser(name string) (UserData, bool) {
	u, ok := c.Data.Kafka.Users[name]
	return u, ok
}

func (c *Cluster) SetUser(u UserData) {
	c.Data.Kafka.Users[u.Name] = u
}

func (c *Cluster) DeleteUser(name string) error {
	_, ok := c.Data.Kafka.Users[name]
	if !ok {
		return semerr.NotFoundf("user %q not found", name)
	}
	delete(c.Data.Kafka.Users, name)
	c.Data.Kafka.DeletedUsers[name] = DeletedUser{Name: name, DeletedAt: time.Now()}
	return nil
}

func (c *Cluster) AddKafkaNode(n KafkaNodeData) error {
	_, ok := c.Data.Kafka.Nodes[n.FQDN]
	if ok {
		return semerr.AlreadyExistsf("node %q already exists", n.FQDN)
	}
	c.Data.Kafka.Nodes[n.FQDN] = n
	return nil
}

func (c *Cluster) AddZooKeeperNode(fqdn string, idx int) error {
	_, ok := c.Data.ZooKeeper.Nodes[fqdn]
	if ok {
		return semerr.AlreadyExistsf("node %q already exists", fqdn)
	}
	c.Data.ZooKeeper.Nodes[fqdn] = idx
	return nil
}

func (c *Cluster) AddConnector(conn ConnectorData) error {
	if err := c.HasConnector(conn.Name); err == nil {
		return semerr.AlreadyExistsf("connector %q already exists", conn.Name)
	}
	if err := c.SetConnector(conn); err != nil {
		return err
	}
	return nil
}

func (c *Cluster) DeleteConnector(name string) error {
	if err := c.HasConnector(name); err != nil {
		return err
	}
	delete(c.Data.Kafka.Connectors, name)
	c.Data.Kafka.DeletedConnectors[name] = DeletedConnector{Name: name, DeletedAt: time.Now()}
	return nil
}

func (c *Cluster) HasConnector(name string) error {
	_, ok := c.Data.Kafka.Connectors[name]
	if !ok {
		return semerr.NotFoundf("connector %q not found", name)
	}
	return nil
}

func (c *Cluster) GetConnector(name string) (ConnectorData, bool) {
	u, ok := c.Data.Kafka.Connectors[name]
	return u, ok
}

func (c *Cluster) SetConnector(connector ConnectorData) error {
	if err := connector.Validate(); err != nil {
		return err
	}
	c.Data.Kafka.Connectors[connector.Name] = connector
	delete(c.Data.Kafka.DeletedConnectors, connector.Name)
	return nil
}

func (c *Cluster) ReplicasQuantity() int64 {
	var quantity int64
	for _, topic := range c.Data.Kafka.Topics {
		quantity = quantity + topic.Partitions*topic.ReplicationFactor
	}
	return quantity
}

func (c *Cluster) totalSizeOfActiveSegments() int64 {
	var totalSizeOfSegments int64
	for _, topic := range c.Data.Kafka.Topics {
		var topicSegmentSize int64
		if topic.Config.SegmentBytes != nil {
			topicSegmentSize = *topic.Config.SegmentBytes
		}
		if topicSegmentSize == 0 && c.Data.Kafka.Config.LogSegmentBytes != nil {
			topicSegmentSize = *c.Data.Kafka.Config.LogSegmentBytes
		}
		if topicSegmentSize == 0 {
			topicSegmentSize = DefaultSegmentBytes
		}
		totalSizeOfSegments += topicSegmentSize * topic.Partitions * topic.ReplicationFactor
	}
	return totalSizeOfSegments
}

func (c *Cluster) requiredDiskSize() int64 {
	// Minimal disk size is to hold two log segments per partition assuming uniform partition distribution
	brokersQuantity := c.BrokersQuantity()
	if brokersQuantity == 0 {
		// overcome "integer divide by zero" in tests
		brokersQuantity = 1
	}
	return 2 * c.totalSizeOfActiveSegments() / brokersQuantity
}

func (kd *KafkaData) TopicsAreManagedViaAdminAPIOnly() bool {
	return kd.UnmanagedTopics && !kd.SyncTopics
}

func (kd *KafkaData) TopicManagementViaAdminAPIAllowed() bool {
	return kd.UnmanagedTopics || kd.SyncTopics
}

func (c *KafkaConfig) Validate() error {
	if c.LogFlushIntervalMessages != nil && *c.LogFlushIntervalMessages < 0 {
		return semerr.InvalidInputf("wrong value for log.flush.interval.messages: %d", *c.LogFlushIntervalMessages)
	}
	if c.LogFlushIntervalMs != nil && *c.LogFlushIntervalMs < 0 {
		return semerr.InvalidInputf("wrong value for log.flush.interval.ms: %d", *c.LogFlushIntervalMs)
	}
	if c.LogFlushSchedulerIntervalMs != nil && *c.LogFlushSchedulerIntervalMs < 0 {
		return semerr.InvalidInputf("wrong value for log.flush.scheduler.interval.ms: %d", *c.LogFlushSchedulerIntervalMs)
	}
	if c.LogRetentionBytes != nil && *c.LogRetentionBytes < -1 {
		return semerr.InvalidInputf("wrong value for log.retention.bytes: %d", *c.LogRetentionBytes)
	}
	if c.LogRetentionHours != nil {
		if *c.LogRetentionHours < -1 || *c.LogRetentionHours > math.MaxInt32 {
			return semerr.InvalidInputf("wrong value for log.retention.hours: %d", *c.LogRetentionHours)
		}
	}
	if c.LogRetentionMinutes != nil {
		if *c.LogRetentionMinutes < -1 || *c.LogRetentionMinutes > math.MaxInt32 {
			return semerr.InvalidInputf("wrong value for log.retention.minutes: %d", *c.LogRetentionMinutes)
		}
	}
	if c.LogRetentionMs != nil && *c.LogRetentionMs < -1 {
		return semerr.InvalidInputf("wrong value for log.retention.ms: %d", *c.LogRetentionMs)
	}
	if c.LogSegmentBytes != nil {
		if *c.LogSegmentBytes < 14 || *c.LogSegmentBytes > math.MaxInt32 {
			return semerr.InvalidInputf("wrong value for log.segment.bytes: %d", *c.LogSegmentBytes)
		}
	}
	if c.SocketSendBufferBytes != nil {
		// 0 or less values are not accepted, except for -1 special value with default semantics
		if (*c.SocketSendBufferBytes < 1 && *c.SocketSendBufferBytes != -1) || *c.SocketSendBufferBytes > math.MaxInt32 {
			return semerr.InvalidInputf("wrong value for socket.send.buffer.bytes: %d", *c.SocketSendBufferBytes)
		}
	}
	if c.SocketReceiveBufferBytes != nil {
		// 0 or less values are not accepted, except for -1 special value with default semantics
		if (*c.SocketReceiveBufferBytes < 1 && *c.SocketReceiveBufferBytes != -1) || *c.SocketReceiveBufferBytes > math.MaxInt32 {
			return semerr.InvalidInputf("wrong value for socket.receive.buffer.bytes: %d", *c.SocketReceiveBufferBytes)
		}
	}
	if c.NumPartitions != nil && *c.NumPartitions < 1 {
		return semerr.InvalidInputf("wrong value for num.partitions: %d", *c.NumPartitions)
	}
	if c.DefaultReplicationFactor != nil && *c.DefaultReplicationFactor < 1 {
		return semerr.InvalidInputf("wrong value for default.replication.factor: %d", *c.DefaultReplicationFactor)
	}
	if c.OffsetsRetentionMinutes != nil && *c.OffsetsRetentionMinutes < 1 {
		return semerr.InvalidInputf("wrong value for offsets.retention.minutes: %d", *c.OffsetsRetentionMinutes)
	}
	if len(c.SslCipherSuites) > 0 {
		if err := validation.IsValidSslCipherSuitesSlice(c.SslCipherSuites); err != nil {
			return err
		}
	}
	return nil
}

func (topic *TopicData) Validate() error {
	if topic.Name == "" {
		return semerr.InvalidInput("topic name must be specified")
	}
	if topic.Partitions < 1 {
		return semerr.InvalidInputf("topic %q has too small partitions number: %d. minimum is 1", topic.Name, topic.Partitions)
	}
	if topic.ReplicationFactor < 1 {
		return semerr.InvalidInputf("topic %q has too small replication factor: %d. minimum is 1", topic.Name, topic.ReplicationFactor)
	}
	if topic.Config.DeleteRetentionMs != nil && *topic.Config.DeleteRetentionMs < 0 {
		return semerr.InvalidInputf("topic %q has wrong value for delete.retention.ms: %d", topic.Name, topic.Config.DeleteRetentionMs)
	}
	if topic.Config.FileDeleteDelayMs != nil && *topic.Config.FileDeleteDelayMs < 0 {
		return semerr.InvalidInputf("topic %q has wrong value for file.delete.delay.ms: %d", topic.Name, *topic.Config.FileDeleteDelayMs)
	}
	if topic.Config.FlushMessages != nil && *topic.Config.FlushMessages < 0 {
		return semerr.InvalidInputf("topic %q has wrong value for flush.messages: %d", topic.Name, *topic.Config.FlushMessages)
	}
	if topic.Config.FlushMs != nil && *topic.Config.FlushMs < 0 {
		return semerr.InvalidInputf("topic %q has wrong value for flush.ms: %d", topic.Name, *topic.Config.FlushMs)
	}
	if topic.Config.MinCompactionLagMs != nil && *topic.Config.MinCompactionLagMs < 0 {
		return semerr.InvalidInputf("topic %q has wrong value for min.compaction.lag.ms: %d", topic.Name, *topic.Config.MinCompactionLagMs)
	}
	if topic.Config.RetentionMs != nil && *topic.Config.RetentionMs < -1 {
		return semerr.InvalidInputf("topic %q has wrong value for retention.ms: %d", topic.Name, *topic.Config.RetentionMs)
	}
	if topic.Config.RetentionBytes != nil && *topic.Config.RetentionBytes < -1 {
		return semerr.InvalidInputf("topic %q has wrong value for retention.bytes: %d", topic.Name, *topic.Config.RetentionBytes)
	}
	if topic.Config.MaxMessageBytes != nil && (*topic.Config.MaxMessageBytes < 0 || *topic.Config.MaxMessageBytes > math.MaxInt32) {
		return semerr.InvalidInputf("topic %q has wrong value for max.message.bytes: %d", topic.Name, *topic.Config.MaxMessageBytes)
	}
	if topic.Config.MinInsyncReplicas != nil && *topic.Config.MinInsyncReplicas < 1 {
		return semerr.InvalidInputf("topic %q has wrong value for min.insync.replicas: %d. minimum is 1", topic.Name, *topic.Config.MinInsyncReplicas)
	}
	if topic.Config.SegmentBytes != nil {
		if *topic.Config.SegmentBytes < 14 || *topic.Config.SegmentBytes > math.MaxInt32 {
			return semerr.InvalidInputf("topic %q has wrong value for segment.bytes: %d. minimum is 14", topic.Name, *topic.Config.SegmentBytes)
		}
	}
	if topic.Config.MinInsyncReplicas != nil && *topic.Config.MinInsyncReplicas > topic.ReplicationFactor {
		return semerr.InvalidInputf("topic %q has too big min.insync.replicas parameter: %d. maximum is topic replication factor: %d", topic.Name, *topic.Config.MinInsyncReplicas, topic.ReplicationFactor)
	}
	return nil
}

func (user *UserData) Validate() error {
	if user.Name == "" {
		return semerr.InvalidInput("user name must be specified")
	}
	for _, perm := range user.Permissions {
		if perm.Role == "admin" {
			if perm.TopicName != "*" {
				return semerr.InvalidInputf("admin role can be set only on all topics. user with admin role %q", user.Name)
			}
		}
	}
	return nil
}

func (cc *ClusterConnection) Validate() error {
	if cc.Type == kfmodels.ClusterConnectionTypeUnspecified {
		return semerr.InvalidInput("cannot create connector with empty cluster connection spec")
	}
	if cc.Type == kfmodels.ClusterConnectionTypeThisCluster {
		return nil
	}
	return kfmodels.ValidateBootstrapServers(cc.BootstrapServers)
}

func (mmConfig *MirrorMakerConfig) Validate(connectorName string) error {
	if mmConfig.Topics == "*" {
		return semerr.InvalidInputf("* is not valid topic name pattern for connector %q. For all topics use .* instead", connectorName)
	}
	if mmConfig.SourceCluster.Type == kfmodels.ClusterConnectionTypeThisCluster && mmConfig.TargetCluster.Type == kfmodels.ClusterConnectionTypeThisCluster {
		return semerr.InvalidInput("cannot create connector with source and target type - this cluster")
	}
	if mmConfig.SourceCluster.Type != kfmodels.ClusterConnectionTypeThisCluster && mmConfig.TargetCluster.Type != kfmodels.ClusterConnectionTypeThisCluster {
		return semerr.InvalidInput("cannot create connector without source or target type - this cluster")
	}
	if mmConfig.ReplicationFactor < 1 {
		return semerr.InvalidInputf("connector %q has too small replication factor: %d. minimum is 1", connectorName, mmConfig.ReplicationFactor)
	}
	if err := mmConfig.SourceCluster.Validate(); err != nil {
		return err
	}
	return mmConfig.TargetCluster.Validate()
}

func (s3Connection *S3Connection) Validate() error {
	if err := kfmodels.ValidateS3BucketName(s3Connection.BucketName); err != nil {
		return err
	}
	if s3Connection.Type == kfmodels.S3ConnectionTypeExternal {
		return kfmodels.ValidateHost(s3Connection.Endpoint)
	} else {
		return semerr.InvalidInputf(
			"Unknown s3-connection type: %q",
			s3Connection.Type,
		)
	}
}

func (s3SinkConnector *S3SinkConnectorConfig) Validate(connectorName string) error {
	if err := s3SinkConnector.S3Connection.Validate(); err != nil {
		return err
	}
	if err := kfmodels.ValidateFileCompressionType(s3SinkConnector.FileCompressionType); err != nil {
		return err
	}
	if s3SinkConnector.FileMaxRecords < 0 {
		return semerr.InvalidInputf(
			"file.max.records value %d is not valid. must be >= 0 where 0 is unlimited.",
			s3SinkConnector.FileMaxRecords,
		)
	}
	return nil
}

func topicConfigVersionMatches(topicConfigVersion string, kafkaVersion string) bool {
	if topicConfigVersion == "" {
		return true
	} else if topicConfigVersion == kfmodels.СonfigVersion3 {
		return kfmodels.IsSupportedVersion3x(kafkaVersion)
	}
	return topicConfigVersion == kafkaVersion
}

func (connector *ConnectorData) Validate() error {
	if err := kfmodels.ValidateConnectorName(connector.Name); err != nil {
		return err
	}
	if connector.TasksMax < 1 {
		return semerr.InvalidInputf("connector %q has too small tasks.max setting's value: %d. minimum is 1", connector.Name, connector.TasksMax)
	}
	if connector.Type == kfmodels.ConnectorTypeMirrormaker {
		return connector.MirrorMakerConfig.Validate(connector.Name)
	} else if connector.Type == kfmodels.ConnectorTypeS3Sink {
		return connector.S3SinkConnectorConfig.Validate(connector.Name)
	} else if connector.Type == kfmodels.ConnectorTypeUnspecified {
		return semerr.InvalidInput("connector type unspecified")
	} else {
		return semerr.InvalidInputf("unknown connector type - %q", connector.Type)
	}
}

func (c *Cluster) Validate() error {
	if c.skipValidation {
		return nil
	}
	if c.Data.Kafka.BrokersCount < 1 {
		return semerr.InvalidInput("brokers count must be at least 1")
	}
	if err := c.Data.Kafka.Config.Validate(); err != nil {
		return err
	}

	if c.Data.Kafka.Config.AutoCreateTopicsEnable != nil && *c.Data.Kafka.Config.AutoCreateTopicsEnable && !c.Data.Kafka.TopicManagementViaAdminAPIAllowed() {
		return semerr.InvalidInput("auto_create_topics_enabled can be set only with unmanaged_topics")
	}
	var brokersQuantity = c.BrokersQuantity()
	if brokersQuantity != int64(len(c.Data.Kafka.Nodes)) {
		return xerrors.Errorf("malformed pillar. expected %d nodes but there is %d", brokersQuantity, len(c.Data.Kafka.Nodes))
	}
	if c.Data.Kafka.Config.DefaultReplicationFactor != nil && brokersQuantity < *c.Data.Kafka.Config.DefaultReplicationFactor {
		return semerr.InvalidInputf("default_replication_factor(%d) must be less then quantity of brokers(%d)",
			*c.Data.Kafka.Config.DefaultReplicationFactor,
			brokersQuantity,
		)
	}
	if c.requiredDiskSize() > c.Data.Kafka.Resources.DiskSize && c.requiredDiskSize() > c.requiredDiskSizeBeforeUpdate && !c.Data.Kafka.DiskSizeValidationDisabled {
		return semerr.InvalidInputf("disk size must be at least %d according to topics partitions number and replication factor but size is %d", c.requiredDiskSize(), c.Data.Kafka.Resources.DiskSize)
	}
	if brokersQuantity > 1 {
		if messageMaxBytesGreater, messageMaxBytes, replicaFetchMaxBytes := validation.MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			c.Data.Kafka.Config.MessageMaxBytes,
			c.Data.Kafka.Config.ReplicaFetchMaxBytes,
		); messageMaxBytesGreater {
			return semerr.InvalidInputf(
				"For multi-node kafka cluster, broker setting \"replica.fetch.max.bytes\" value(%d) must be equal or greater then broker setting \"message.max.bytes\" value(%d) - record log overhead size(%d).",
				replicaFetchMaxBytes,
				messageMaxBytes,
				defaults.RecordLogOverheadBytes,
			)
		}
	}
	for _, topic := range c.Data.Kafka.Topics {
		if err := topic.Validate(); err != nil {
			return err
		}
		if !topicConfigVersionMatches(topic.Config.Version, c.Data.Kafka.Version) {
			return semerr.InvalidInputf("wrong topic config version %s for topic %s, must be: %s", topic.Config.Version, topic.Name, c.Data.Kafka.Version)
		}
		if topic.ReplicationFactor > brokersQuantity {
			return semerr.InvalidInputf("topic %q has too big replication factor: %d. maximum is brokers quantity: %d", topic.Name, topic.ReplicationFactor, brokersQuantity)
		}
		if brokersQuantity > 1 {
			if maxMessageBytesGreater, messageMaxBytes, replicaFetchMaxBytes := validation.MessageMaxBytesMoreThenReplicaFetchMaxBytes(
				topic.Config.MaxMessageBytes,
				c.Data.Kafka.Config.ReplicaFetchMaxBytes,
			); maxMessageBytesGreater {
				return semerr.InvalidInputf(
					"For multi-node kafka cluster, broker setting \"replica.fetch.max.bytes\" value(%d) must be equal or greater then topic (\"%s\") setting \"max.message.bytes\" value(%d) - record log overhead size(%d).",
					replicaFetchMaxBytes,
					topic.Name,
					messageMaxBytes,
					defaults.RecordLogOverheadBytes,
				)
			}
		}
	}
	for _, user := range c.Data.Kafka.Users {
		if err := user.Validate(); err != nil {
			return err
		}
		if !c.Data.Kafka.TopicManagementViaAdminAPIAllowed() {
			for _, perm := range user.Permissions {
				if perm.Role == "admin" {
					return semerr.InvalidInputf("admin role can be set only on clusters with unmanaged topics. user with admin role %q", user.Name)
				}
			}
		}
	}
	for _, connector := range c.Data.Kafka.Connectors {
		if err := connector.Validate(); err != nil {
			return err
		}
		if connector.Type == "mirrormaker" {
			var mmReplicationFactor = connector.MirrorMakerConfig.ReplicationFactor
			if mmReplicationFactor > brokersQuantity {
				return semerr.InvalidInputf("mirrormaker connector %q has too big replication factor: %d. maximum is brokers quantity: %d", connector.Name, mmReplicationFactor, brokersQuantity)
			}
			if mmReplicationFactor < 1 {
				var defaultMirrormakerReplicationFactor = int64(1)
				if brokersQuantity > 1 {
					defaultMirrormakerReplicationFactor = 2
				}
				connector.MirrorMakerConfig.ReplicationFactor = defaultMirrormakerReplicationFactor
				c.Data.Kafka.Connectors[connector.Name] = connector
			}
		}
	}
	return nil
}

func (c *Cluster) SetOneHostMode() {
	c.Data.Kafka.HasZkSubcluster = false
	c.Data.ZooKeeper.DisableAppArmor = true
}

func (c *Cluster) SetHasZkSubcluster() {
	c.Data.Kafka.HasZkSubcluster = true
	c.Data.ZooKeeper.DisableAppArmor = false
}

func (c *Cluster) HasZkSubcluster() bool {
	return c.Data.Kafka.HasZkSubcluster
}

func (c *Cluster) HasZone(zone string) bool {
	return slices.ContainsString(c.Data.Kafka.ZoneID, zone)
}

func (c *Cluster) SetAccess(access clusters.Access) {
	if access.DataTransfer.Valid || access.Ipv4CidrBlocks != nil || access.Ipv6CidrBlocks != nil {
		c.Data.Access.DataTransfer = pillars.MapOptionalBoolToPtrBool(access.DataTransfer)
		c.Data.Access.Ipv4CidrBlocks = access.Ipv4CidrBlocks
		c.Data.Access.Ipv6CidrBlocks = access.Ipv6CidrBlocks
	}
}

func (c *Cluster) SetEncryption(es clusters.Encryption) {
	c.Data.Encryption = EncryptionSettings{
		Enabled: pillars.MapOptionalBoolToPtrBool(es.Enabled),
		Key:     es.Key,
	}
}

func (c *Cluster) GetAdminUserName() string {
	if c.Data.Kafka.AdminUserName != "" {
		return c.Data.Kafka.AdminUserName
	}
	// Previous name for admin user. Changed in ORION-705
	return "owner"
}

func (c *Cluster) SetAdminUserName(userName string) {
	c.Data.Kafka.AdminUserName = userName
}
