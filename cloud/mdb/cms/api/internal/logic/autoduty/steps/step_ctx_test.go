package steps_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestInstructionCtxHandlesRDs(t *testing.T) {
	t.Run("sets actual rds", func(t *testing.T) {
		insCtx := steps.NewEmptyInstructionCtx()

		require.Nil(t, insCtx.GetActualRD())
		rd1 := &types.RequestDecisionTuple{
			R: models.ManagementRequest{
				Name: "1",
			},
		}
		rd2 := &types.RequestDecisionTuple{
			R: models.ManagementRequest{
				Name: "2",
			},
		}

		require.Equal(t, 0, len(insCtx.DoneRDs()))
		require.Equal(t, 0, insCtx.TouchedCount())
		insCtx.SetActualRD(rd1)
		require.Equal(t, rd1, insCtx.GetActualRD())
		require.Equal(t, 1, insCtx.TouchedCount())
		require.Equal(t, 0, len(insCtx.DoneRDs()))
		insCtx.SetActualRD(rd2)
		require.Equal(t, rd2, insCtx.GetActualRD())
		require.Equal(t, 2, insCtx.TouchedCount())
		require.Equal(t, 1, len(insCtx.DoneRDs()))
		require.Equal(t, *rd1, insCtx.DoneRDs()[0])
	})

	t.Run("actual can be changed and then retrieved with changes", func(t *testing.T) {
		insCtx := steps.NewEmptyInstructionCtx()
		rd1 := &types.RequestDecisionTuple{
			R: models.ManagementRequest{
				Status: models.StatusInProcess,
			},
		}
		rd2 := &types.RequestDecisionTuple{}
		insCtx.SetActualRD(rd1)

		rd := insCtx.GetActualRD()
		rd.R.Status = models.StatusOK

		insCtx.SetActualRD(rd2)
		done := insCtx.DoneRDs()[0]
		require.Equal(t, models.StatusOK, done.R.Status)
	})
}
