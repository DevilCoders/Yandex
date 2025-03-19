package config

import (
	"context"
	"fmt"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"
	"github.com/imdario/mergo"
	"github.com/mitchellh/go-homedir"
)

const (
	DefaultConfigPath        = "~/.config/mktctl/config.yaml"
	defaultConfigLoadTimeout = 10 * time.Second
)

// add context for logger?
type Config struct {
	Current       string              `config:"current"`
	Profiles      map[string]*Profile `config:"profiles"`
	TokenOverride string              `config:"-"`
	Format        string              `config:"-"`
	LogLevel      string              `config:"-"`
}

func New() *Config {
	return &Config{}
}

// ToDo:
//	* add env parsing for second-load confita overrides?
func (c *Config) Load() error {
	configPath, err := homedir.Expand(DefaultConfigPath)
	if err != nil {
		return err
	}

	ctx, cancel := context.WithTimeout(context.Background(), defaultConfigLoadTimeout)
	defer cancel()

	tmpCfg := Config{}
	err = confita.NewLoader(env.NewBackend(), file.NewBackend(configPath)).Load(ctx, &tmpCfg)
	if err != nil {
		return err
	}

	// confita ovewrite fields
	// we want cobra to have a chance to override'em
	// thou empty field will be overriden which is fine
	err = mergo.Merge(c, tmpCfg)
	if err != nil {
		return err
	}

	if c.Current == "" {
		return ErrProfileNotSet
	}

	_, exist := c.Profiles[c.Current]
	if !exist {
		return ErrProfileNotFound
	}

	// fill names
	for k, v := range c.Profiles {
		v.Name = k
	}

	return nil
}

func (c *Config) GetToken() string {
	if c.TokenOverride != "" {
		return c.TokenOverride
	}

	return c.getCurrentProfile().getToken()
}

func (c *Config) getCurrentProfile() *Profile {
	return c.GetProfile(c.Current)
}

func (c *Config) GetProfile(profileName string) *Profile {
	profile, exist := c.Profiles[profileName]
	if !exist {
		panic("non existent profile")
	}

	return profile
}

func (c *Config) GetMarketplaceConsoleEndpoint() string {
	return fmt.Sprintf("https://%v/marketplace/v2/console", c.getCurrentProfile().MarketplaceConsoleEndpoint)
}

func (c *Config) GetMarketplacePartnersEndpoint() string {
	return fmt.Sprintf("https://%v/marketplace/v2/partners", c.getCurrentProfile().MarketplacePartnersEndpoint)
}

func (c *Config) GetMarketplacePrivateEndpoint() string {
	return fmt.Sprintf("https://%v/marketplace/v2/private", c.getCurrentProfile().MarketplacePrivateEndpoint)
}
