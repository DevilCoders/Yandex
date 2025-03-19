package config

type Service struct {
	AccessService *AccessService `config:"access-service,required" yaml:"access-service"`

	HTTP *HTTPService `config:"http" yaml:"http"`
	GRPC *GRPCService `config:"grpc,required" yaml:"grpc"`

	BillingClient   *BillingClient   `config:"billing-client,required" yaml:"billing-client"`
	ResourceManager *ResourceManager `config:"resource-manager,required" yaml:"resource-manager"`

	YDB *YDB `config:"ydb,required" yaml:"ydb"`

	Logger *Logger `config:"logger,required" yaml:"logger"`

	Monitoring *Monitoring `config:"monitoring" yaml:"monitoring"`
	Tracer     *Tracer     `config:"tracer" yaml:"tracer"`
}
