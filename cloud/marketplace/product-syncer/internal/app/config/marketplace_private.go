package config

type MarketplaceClient struct {
	Endpoint                 string `config:"endpoint,required" yaml:"endpoint"`
	OpTimeout                string `config:"OpTimeout,required" yaml:"operation-timeout"`
	OpPollInterval           string `config:"opPollInterval,required" yaml:"operation-poll-interval"`
	MktPendingImagesFolderID string `config:"MktPendingImagesFolderID,required" yaml:"mkt-pending-folder"`
}
