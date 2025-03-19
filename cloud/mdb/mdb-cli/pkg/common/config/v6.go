package config

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type V6 struct {
	LogLevel           log.Level               `yaml:"log_level"`
	Output             pretty.Format           `yaml:"output"`
	LogHTTPBody        bool                    `yaml:"log_httpbody"`
	DeployAPIToken     string                  `yaml:"deploy_api_token"`
	TrackerToken       string                  `yaml:"tracker_token"`
	IamOauthToken      string                  `yaml:"iam_oauth_token"`
	CAPath             string                  `yaml:"capath"`
	DefaultDeployment  string                  `yaml:"default_deployment"`
	Deployments        map[string]DeploymentV6 `yaml:"deployments"`
	SelectedDeployment string
}

// DeploymentNames returns list of all configured deployments
func (v6 V6) DeploymentNames() []string {
	names := make([]string, 0, len(v6.Deployments))
	for d := range v6.Deployments {
		names = append(names, d)
	}

	return names
}

// DeploymentConfig returns config for chosen deployment
func (v6 V6) DeploymentConfig() DeploymentV6 {
	d := v6.DeploymentName()
	return v6.Deployments[d]
}

func (v6 V6) DeploymentName() string {
	if v6.SelectedDeployment != "" {
		return v6.SelectedDeployment
	}
	return v6.DefaultDeployment
}

func (v6 *V6) Validate() error {
	for _, d := range v6.Deployments {
		if err := d.Validate(); err != nil {
			return err
		}
	}

	return nil
}

// DeploymentV6 describes one MDB deployment
type DeploymentV6 struct {
	// DeployAPIURL is MDB Deploy API address
	DeployAPIURL string `yaml:"deploy_api_uri"`
	// MDBAPIURI is MDB Internal API address
	MDBAPIURI string `yaml:"mdb_api_uri"`
	// MDBUIURI is MDB UI address
	MDBUIURI string `yaml:"mdb_ui_uri"`
	// IAMURI is IAM address for exchanging OAuth token for IAM token
	IAMURI                       string                 `yaml:"iam_uri"`
	TokenServiceHost             string                 `yaml:"token_service_host"`
	CMSHost                      string                 `yaml:"cms_host"`
	ControlplaneFQDNSuffix       string                 `yaml:"controlplane_fqdn_suffix,omitempty"`
	UnamangedDataplaneFQDNSuffix string                 `yaml:"unmanaged_dataplane_fqdn_suffix,omitempty"`
	ManagedDataplaneFQDNSuffix   string                 `yaml:"managed_dataplane_fqdn_suffix,omitempty"`
	Federation                   DeploymentFederationV6 `yaml:"federation,omitempty"`
	Token                        DeploymentTokenV6      `yaml:"token,omitempty"`
	CAPath                       string                 `yaml:"ca_path,omitempty"`
}

type DeploymentTokenV6 struct {
	IAMToken  string    `yaml:"iam_token,omitempty"`
	ExpiresAt time.Time `yaml:"expires_at,omitempty"`
}

type DeploymentFederationV6 struct {
	ID       string `yaml:"id,omitempty"`
	Endpoint string `yaml:"endpoint,omitempty"`
}

func (d *DeploymentV6) Validate() error {
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

func (t DeploymentTokenV6) Validate() error {
	if t.IAMToken == "" {
		return xerrors.New("token is empty")
	}
	if t.ExpiresAt.Before(time.Now().Add(time.Minute)) {
		return xerrors.New("token is expired")
	}
	return nil
}

func (v6 V6) Upgrade() Config {
	cfg := DefaultConfig()
	cfg.LogLevel = v6.LogLevel
	cfg.Output = v6.Output
	cfg.LogHTTPBody = v6.LogHTTPBody
	cfg.DeployAPIToken = v6.DeployAPIToken
	cfg.CAPath = v6.CAPath
	cfg.DefaultDeployment = v6.DefaultDeployment
	cfg.SelectedDeployment = v6.SelectedDeployment
	cfg.TrackerToken = v6.TrackerToken
	cfg.IamOauthToken = v6.IamOauthToken
	return cfg
}
