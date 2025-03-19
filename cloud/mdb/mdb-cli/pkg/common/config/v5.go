package config

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type V5 struct {
	LogLevel           log.Level               `yaml:"log_level"`
	Output             pretty.Format           `yaml:"output"`
	LogHTTPBody        bool                    `yaml:"log_httpbody"`
	DeployAPIToken     string                  `yaml:"deploy_api_token"`
	TrackerToken       string                  `yaml:"tracker_token"`
	CAPath             string                  `yaml:"capath"`
	DefaultDeployment  string                  `yaml:"default_deployment"`
	Deployments        map[string]DeploymentV5 `yaml:"deployments"`
	SelectedDeployment string
}

// DeploymentNames returns list of all configured deployments
func (v5 V5) DeploymentNames() []string {
	names := make([]string, 0, len(v5.Deployments))
	for d := range v5.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v5 V5) DeploymentConfig() DeploymentV5 {
	d := v5.DeploymentName()
	return v5.Deployments[d]
}

func (v5 V5) DeploymentName() string {
	if v5.SelectedDeployment != "" {
		return v5.SelectedDeployment
	}
	return v5.DefaultDeployment
}

func (v5 *V5) Validate() error {
	for _, d := range v5.Deployments {
		if err := d.Validate(); err != nil {
			return err
		}
	}

	return nil
}

// DeploymentV5 describes one MDB deployment
type DeploymentV5 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// MDBUIURI is MDB UI address
	MDBUIURI string `yaml:"mdb_ui_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI                       string                 `yaml:"iam_uri"`
	CMSHost                      string                 `yaml:"cms_host"`
	ControlplaneFQDNSuffix       string                 `yaml:"controlplane_fqdn_suffix,omitempty"`
	UnamangedDataplaneFQDNSuffix string                 `yaml:"unmanaged_dataplane_fqdn_suffix,omitempty"`
	ManagedDataplaneFQDNSuffix   string                 `yaml:"managed_dataplane_fqdn_suffix,omitempty"`
	Federation                   DeploymentFederationV5 `yaml:"federation,omitempty"`
	Token                        DeploymentTokenV5      `yaml:"token,omitempty"`
	CAPath                       string                 `yaml:"ca_path,omitempty"`
}

type DeploymentTokenV5 struct {
	IAMToken  string    `yaml:"iam_token,omitempty"`
	ExpiresAt time.Time `yaml:"expires_at,omitempty"`
}

type DeploymentFederationV5 struct {
	ID       string `yaml:"id,omitempty"`
	Endpoint string `yaml:"endpoint,omitempty"`
}

func (d *DeploymentV5) Validate() error {
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

func (t DeploymentTokenV5) Validate() error {
	if t.IAMToken == "" {
		return xerrors.New("token is empty")
	}
	if t.ExpiresAt.Before(time.Now().Add(time.Minute)) {
		return xerrors.New("token is expired")
	}
	return nil
}

func (v5 V5) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v5.LogLevel
	cfg.Output = v5.Output
	cfg.LogHTTPBody = v5.LogHTTPBody
	cfg.DeployAPIToken = v5.DeployAPIToken
	cfg.CAPath = v5.CAPath
	cfg.DefaultDeployment = v5.DefaultDeployment
	cfg.SelectedDeployment = v5.SelectedDeployment
	cfg.TrackerToken = v5.TrackerToken
	return cfg
}
