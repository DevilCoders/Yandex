package integrationtests

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication/tvm"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/authorization"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/walle"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func TestProcessPendingRequests(t *testing.T) {
	user := tvm.NewResult(authorization.WalleID)

	type instr struct {
		analysis          *instructions.Instructions
		letGo             *instructions.Instructions
		toReturnFromWalle *instructions.Instructions
		afterWalle        *instructions.Instructions
		cleanup           *instructions.Instructions
	}
	type iteration struct {
		prepare  func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor)
		validate func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor)
	}
	tcs := []struct {
		name       string
		instr      instr
		iterations []iteration
	}{
		{
			name: "full requests workflow",
			instr: instr{
				analysis:          newEmptyInstructions("always allow", steps.AfterStepApprove, "it's safe to let go"),
				letGo:             newEmptyInstructions("letting dom0 go", steps.AfterStepAtWalle, "let go to Wall-e"),
				toReturnFromWalle: newEmptyInstructions("to return from walle", steps.AfterStepContinue, "prolonged"),
				afterWalle:        newEmptyInstructions("finishing", steps.AfterStepNext, "returned"),
				cleanup:           newEmptyInstructions("cleanup", steps.AfterStepNext, "clean finished"),
			},
			iterations: []iteration{
				{
					prepare: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"
						req2ID := "req2"
						fqdn1 := "fqdn1"
						fqdn2 := "fqdn2"

						_, err := w.CreateRequest(ctx, user, "reboot", req1ID, "test1", "test", "automated", nil, []string{fqdn1}, "", models.ScenarioInfo{}, false)
						require.NoError(t, err)

						_, err = w.CreateRequest(ctx, user, "reboot", req2ID, "test1", "test", "automated", nil, []string{fqdn2}, "", models.ScenarioInfo{}, false)
						require.NoError(t, err)
					},
					validate: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"
						req2ID := "req2"
						fqdn1 := "fqdn1"
						fqdn2 := "fqdn2"

						req, err := w.GetRequest(ctx, user, req1ID)
						require.NoError(t, err)
						require.Equal(t, fqdn1, req.Fqnds[0])
						require.Equal(t, models.StatusOK, req.Status)
						require.False(t, req.ResolvedAt.IsZero())
						require.True(t, req.CameBackAt.IsZero())

						dec, err := db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionAtWalle, dec[0].Status)

						req, err = w.GetRequest(ctx, user, req2ID)
						require.NoError(t, err)
						require.Equal(t, fqdn2, req.Fqnds[0])
						require.Equal(t, models.StatusOK, req.Status)
						require.False(t, req.ResolvedAt.IsZero())
						require.True(t, req.CameBackAt.IsZero())

						dec, err = db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionAtWalle, dec[0].Status)
					},
				},
				{
					prepare: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						// Wall-e took requests and still didn't remove it
					},
					validate: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						// test autoduty works with at-walle requests correctly
						req1ID := "req1"
						req2ID := "req2"

						req, err := w.GetRequest(ctx, user, req1ID)
						require.NoError(t, err)
						dec, err := db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionAtWalle, dec[0].Status)

						req, err = w.GetRequest(ctx, user, req2ID)
						require.NoError(t, err)
						dec, err = db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionAtWalle, dec[0].Status)
					},
				},
				{
					prepare: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"
						req2ID := "req2"

						require.NoError(t, w.DeleteRequest(ctx, user, req1ID))
						require.NoError(t, w.DeleteRequest(ctx, user, req2ID))
					},
					validate: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"
						req1IntID := int64(1)
						req2ID := "req2"
						req2IntID := int64(2)

						_, err := w.GetRequest(ctx, user, req1ID)
						require.True(t, semerr.IsNotFound(err))
						reqs, err := db.GetRequestsWithDeletedByID(ctx, []int64{req1IntID})
						require.NoError(t, err)
						require.Len(t, reqs, 1)
						req := reqs[req1IntID]
						require.Equal(t, req1ID, req.ExtID)
						require.False(t, req.CameBackAt.IsZero())
						dec, err := db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionDone, dec[0].Status)

						_, err = w.GetRequest(ctx, user, req2ID)
						require.True(t, semerr.IsNotFound(err))
						reqs, err = db.GetRequestsWithDeletedByID(ctx, []int64{req2IntID})
						require.NoError(t, err)
						require.Len(t, reqs, 1)
						req = reqs[req2IntID]
						require.Equal(t, req2ID, req.ExtID)
						require.False(t, req.CameBackAt.IsZero())
						dec, err = db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionDone, dec[0].Status)
					},
				},
			},
		},
		{
			name: "stack at analysis",
			instr: instr{
				analysis: newEmptyInstructions("never allow", steps.AfterStepWait, "it's still not allowed"),
			},
			iterations: []iteration{
				{
					prepare: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"
						fqdn1 := "fqdn1"
						_, err := w.CreateRequest(ctx, user, "reboot", req1ID, "test1", "test", "automated", nil, []string{fqdn1}, "", models.ScenarioInfo{}, false)
						require.NoError(t, err)
					},
					validate: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"

						req, err := w.GetRequest(ctx, user, req1ID)
						require.NoError(t, err)
						require.Equal(t, models.StatusInProcess, req.Status)
						dec, err := db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionWait, dec[0].Status)
					},
				},
				{
					prepare: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {},
					validate: func(ctx context.Context, t *testing.T, db cmsdb.Client, w walle.WalleInteractor) {
						req1ID := "req1"

						req, err := w.GetRequest(ctx, user, req1ID)
						require.NoError(t, err)
						require.Equal(t, models.StatusInProcess, req.Status)
						dec, err := db.GetDecisionsByRequestID(ctx, []int64{req.ID})
						require.NoError(t, err)
						require.Len(t, dec, 1)
						require.Equal(t, models.DecisionWait, dec[0].Status)
					},
				},
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx, db, l := initPG(t)
			waitForBackend(ctx, t, db, l)
			defer tearDown(t, db)
			ctx, cancel := context.WithTimeout(ctx, 30*time.Second)
			defer cancel()

			w := walle.NewWalleInteractor(l, db, nil)
			d := duty.NewCustomDuty(
				l,
				db,
				tc.instr.analysis,
				tc.instr.letGo,
				tc.instr.toReturnFromWalle,
				tc.instr.afterWalle,
				tc.instr.cleanup,
				false,
				10,
			)
			for i, iter := range tc.iterations {
				iter.prepare(ctxlog.WithFields(ctx, log.String("stage", "prepare"), log.Int("iteration number", i)), t, db, w)
				require.NoError(
					t,
					d.Run(
						ctxlog.WithFields(ctx, log.String("stage", "duty work"), log.Int("iteration number", i)),
					),
					fmt.Sprintf("failed duty.Run at iteration #%d", i),
				)
				iter.validate(ctxlog.WithFields(ctx, log.String("stage", "validate"), log.Int("iteration number", i)), t, db, w)
			}
		})
	}
}

func newEmptyInstructions(name string, action steps.AfterStepAction, explain string) *instructions.Instructions {
	return &instructions.Instructions{
		Default: &instructions.DecisionStrategy{
			EntryPoint: instructions.NewCommonWorkflow(
				name,
				[]steps.DecisionStep{
					steps.NewInParticularState(
						action,
						explain,
					),
				},
			),
		},
	}
}
