package config

type Service struct {
	GRPC              *GRPCService       `config:"grpc,required" yaml:"grpc"`
	BillingClient     *BillingClient     `config:"billing-client,required" yaml:"billing-client"`
	MarketplaceClient *MarketplaceClient `config:"marketplace-client,required" yaml:"marketplace-client"`
	YDB               *YDB               `config:"ydb,required" yaml:"ydb"`
	Logger            *Logger            `config:"logger,required" yaml:"logger"`
	Prefix            string             `config:"prefix,required" yaml:"prefix"`
}

type MigrationService struct {
	YDB    *YDB    `config:"ydb,required" yaml:"ydb"`
	Logger *Logger `config:"logger,required" yaml:"logger"`
}

type AutorecreateWorkerService struct {
	YDB           *YDB           `config:"ydb,required" yaml:"ydb"`
	BillingClient *BillingClient `config:"billing-client,required" yaml:"billing-client"`
	Worker        *WorkerService `config:"worker,required" yaml:"worker"`
	Logger        *Logger        `config:"logger,required" yaml:"logger"`
	Prefix        string         `config:"prefix,required" yaml:"prefix"`
}
