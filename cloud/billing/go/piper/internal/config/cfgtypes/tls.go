package cfgtypes

type TLSConfig struct {
	CAPath string `yaml:"ca_path,omitempty" config:"ca_path"`
}
