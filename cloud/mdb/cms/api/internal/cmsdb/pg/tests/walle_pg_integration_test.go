package tests_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	swmodels "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication/tvm"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/authorization"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/walle"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

func TestCreateWalleRequest(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	w := walle.NewWalleInteractor(l, backend, nil)

	req1ID := "req1"
	req2ID := "req2"
	fqdn := "fqdn1"
	user := tvm.NewResult(authorization.WalleID)

	_, err := w.CreateRequest(
		ctx,
		user,
		"reboot",
		req1ID,
		"test1",
		"test",
		"automated",
		nil,
		[]string{fqdn},
		"cpu_failure",
		models.ScenarioInfo{
			ID:   42,
			Type: swmodels.ManagementRequestScenarioInfoScenarioTypeItdcDashMaintenance,
		},
		false,
	)
	require.NoError(t, err)

	// move request to OK by autoduty //
	txCtx, err := backend.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	defer func() { _ = backend.Rollback(txCtx) }()

	reqs, err := backend.GetRequestsByTaskID(txCtx, []string{req1ID})
	require.NoError(t, err)
	require.Equal(t, reqs[req1ID].FailureType, "cpu_failure")
	require.Equal(t, swmodels.ManagementRequestScenarioInfoScenarioTypeItdcDashMaintenance, reqs[req1ID].ScenarioInfo.Type)
	decs, err := backend.GetDecisionsByRequestID(txCtx, []int64{reqs[req1ID].ID})
	require.NoError(t, err)

	decs[0].Status = models.DecisionApprove
	err = backend.MarkRequestsResolvedByAutoDuty(txCtx, decs)
	require.NoError(t, err)

	err = backend.MoveDecisionsToStatus(txCtx, []int64{decs[0].ID}, models.DecisionApprove)
	require.NoError(t, err)

	err = backend.Commit(txCtx)
	require.NoError(t, err)
	// move request to OK by autoduty //

	// walle creates new task for same host //
	err = w.DeleteRequest(ctx, user, req1ID)
	require.NoError(t, err)

	_, err = w.CreateRequest(
		ctx,
		user,
		"profile",
		req2ID,
		"test1",
		"test",
		"automated",
		nil,
		[]string{fqdn},
		"",
		models.ScenarioInfo{},
		false,
	)
	require.NoError(t, err)
	// walle creates new task for same host //

	// check old decision was moved to cleanup //
	txCtx, err = backend.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	defer func() { _ = backend.Rollback(txCtx) }()

	newDecs, err := backend.GetDecisionsByID(txCtx, []int64{decs[0].ID})
	require.NoError(t, err)
	require.Equal(t, models.DecisionCleanup, newDecs[decs[0].ID].Status)
}
