package cfgtypes

type AuthConfig struct {
	Type           AuthType `yaml:"type" config:"type"`
	TVMDestination uint     `yaml:"tvm_destination,omitempty" config:"tvm_destination"`
	Token          string   `yaml:"token,omitempty" config:"token"`
	TokenFile      string   `yaml:"token_file,omitempty" config:"token_file"`
}

type TVMConfig struct {
	ClientID string `yaml:"client_id,omitempty" config:"client_id"`
	APIURL   string `yaml:"api_url,omitempty" config:"api_url"`
	APIAuth  string `yaml:"api_auth,omitempty" config:"api_auth"`
}

type IAMMetaConfig struct {
	UseLocalhost OverridableBool `yaml:"use_localhost,omitempty" config:"use_localhost"`
}

type JWTConfig struct {
	Endpoint string `yaml:"endpoint,omitempty" config:"endpoint"`
	KeyFile  string `yaml:"key_file,omitempty" config:"key_file"`
	Key      JWTKey `yaml:"key,omitempty" config:"key"`
	Audience string `yaml:"audience,omitempty" config:"audience"`
}

type JWTKey struct {
	ServiceAccountID string `json:"service_account_id" yaml:"service_account_id" config:"service_account_id"`
	ID               string `json:"id" yaml:"id" config:"id"`
	PrivateKey       string `json:"private_key" yaml:"private_key" config:"private_key"`
}
