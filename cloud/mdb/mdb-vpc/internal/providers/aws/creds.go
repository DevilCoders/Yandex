package aws

import (
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/aws/aws-sdk-go/service/sts"
)

type SAConfig struct {
	RoleARN   string `json:"role_arn" yaml:"role_arn"`
	TokenPath string `json:"token_path" yaml:"token_path"`
}

func DefaultSAConfig() SAConfig {
	return SAConfig{
		TokenPath: "/var/run/secrets/eks.amazonaws.com/serviceaccount/token",
	}
}

func NewSACreds(cfg SAConfig) *credentials.Credentials {
	provider := stscreds.NewWebIdentityRoleProviderWithOptions(
		sts.New(NewSession(nil, "", nil)),
		cfg.RoleARN,
		"", // This simply uses unix time in nanoseconds to uniquely identify sessions.
		stscreds.FetchTokenPath(cfg.TokenPath),
	)
	return credentials.NewCredentials(provider)
}
