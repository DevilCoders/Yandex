package config

type BillingClient struct {
	Endpoint string `config:"endpoint" yaml:"endpoint"`

	RetryCount int `config:"retry_count" yaml:"retry_count"`
}
