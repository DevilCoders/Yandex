package operations

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestCreateAction(t *testing.T) {
	suite.Run(t, new(CreateActionTestSuite))
}

type CreateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *CreateActionTestSuite) TestCreatingOk() {
	actionResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{Description: "test"})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
	suite.Require().NotEmpty(actionResult.ID)
}
