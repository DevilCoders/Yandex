package config

import (
	"context"
	"errors"
	"strings"

	"github.com/heetch/confita/backend"
	"github.com/stretchr/testify/suite"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/loader"
)

type baseTestSuite struct {
	suite.Suite

	container *Container
	ctxCancel context.CancelFunc

	yamlConfig *inmemoryYAMLBackend
}

func (suite *baseTestSuite) SetupTest() {
	suite.container = &Container{}
	suite.container.mainCtx, suite.ctxCancel = context.WithCancel(context.TODO())
	suite.yamlConfig = newInmemoryBackend("")
	suite.container.testLocalConfigLoader(loader.WithMergeLoad(suite.yamlConfig))
}

func (suite *baseTestSuite) TearDownTest() {
	suite.ctxCancel()
}

func (c *Container) testLocalConfigLoader(backend backend.Backend) {
	_ = c.configBackendsOnce.Do(func() error {
		c.localLoader = loader.New(backend)
		return nil
	})
}

func (c *Container) testDBConfigurator(conf func(namespace string) (map[string]string, error)) {
	_ = c.dbConfiguratorOnce.Do(func() error {
		c.dbConfigurator = configuratorFunc(conf)
		return nil
	})
}

func (c *Container) testLockboxConfigurator(conf testConfigurator) {
	_ = c.lockboxConfiguratorOnce.Do(func() error {
		c.lockboxConfigurator = conf
		return nil
	})
}

type inmemoryYAMLBackend struct {
	data string
	name string
}

func newInmemoryBackend(data string) *inmemoryYAMLBackend {
	return &inmemoryYAMLBackend{
		data: data,
	}
}

func (b *inmemoryYAMLBackend) SetData(data string) {
	b.data = data
}

func (b *inmemoryYAMLBackend) Unmarshal(_ context.Context, to interface{}) error {
	return yaml.NewDecoder(strings.NewReader(b.data)).Decode(to)
}

func (b *inmemoryYAMLBackend) Get(context.Context, string) ([]byte, error) {
	return nil, errors.New("not implemented")
}

func (b *inmemoryYAMLBackend) Name() string {
	return "yaml"
}

type configuratorFunc func(namespace string) (map[string]string, error)

func (f configuratorFunc) GetConfig(_ context.Context, namespace string) (map[string]string, error) {
	if f == nil {
		return nil, nil
	}
	return f(namespace)
}

type yamlGetFunc func() ([]byte, error)

func (f yamlGetFunc) GetYaml(context.Context) ([]byte, error) {
	if f == nil {
		return nil, nil
	}
	return f()
}

type testConfigurator struct {
	configuratorFunc
	yamlGetFunc
}
