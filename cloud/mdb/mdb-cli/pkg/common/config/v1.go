package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
)

type V1 struct {
	LogLevel          log.Level               `yaml:"log_level"`
	Output            pretty.Format           `yaml:"output"`
	LogHTTPBody       bool                    `yaml:"log_httpbody"`
	DeployAPIToken    string                  `yaml:"deploy_api_token"`
	CAPath            string                  `yaml:"capath"`
	DefaultDeployment string                  `yaml:"default_deployment"`
	Deployments       map[string]DeploymentV1 `yaml:"deployments"`
}

// DeploymentNames returns list of all configured deployments
func (v1 V1) DeploymentNames() []string {
	names := make([]string, 0, len(v1.Deployments))
	for d := range v1.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v1 V1) DeploymentConfig() DeploymentV1 {
	d := v1.DefaultDeployment
	return v1.Deployments[d]
}

func (v1 V1) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v1.LogLevel
	cfg.Output = v1.Output
	cfg.LogHTTPBody = v1.LogHTTPBody
	cfg.DeployAPIToken = v1.DeployAPIToken
	cfg.CAPath = v1.CAPath
	cfg.DefaultDeployment = v1.DefaultDeployment
	return cfg
}

// DeploymentV1 describes one MDB deployment
type DeploymentV1 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI              string `yaml:"iam_uri"`
	DataplaneFQDNSuffix string `yaml:"dataplane_fqdn_suffix,omitempty"`
}
