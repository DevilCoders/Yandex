package config

import (
	"fmt"
)

type ServiceConfig struct {
	S3SyncConfig *S3SyncConfig `config:"sync-s3,required" yaml:"sync-s3"`
	S3LogoConfig *S3LogoConfig `config:"logo-s3,required" yaml:"logo-s3"`

	YcClient          *YcClient          `config:"yc-client,required" yaml:"yc-client"`
	MarketplaceClient *MarketplaceClient `config:"marketplace-client,required" yaml:"marketplace-client"`
	Logger            *Logger            `config:"logger,required" yaml:"logger"`
	Tracer            *Tracer            `config:"tracer" yaml:"tracer"`
}

func (c ServiceConfig) Validate() error {
	if c.S3SyncConfig.Bucket == "" || c.S3SyncConfig.Endpoint == "" || c.S3SyncConfig.Region == "" || c.S3SyncConfig.StandPrefix == "" || c.S3SyncConfig.SecretKey == "" || c.S3SyncConfig.AccessKey == "" {
		return fmt.Errorf("some sync-s3 params are not filled")
	}
	if c.S3LogoConfig.Bucket == "" || c.S3LogoConfig.Endpoint == "" || c.S3LogoConfig.Region == "" || c.S3LogoConfig.SecretKey == "" || c.S3SyncConfig.AccessKey == "" {
		return fmt.Errorf("some logo-s3 params are not filled")
	}
	if c.YcClient.ComputeEndpoint == "" {
		return fmt.Errorf("incorrect cloud compute API endpoint")
	}
	if c.YcClient.OperationEndpoint == "" {
		return fmt.Errorf("incorrect cloud operation API endpoint")
	}
	return nil
}
