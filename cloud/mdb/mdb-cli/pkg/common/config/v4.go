package config

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type V4 struct {
	LogLevel           log.Level               `yaml:"log_level"`
	Output             pretty.Format           `yaml:"output"`
	LogHTTPBody        bool                    `yaml:"log_httpbody"`
	DeployAPIToken     string                  `yaml:"deploy_api_token"`
	CAPath             string                  `yaml:"capath"`
	DefaultDeployment  string                  `yaml:"default_deployment"`
	Deployments        map[string]DeploymentV4 `yaml:"deployments"`
	SelectedDeployment string
}

// DeploymentNames returns list of all configured deployments
func (v4 V4) DeploymentNames() []string {
	names := make([]string, 0, len(v4.Deployments))
	for d := range v4.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v4 V4) DeploymentConfig() DeploymentV4 {
	d := v4.DeploymentName()
	return v4.Deployments[d]
}

func (v4 V4) DeploymentName() string {
	if v4.SelectedDeployment != "" {
		return v4.SelectedDeployment
	}
	return v4.DefaultDeployment
}

func (v4 *V4) Validate() error {
	for _, d := range v4.Deployments {
		if err := d.Validate(); err != nil {
			return err
		}
	}

	return nil
}

// DeploymentV4 describes one MDB deployment
type DeploymentV4 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI                       string                 `yaml:"iam_uri"`
	CMSHost                      string                 `yaml:"cms_host"`
	ControlplaneFQDNSuffix       string                 `yaml:"controlplane_fqdn_suffix,omitempty"`
	UnamangedDataplaneFQDNSuffix string                 `yaml:"unmanaged_dataplane_fqdn_suffix,omitempty"`
	ManagedDataplaneFQDNSuffix   string                 `yaml:"managed_dataplane_fqdn_suffix,omitempty"`
	Federation                   DeploymentFederationV4 `yaml:"federation,omitempty"`
	Token                        DeploymentTokenV4      `yaml:"token,omitempty"`
	CAPath                       string                 `yaml:"ca_path,omitempty"`
}

type DeploymentTokenV4 struct {
	IAMToken  string    `yaml:"iam_token,omitempty"`
	ExpiresAt time.Time `yaml:"expires_at,omitempty"`
}

type DeploymentFederationV4 struct {
	ID       string `yaml:"id,omitempty"`
	Endpoint string `yaml:"endpoint,omitempty"`
}

func (d *DeploymentV4) Validate() error {
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

func (t DeploymentTokenV4) Validate() error {
	if t.IAMToken == "" {
		return xerrors.New("token is empty")
	}
	if t.ExpiresAt.Before(time.Now().Add(time.Minute)) {
		return xerrors.New("token is expired")
	}
	return nil
}

func (v4 V4) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v4.LogLevel
	cfg.Output = v4.Output
	cfg.LogHTTPBody = v4.LogHTTPBody
	cfg.DeployAPIToken = v4.DeployAPIToken
	cfg.CAPath = v4.CAPath
	cfg.DefaultDeployment = v4.DefaultDeployment
	cfg.SelectedDeployment = v4.SelectedDeployment
	return cfg
}
