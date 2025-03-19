package config

type YcClient struct {
	ComputeEndpoint   string `config:"compute-endpoint,required" yaml:"compute-endpoint"`
	OperationEndpoint string `config:"operation-endpoint,required" yaml:"operation-endpoint"`
	CAPath            string `config:"ca-path" yaml:"ca-path"`
	OpTimeout         string `config:"OpTimeout,required" yaml:"operation-timeout"`
	OpPollInterval    string `config:"OpPollInterval,required" yaml:"operation-poll-interval"`
}
