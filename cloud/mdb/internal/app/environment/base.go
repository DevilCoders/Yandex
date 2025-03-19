package environment

type EnvironmentCfg struct {
	Services ServicesCfg `json:"services" yaml:"services"`
}

type ServicesCfg struct {
	Iam IAM `json:"iam" yaml:"iam"`
}

type SimpleServiceCfg struct {
	Endpoint string `json:"endpoint" yaml:"endpoint"`
}
