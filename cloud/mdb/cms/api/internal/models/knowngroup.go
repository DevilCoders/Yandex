package models

type KnownGroups struct {
	Type         string   `json:"type" yaml:"type"`
	CGroups      []string `json:"groups" yaml:"groups"`
	Checks       []string `json:"checks" yaml:"checks"`
	MaxUnhealthy int      `json:"max_unhealthy" yaml:"max_unhealthy"`
}
