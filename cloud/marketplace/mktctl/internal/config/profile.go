package config

import (
	"errors"
	"os"
	"time"
)

type Profile struct {
	Name                        string `config:"-" yaml:"-"`
	FederationID                string `config:"federationID,required" yaml:"federation-id"`
	FederationEndpoint          string `config:"federationEndpoint,required" yaml:"federation-endpoint"`
	MarketplaceConsoleEndpoint  string `config:"marketplaceConsoleEndpoint,required" yaml:"marketplace-console-endpoint"`
	MarketplacePartnersEndpoint string `config:"marketplacePartnersEndpoint,required" yaml:"marketplace-partners-endpoint"`
	MarketplacePrivateEndpoint  string `config:"marketplacePrivateEndpoint,required" yaml:"marketplace-private-endpoint"`
}

// mb always refresh saved token and then load it?
// in refresh function we could check if actual refresh is needed
func (p *Profile) getToken() string {
	savedFederationToken, err := getSavedFederationToken(p.Name)
	if errors.Is(err, os.ErrNotExist) {
		mustRefreshSavedFederationToken(p.Name, p.FederationID, p.FederationEndpoint)

		return p.getToken()
	}
	if err != nil {
		panic(err)
	}

	expiresAt, err := time.Parse(time.RFC3339, savedFederationToken.ExpiresAt)
	if err != nil {
		panic(err)
	}

	if expiresAt.Before(time.Now()) {
		mustRefreshSavedFederationToken(p.Name, p.FederationID, p.FederationEndpoint)

		return p.getToken()
	}

	return savedFederationToken.Token
}
