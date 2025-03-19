package cfgtypes

type RMConfig struct {
	Endpoint string     `yaml:"endpoint" config:"endpoint"`
	Auth     AuthConfig `yaml:"auth" config:"auth"`
	GRPC     GRPCConfig `yaml:"grpc,omitempty" config:"grpc"`
}

type TeamIntegrationConfig struct {
	Endpoint string     `yaml:"endpoint" config:"endpoint"`
	Auth     AuthConfig `yaml:"auth" config:"auth"`
	GRPC     GRPCConfig `yaml:"grpc,omitempty" config:"grpc"`
}

type UAConfig struct {
	SolomonMetricsPort uint `yaml:"solomon_metrics_port" config:"solomon_metrics_port"`
	HealthCheckPort    uint `yaml:"health_check_port" config:"health_check_port"`
}

type GRPCConfig struct {
	DisableTLS          OverridableBool `yaml:"disable_tls,omitempty" config:"disable_tls"`
	RequestRetries      uint            `yaml:"request_retries,omitempty" config:"request_retries"`
	RequestRetryTimeout Seconds         `yaml:"request_retry_timeout,omitempty" config:"request_retry_timeout"`
	KeepAliveTime       Seconds         `yaml:"keepalive_time,omitempty" config:"keepalive_time"`
	KeepAliveTimeout    Seconds         `yaml:"keepalive_timeout,omitempty" config:"keepalive_timeout"`
}
