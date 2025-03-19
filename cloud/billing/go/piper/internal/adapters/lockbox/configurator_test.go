package lockbox

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	lbx "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/lockbox"
)

//go:generate mockery --name LockboxClient --disable-version-string --inpackage --testonly

type configuratorTestSuite struct {
	suite.Suite

	conf Configurator
	mock MockLockboxClient
}

func TestConfigurator(t *testing.T) {
	suite.Run(t, new(configuratorTestSuite))
}

func (suite *configuratorTestSuite) SetupTest() {
	suite.mock = MockLockboxClient{}
	suite.conf = *NewConfigurator(context.TODO(), &suite.mock, "secret-id")
}

func (suite *configuratorTestSuite) TestKeys() {
	secret := map[string]lbx.SecretValue{
		"ns.key":  {Text: "value"},
		"ns.file": {Data: []byte("file")},
		"yaml":    {Text: "{k:v}"},
	}
	suite.mock.On("GetSecret", mock.Anything, "secret-id").Return(secret, nil)

	result, err := suite.conf.GetConfig(context.TODO(), "ns")
	suite.Require().NoError(err)
	suite.Len(result, 1)
	suite.Equal("value", result["key"])

	suite.mock.AssertExpectations(suite.T())
}

func (suite *configuratorTestSuite) TestYaml() {
	secret := map[string]lbx.SecretValue{
		"ns.key":  {Text: "value"},
		"ns.file": {Data: []byte("file")},
		"yaml":    {Data: []byte("{k:v}")},
	}
	suite.mock.On("GetSecret", mock.Anything, "secret-id").Return(secret, nil)

	data, err := suite.conf.GetYaml(context.TODO())
	suite.Require().NoError(err)
	suite.Equal("{k:v}", string(data))
}

func (suite *configuratorTestSuite) TestKeysNotFound() {
	suite.mock.On("GetSecret", mock.Anything, "secret-id").Once().Return(nil, lbx.ErrNotFound)

	_, err := suite.conf.GetConfig(context.TODO(), "ns")
	suite.Require().ErrorIs(err, lbx.ErrNotFound)
}

func (suite *configuratorTestSuite) TestYamlNotFound() {
	suite.mock.On("GetSecret", mock.Anything, "secret-id").Once().Return(nil, lbx.ErrNotFound)

	_, err := suite.conf.GetYaml(context.TODO())
	suite.Require().ErrorIs(err, lbx.ErrNotFound)
}
