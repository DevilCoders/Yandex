package collector

type YamlCollectorCfg struct {
	Type     string            `yaml:"type"`
	Tasks    map[string]string `yaml:"tasks"`
	Interval uint8             `yaml:"interval"`
}
