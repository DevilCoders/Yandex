package config

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type V3 struct {
	LogLevel           log.Level               `yaml:"log_level"`
	Output             pretty.Format           `yaml:"output"`
	LogHTTPBody        bool                    `yaml:"log_httpbody"`
	DeployAPIToken     string                  `yaml:"deploy_api_token"`
	CAPath             string                  `yaml:"capath"`
	DefaultDeployment  string                  `yaml:"default_deployment"`
	Deployments        map[string]DeploymentV3 `yaml:"deployments"`
	SelectedDeployment string
}

// DeploymentNames returns list of all configured deployments
func (v3 V3) DeploymentNames() []string {
	names := make([]string, 0, len(v3.Deployments))
	for d := range v3.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v3 V3) DeploymentConfig() DeploymentV3 {
	d := v3.DeploymentName()
	return v3.Deployments[d]
}

func (v3 V3) DeploymentName() string {
	if v3.SelectedDeployment != "" {
		return v3.SelectedDeployment
	}
	return v3.DefaultDeployment
}

func (v3 *V3) Validate() error {
	for _, d := range v3.Deployments {
		if err := d.Validate(); err != nil {
			return err
		}
	}

	return nil
}

// DeploymentV3 describes one MDB deployment
type DeploymentV3 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI                       string                 `yaml:"iam_uri"`
	ControlplaneFQDNSuffix       string                 `yaml:"controlplane_fqdn_suffix,omitempty"`
	UnamangedDataplaneFQDNSuffix string                 `yaml:"unmanaged_dataplane_fqdn_suffix,omitempty"`
	ManagedDataplaneFQDNSuffix   string                 `yaml:"managed_dataplane_fqdn_suffix,omitempty"`
	Federation                   DeploymentFederationV3 `yaml:"federation,omitempty"`
	Token                        DeploymentTokenV3      `yaml:"token,omitempty"`
	CAPath                       string                 `yaml:"ca_path,omitempty"`
}

type DeploymentTokenV3 struct {
	IAMToken  string    `yaml:"iam_token,omitempty"`
	ExpiresAt time.Time `yaml:"expires_at,omitempty"`
}

type DeploymentFederationV3 struct {
	ID       string `yaml:"id,omitempty"`
	Endpoint string `yaml:"endpoint,omitempty"`
}

func (d *DeploymentV3) Validate() error {
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

func (t DeploymentTokenV3) Validate() error {
	if t.IAMToken == "" {
		return xerrors.New("token is empty")
	}
	if t.ExpiresAt.Before(time.Now().Add(time.Minute)) {
		return xerrors.New("token is expired")
	}
	return nil
}

func (v3 V3) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v3.LogLevel
	cfg.Output = v3.Output
	cfg.LogHTTPBody = v3.LogHTTPBody
	cfg.DeployAPIToken = v3.DeployAPIToken
	cfg.CAPath = v3.CAPath
	cfg.DefaultDeployment = v3.DefaultDeployment
	cfg.SelectedDeployment = v3.SelectedDeployment
	return cfg
}
