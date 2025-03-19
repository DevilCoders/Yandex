package metaparser

import "context"

type CustomRule struct {
	MetricName string `yaml:"metric_name"`
}

type AutoScale struct {
	CustomRules []CustomRule `yaml:"custom_rules"`
}

type ScalePolicy struct {
	AutoScale AutoScale `yaml:"auto_scale"`
}

type InstanceGroupConfig struct {
	ScalePolicy ScalePolicy `yaml:"scale_policy"`
}

type Subcluster struct {
	Role                string              `yaml:"role"`
	HostsCount          int64               `yaml:"hosts_count"`
	InstanceGroupConfig InstanceGroupConfig `yaml:"instance_group_config"`
}

type Topology struct {
	Revision    int64                 `yaml:"revision"`
	Subclusters map[string]Subcluster `yaml:"subclusters"`
}

type UserData struct {
	Topology Topology `yaml:"topology"`
	S3Bucket string   `yaml:"s3_bucket"`
	UIProxy  bool     `yaml:"ui_proxy"`
	Services []string `yaml:"services"`
}

type InstanceAttributes struct {
	UserData UserData
	FolderID string
}

type ComputeMetadata struct {
	InstanceAttributes InstanceAttributes
	IAMToken           string
}

// Metaparser implements instance metadata info getter
type Metaparser interface {
	GetComputeMetadata(ctx context.Context) (ComputeMetadata, error)
}
