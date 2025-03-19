package kfmodels

import (
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

const (
	ConnectorTypeUnspecified string = ""
	ConnectorTypeMirrormaker string = "mirrormaker"
	ConnectorTypeS3Sink      string = "s3_sink"
)

type ConnectorStatus = string

const (
	ConnectorStatusUnspecified string = ""
	ConnectorStatusRunning     string = "running"
	ConnectorStatusPaused      string = "paused"
	ConnectorStatusError       string = "error"
)

type ConnectorHealth int

const (
	ConnectorHealthUnknown ConnectorHealth = iota
	ConnectorHealthAlive
	ConnectorHealthDead
)

const (
	ClusterConnectionTypeUnspecified string = ""
	ClusterConnectionTypeThisCluster string = "this_cluster"
	ClusterConnectionTypeExternal    string = "external"
	S3ConnectionTypeUnspecified      string = ""
	S3ConnectionTypeExternal         string = "s3_external"
	SecurityProtocolPlaintext        string = "PLAINTEXT"
	SecurityProtocolSaslPlaintext    string = "SASL_PLAINTEXT"
)

type ConnectorSpec struct {
	Name                  string
	TasksMax              int64
	Properties            map[string]string
	Type                  string
	MirrormakerConfig     MirrormakerConfigSpec
	S3SinkConnectorConfig S3SinkConnectorConfigSpec
}

type UpdateConnectorSpec struct {
	TasksMax              optional.Int64
	Properties            map[string]string
	PropertiesContains    map[string]bool
	PropertiesOverwrite   bool
	Type                  optional.String
	MirrormakerConfig     UpdateMirrormakerConfigSpec
	S3SinkConnectorConfig UpdateS3SinkConnectorConfigSpec
}

type MirrormakerConfigSpec struct {
	SourceCluster     ClusterConnectionSpec
	TargetCluster     ClusterConnectionSpec
	Topics            string
	ReplicationFactor int64
}

type UpdateMirrormakerConfigSpec struct {
	SourceCluster     UpdateClusterConnectionSpec
	TargetCluster     UpdateClusterConnectionSpec
	Topics            optional.String
	ReplicationFactor optional.Int64
}

type ClusterConnectionSpec struct {
	Alias                     string
	Type                      string
	BootstrapServers          string
	SaslUsername              string
	SaslPassword              secret.String
	SaslMechanism             string
	SecurityProtocol          string
	SslTruststoreCertificates string
}

type UpdateClusterConnectionSpec struct {
	Alias                     optional.String
	Type                      optional.String
	BootstrapServers          optional.String
	SaslUsername              optional.String
	SaslPassword              secret.String
	SaslPasswordUpdated       bool
	SaslMechanism             optional.String
	SecurityProtocol          optional.String
	SslTruststoreCertificates optional.String
}

type S3ConnectionSpec struct {
	Type            string
	AccessKeyID     string
	SecretAccessKey secret.String
	BucketName      string
	Endpoint        string
	Region          string
}

type S3SinkConnectorConfigSpec struct {
	S3Connection        S3ConnectionSpec
	Topics              string
	FileCompressionType string
	FileMaxRecords      int64
}

type UpdateS3ConnectionSpec struct {
	Type                   optional.String
	AccessKeyID            optional.String
	SecretAccessKey        secret.String
	SecretAccessKeyUpdated bool
	BucketName             optional.String
	Endpoint               optional.String
	Region                 optional.String
}

type UpdateS3SinkConnectorConfigSpec struct {
	S3Connection   UpdateS3ConnectionSpec
	Topics         optional.String
	FileMaxRecords optional.Int64
}

type Connector struct {
	Health                ConnectorHealth
	Status                ConnectorStatus
	Name                  string
	TasksMax              int64
	Properties            map[string]string
	Type                  string
	MirrormakerConfig     MirrormakerConfig
	S3SinkConnectorConfig S3SinkConnectorConfig
}

type MirrormakerConfig struct {
	SourceCluster     ClusterConnection
	TargetCluster     ClusterConnection
	Topics            string
	ReplicationFactor int64
}

type ClusterConnection struct {
	Alias            string
	Type             string
	BootstrapServers string
	SaslUsername     string
	SaslMechanism    string
	SecurityProtocol string
}

type S3Connection struct {
	Type        string
	AccessKeyID string
	BucketName  string
	Endpoint    string
	Region      string
}

type S3SinkConnectorConfig struct {
	S3Connection        S3Connection
	Topics              string
	FileCompressionType string
	FileMaxRecords      int64
}

var ConnectorNameValidator = models.MustConnectorNameValidator(models.DefaultConnectorNamePattern, nil)

func ValidateConnectorName(connectorName string) error {
	return ConnectorNameValidator.ValidateString(connectorName)
}

func (cs ConnectorSpec) Validate() error {
	if err := ValidateConnectorName(cs.Name); err != nil {
		return err
	}
	if cs.Type == ConnectorTypeMirrormaker {
		return cs.MirrormakerConfig.Validate(cs.Name)
	} else if cs.Type == ConnectorTypeS3Sink {
		return cs.S3SinkConnectorConfig.Validate(cs.Name)
	} else if cs.Type == ConnectorTypeUnspecified {
		return semerr.InvalidInput("connector type unspecified")
	} else {
		return semerr.InvalidInputf("unknown connector type - %q", cs.Type)
	}
}

func (mm MirrormakerConfigSpec) Validate(connectorName string) error {
	if mm.SourceCluster.Type == ClusterConnectionTypeThisCluster && mm.TargetCluster.Type == ClusterConnectionTypeThisCluster {
		return semerr.InvalidInput("cannot create connector with source and target type - this cluster")
	}
	if mm.SourceCluster.Type != ClusterConnectionTypeThisCluster && mm.TargetCluster.Type != ClusterConnectionTypeThisCluster {
		return semerr.InvalidInput("cannot create connector without source or target type - this cluster")
	}
	if err := mm.SourceCluster.Validate(); err != nil {
		return err
	}
	if err := mm.TargetCluster.Validate(); err != nil {
		return err
	}
	return nil
}

func TrimBootstrapServers(bs string) string {
	bs = strings.TrimSpace(bs)
	bsParts := strings.Split(bs, ",")
	for i, part := range bsParts {
		bsParts[i] = strings.TrimSpace(part)
	}
	return strings.Join(bsParts, ",")
}

var PortOfFqdnMaskValidator = models.MustPortOfFqdnValidator(models.DefaultPortOfFqdnPattern, nil)

func ValidatePort(port string) error {
	if port == "" {
		return semerr.InvalidInput("empty port")
	}
	err := PortOfFqdnMaskValidator.ValidateString(port)
	if err != nil {
		return err
	}
	portNumber, err := strconv.Atoi(port)
	if err != nil {
		return semerr.InvalidInput("can't convert port to number: " + port)
	}
	if portNumber < 1 || portNumber > 65535 {
		return semerr.InvalidInput("port " + port + " not in range [1, 65535]")
	}
	return nil
}

var Ipv4MaskValidator = models.MustIpv4Validator(models.DefaultIpv4Pattern, nil)

func IsValidIpv4Address(ip string) bool {
	if err := Ipv4MaskValidator.ValidateString(ip); err != nil {
		return false
	}
	ipParts := strings.Split(ip, ".")
	for _, ipPart := range ipParts {
		ipPartNumber, err := strconv.Atoi(ipPart)
		if err != nil || ipPartNumber < 0 || ipPartNumber > 255 {
			return false
		}
	}
	return true
}

var DomainNameValidator = models.MustDomainNameValidator(models.DefaultDomainNamePattern, nil)

func ValidateHost(host string) error {
	if host == "" {
		return semerr.InvalidInput("empty host")
	}
	if IsValidIpv4Address(host) {
		return nil
	}
	return DomainNameValidator.ValidateString(host)
}

func ValidateBootstrapServer(bootstrapServer string) error {
	parts := strings.Split(bootstrapServer, ":")
	if err := ValidateHost(parts[0]); err != nil {
		return semerr.InvalidInputf("host %q is not valid ip-address or domain-name",
			parts[0])
	}
	if len(parts) == 1 {
		return semerr.InvalidInputf("no port in bootstrap_server %q", bootstrapServer)
	}
	return ValidatePort(parts[1])
}

func ValidateBootstrapServers(bootstrapServers string) error {
	bootstrapServers = TrimBootstrapServers(bootstrapServers)
	if bootstrapServers == "" {
		return semerr.InvalidInput("bootstrap servers cannot be empty")
	}
	bsParts := strings.Split(bootstrapServers, ",")
	for _, bootstrapServer := range bsParts {
		if err := ValidateBootstrapServer(bootstrapServer); err != nil {
			return err
		}
	}
	return nil
}

func (cc ClusterConnectionSpec) Validate() error {
	if cc.Type == ClusterConnectionTypeUnspecified {
		return semerr.InvalidInput("cannot create connector with empty cluster connection spec")
	}
	if cc.Type == ClusterConnectionTypeThisCluster {
		return nil
	}
	return ValidateBootstrapServers(cc.BootstrapServers)
}

var S3BucketNameValidator = models.MustS3BucketNameValidator(models.DefaultS3BucketNamePattern, nil)

func ValidateS3BucketName(bucketName string) error {
	return S3BucketNameValidator.ValidateString(bucketName)
}

func (s3Connection S3ConnectionSpec) Validate() error {
	if err := ValidateS3BucketName(s3Connection.BucketName); err != nil {
		return err
	}
	if s3Connection.Type == S3ConnectionTypeExternal {
		return ValidateHost(s3Connection.Endpoint)
	} else {
		return semerr.InvalidInputf(
			"Unknown s3-connection type: %q",
			s3Connection.Type,
		)
	}
}

func ValidateFileCompressionType(compressionType string) error {
	var supportedCompressionTypes = [4]string{"none", "gzip", "snappy", "zstd"}
	var supported = false
	for _, supportedCompressionType := range supportedCompressionTypes {
		if supportedCompressionType == compressionType {
			supported = true
		}
	}
	if !supported {
		return semerr.InvalidInputf(
			"compression type %q is not supported. Use one of this: {none, gzip, snappy, zstd}",
			compressionType,
		)
	}
	return nil
}

func (s3SinkConnector S3SinkConnectorConfigSpec) Validate(connectorName string) error {
	if err := s3SinkConnector.S3Connection.Validate(); err != nil {
		return err
	}
	if err := ValidateFileCompressionType(s3SinkConnector.FileCompressionType); err != nil {
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
