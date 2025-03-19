package environment

type IAM struct {
	V1 IAMv1 `json:"v1" yaml:"v1"`
}

type IAMv1 struct {
	TokenService SimpleServiceCfg `json:"token_service" yaml:"token_service"`
}
