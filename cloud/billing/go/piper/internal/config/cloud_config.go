package config

import (
	"bytes"
	"context"
	"errors"

	"github.com/heetch/confita/backend"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/adapters/lockbox"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/loader"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/auth/cloudmeta"
	cm "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/cloudmeta"
	lbx "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/lockbox"
)

type cloudContainer struct {
	lockboxEndpoint  string
	loadSecretConfig bool
	metadataAttrs    map[string]string

	lockboxConfigOnce       initSync
	metadataSecretOnce      initSync
	lockboxConfiguratorOnce initSync
	lockboxLoadOnce         initSync

	lockboxConfigurator FullConfigurator
	lockboxLoader       *loader.Loader
}

const metadataSecretKey = "lockbox-secret-id"

func (c *Container) GetMetadataSecretID() (string, error) {
	err := c.initializeMetadataConfig()
	return c.metadataAttrs[metadataSecretKey], err
}

func (c *Container) initializeLockboxConfig() error {
	return c.lockboxConfigOnce.Do(c.loadLockboxConfig)
}

func (c *Container) initializeMetadataConfig() error {
	return c.metadataSecretOnce.Do(c.loadMetadata)
}

func (c *Container) initializeLockboxConfigurator() error {
	return c.lockboxConfiguratorOnce.Do(c.makeLockboxConfigurator)
}

func (c *Container) initializeLockboxBackend() error {
	return c.lockboxLoadOnce.Do(c.makeLockboxBackends)
}

type lockboxLocalConfig struct {
	Lockbox cfgtypes.LockboxConfig `yaml:"lockbox" config:"lockbox"`
}

var defaultLockboxConfig = lockboxLocalConfig{
	Lockbox: cfgtypes.LockboxConfig{
		Endpont: "https://payload.lockbox.api.cloud.yandex.net",
	},
}

func (c *Container) loadLockboxConfig() error {
	c.initializedLocalLoader()
	if c.initFailed() {
		return c.initError
	}
	cfg := defaultLockboxConfig
	if err := c.localLoader.Load(c.mainCtx, &cfg); err != nil {
		return c.failInitF("lockbox local config: %w", err)
	}
	c.lockboxEndpoint = cfg.Lockbox.Endpont
	c.loadSecretConfig = cfg.Lockbox.Enabled.Bool()
	return nil
}

func (c *Container) loadMetadata() error {
	_ = c.initializeLockboxConfig()
	if c.initFailed() {
		return c.initError
	}
	if !c.loadSecretConfig {
		return nil
	}
	cl := cm.New()
	attrs, err := cl.GetAttributes(c.mainCtx)
	if err != nil {
		return c.failInitF("load instance metadata: %w", err)
	}
	c.metadataAttrs = attrs
	return nil
}

func (c *Container) makeLockboxConfigurator() error {
	_ = c.initializeLockboxConfig()
	if c.initFailed() {
		return c.initError
	}
	secretID, err := c.GetMetadataSecretID()
	if err != nil || secretID == "" {
		c.lockboxConfigurator = c.dummy()
		return err
	}

	// Always use VM metadata config for lockbox. May be add auth config in future
	auth := cloudmeta.New(c.mainCtx)

	// We only load config once during init, so client does not persist
	lbClient := lbx.New(c.lockboxEndpoint, auth)
	c.lockboxConfigurator = lockbox.NewConfigurator(c.mainCtx, lbClient, secretID)
	return nil
}

func (c *Container) makeLockboxBackends() error {
	_ = c.initializeLockboxConfigurator()
	if c.initFailed() {
		return c.initError
	}
	if c.lockboxConfigurator == nil {
		return nil
	}

	yaml, err := c.lockboxConfigurator.GetYaml(c.mainCtx)
	if err != nil {
		return c.failInitF("lockbox yaml load: %w", err)
	}
	nsConf, err := c.lockboxConfigurator.GetConfig(c.mainCtx, piperNamespace)
	if err != nil {
		return c.failInitF("lockbox namespace load: %w", err)
	}

	backends := []backend.Backend{}
	if len(yaml) > 0 {
		b := loader.WithMergeLoad(&lockboxYAMLBackend{data: yaml})
		backends = append(backends, b)
	}
	if len(nsConf) > 0 {
		b := backend.Func("lockbox-"+piperNamespace, func(_ context.Context, key string) ([]byte, error) {
			if v, ok := nsConf[key]; ok {
				return []byte(v), nil
			}
			return nil, backend.ErrNotFound
		})
		backends = append(backends, b)
	}

	if len(backends) > 0 {
		c.lockboxLoader = loader.New(backends...)
	}
	return nil
}

type lockboxYAMLBackend struct {
	data []byte
}

func (b *lockboxYAMLBackend) Unmarshal(_ context.Context, to interface{}) error {
	return yaml.NewDecoder(bytes.NewReader(b.data)).Decode(to)
}

func (b *lockboxYAMLBackend) Get(context.Context, string) ([]byte, error) {
	return nil, errors.New("not implemented")
}

func (b *lockboxYAMLBackend) Name() string {
	return "lockbox-yaml"
}
