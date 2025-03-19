package juggler

type JugglerConfig struct {
	UnreachableServiceWindowMin int `json:"unreachachable_service_window" yaml:"unreachachable_service_window"`
}

func DefaultJugglerConfig() JugglerConfig {
	return JugglerConfig{
		UnreachableServiceWindowMin: 10,
	}
}
