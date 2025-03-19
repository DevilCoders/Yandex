package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
)

// Config represents mdb-admin configuration
type V0 struct {
	LogLevel          log.Level     `yaml:"loglevel"`
	Output            pretty.Format `yaml:"output"`
	LogHTTPBody       bool          `yaml:"loghttpbody"`
	DeployAPIToken    string        `yaml:"deploy_api_token"`
	CAPath            string        `yaml:"capath"`
	DefaultDeployment string        `yaml:"defaultdeployment"`
}

func (v0 V0) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v0.LogLevel
	cfg.Output = v0.Output
	cfg.LogHTTPBody = v0.LogHTTPBody
	cfg.DeployAPIToken = v0.DeployAPIToken
	cfg.CAPath = v0.CAPath

	// Convert deployment name to new one
	deploymentsMapping := map[string]string{
		"computeprod":    DeploymentNameComputeProd,
		"computepreprod": DeploymentNameComputePreprod,
		"portoprod":      DeploymentNamePortoProd,
		"portotest":      DeploymentNamePortoTest,
	}
	if v, ok := deploymentsMapping[v0.DefaultDeployment]; ok {
		cfg.DefaultDeployment = v
	}

	return cfg
}
