package config

//S3SyncConfig is a source of truth for version sync in mkt ru prod
type S3SyncConfig struct {
	Endpoint    string `config:"endpoint" yaml:"endpoint"`
	Region      string `config:"region" yaml:"region"`
	Bucket      string `config:"bucket" yaml:"bucket"`
	StandPrefix string `config:"standPrefix" yaml:"standPrefix"`
	AccessKey   string `config:"accessKey" yaml:"accessKey"`
	SecretKey   string `config:"secretKey" yaml:"secretKey"`
}

//S3LogoConfig process logos for 'version-images' and 'products' buckets in standard-images folder
type S3LogoConfig struct {
	Endpoint  string `config:"endpoint" yaml:"endpoint"`
	Region    string `config:"region" yaml:"region"`
	Bucket    string `config:"bucket" yaml:"bucket"`
	AccessKey string `config:"accessKey" yaml:"accessKey"`
	SecretKey string `config:"secretKey" yaml:"secretKey"`
}
