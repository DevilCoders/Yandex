package console

type BillingEstimate struct {
	// List of billing metrics of estimation
	Metrics []BillingMetric `json:"metrics" yaml:"metrics"`
}

type BillingMetric struct {
	// ID of the folder for metric
	FolderID string `json:"folder_id" yaml:"folder_id"`
	// Billing metric schema spec
	Schema string `json:"schema" yaml:"schema"`
	// Billing tags
	Tags BillingMetricTags `json:"tags" yaml:"tags"`
}

type BillingMetricTags struct {
	CloudProvider                   string   `json:"cloud_provider" yaml:"cloud_provider"`
	CloudRegion                     string   `json:"cloud_region" yaml:"cloud_region"`
	PublicIP                        int64    `json:"public_ip" yaml:"public_ip"`
	DiskTypeID                      string   `json:"disk_type_id" yaml:"disk_type_id"`
	ClusterType                     string   `json:"cluster_type" yaml:"cluster_type"`
	DiskSize                        int64    `json:"disk_size" yaml:"disk_size"`
	ResourcePresetID                string   `json:"resource_preset_id" yaml:"resource_preset_id"`
	ResourcePresetType              string   `json:"resource_preset_type" yaml:"resource_preset_type"`
	PlatformID                      string   `json:"platform_id" yaml:"platform_id"`
	Cores                           int64    `json:"cores" yaml:"cores"`
	CoreFraction                    int64    `json:"core_fraction" yaml:"core_fraction"`
	Memory                          int64    `json:"memory" yaml:"memory"`
	SoftwareAcceleratedNetworkCores int64    `json:"software_accelerated_network_cores" yaml:"software_accelerated_network_cores"`
	Roles                           []string `json:"roles" yaml:"roles"`
	Online                          int64    `json:"online" yaml:"online"`
	OnDedicatedHost                 int64    `json:"on_dedicated_host" yaml:"on_dedicated_host"`
	Edition                         string   `json:"edition" yaml:"edition"`
}
