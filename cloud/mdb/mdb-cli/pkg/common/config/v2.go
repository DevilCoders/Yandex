package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type V2 struct {
	LogLevel          log.Level               `yaml:"log_level"`
	Output            pretty.Format           `yaml:"output"`
	LogHTTPBody       bool                    `yaml:"log_httpbody"`
	DeployAPIToken    string                  `yaml:"deploy_api_token"`
	CAPath            string                  `yaml:"capath"`
	DefaultDeployment string                  `yaml:"default_deployment"`
	Deployments       map[string]DeploymentV2 `yaml:"deployments"`
}

// DeploymentNames returns list of all configured deployments
func (v2 V2) DeploymentNames() []string {
	names := make([]string, 0, len(v2.Deployments))
	for d := range v2.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v2 V2) DeploymentConfig() DeploymentV2 {
	d := v2.DefaultDeployment
	return v2.Deployments[d]
}

func (v2 *V2) Validate() error {
	for _, d := range v2.Deployments {
		if err := d.Validate(); err != nil {
			return err
		}
	}

	return nil
}

// DeploymentV2 describes one MDB deployment
type DeploymentV2 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI                       string `yaml:"iam_uri"`
	ControlplaneFQDNSuffix       string `yaml:"controlplane_fqdn_suffix,omitempty"`
	UnamangedDataplaneFQDNSuffix string `yaml:"unmanaged_dataplane_fqdn_suffix,omitempty"`
	ManagedDataplaneFQDNSuffix   string `yaml:"managed_dataplane_fqdn_suffix,omitempty"`
}

func (v2 V2) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v2.LogLevel
	cfg.Output = v2.Output
	cfg.LogHTTPBody = v2.LogHTTPBody
	cfg.DeployAPIToken = v2.DeployAPIToken
	cfg.CAPath = v2.CAPath
	cfg.DefaultDeployment = v2.DefaultDeployment
	return cfg
}

func (d *DeploymentV2) Validate() error {
	if (d.ManagedDataplaneFQDNSuffix == "") != (d.UnamangedDataplaneFQDNSuffix == "") {
		return xerrors.New("managed and unmanaged data plane fqdn suffixes must either be both set or both unset")
	}

	if d.ManagedDataplaneFQDNSuffix == "" && d.ControlplaneFQDNSuffix != "" {
		return xerrors.New("control plane fqdn suffix cannot be set when data plane suffixes are not set")
	}

	if d.ManagedDataplaneFQDNSuffix != "" && d.ControlplaneFQDNSuffix == "" {
		return xerrors.New("control plane fqdn suffix must be set when data plane suffixes are not set")
	}

	return nil
}
