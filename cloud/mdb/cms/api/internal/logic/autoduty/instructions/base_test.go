package instructions_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	swaggerModels "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	cmsModels "a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestStrategyName(t *testing.T) {
	i := instructions.Instructions{}
	t.Run("name based", func(t *testing.T) {
		r := cmsModels.ManagementRequest{
			Name: swaggerModels.ManagementRequestActionTemporaryDashUnreachable,
		}
		require.Equal(t, swaggerModels.ManagementRequestActionTemporaryDashUnreachable, i.StrategyName(r))
	})
}
