package tests_test

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/go-openapi/loads"
	"github.com/gofrs/uuid"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/restapi"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication/tvm"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	db_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/authorization"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/webservice"
	mdb_cms_client "a.yandex-team.ru/cloud/mdb/cms/api/pkg/cmsclient"
	cmsModels "a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

func waitForBackend(ctx context.Context, t *testing.T, backend cmsdb.Client, l log.Logger) {
	require.NoError(t, ready.Wait(ctx, backend, &ready.DefaultErrorTester{Name: "cms database", L: l}, time.Second))
}

func initPG(t *testing.T) (context.Context, cmsdb.Client, log.Logger) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

	cfg := pgConfig(t)
	backend, err := pg.New(cfg, logger)
	require.NoError(t, err)
	require.NotNil(t, backend)

	require.NoError(t, err)
	return ctx, backend, logger
}

func pgConfig(t *testing.T) pgutil.Config {
	cfg := pg.DefaultConfig()
	require.NotNil(t, cfg)

	cfg.Addrs = []string{dbteststeps.DBHostPort("CMSDB")}
	return cfg
}

func tearDown(_ *testing.T, client cmsdb.Client) {
	c := client.(*pg.Backend)
	tables := []string{
		"requests",
		"decisions",
		"instance_operations",
	}
	for _, table := range tables {
		var query = fmt.Sprintf("DELETE FROM cms.%v;", table)
		_, err := c.GetDB().Primary().DB().Exec(query)
		if err != nil {
			panic(err)
		}
	}

}

func TestMarkDeletedNothingNoError(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	deleted, err := backend.MarkRequestsDeletedByTaskID(ctx, []string{"20"})
	require.NoError(t, err)
	require.Empty(t, deleted)
}

func TestMarkDeleted(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	tID := "0000-i-am-task-id"
	reqs := []cmsdb.RequestToCreate{{
		Name:        "redeploy",
		ExtID:       tID,
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
	}}
	_, _ = backend.CreateRequests(
		ctx,
		reqs)

	deleted, err := backend.MarkRequestsDeletedByTaskID(ctx, []string{tID, "and-another-nonexistent"})
	require.NoError(t, err)
	require.Len(t, deleted, 1)
	require.Equal(t, deleted[0], tID)
}

func allTasksInCMSWithDeleted(ctx context.Context, backend cmsdb.Client, l log.Logger) ([]string, error) {
	c := backend.(*pg.Backend)

	var versions []string
	parser := func(rows *sqlx.Rows) error {
		var version string
		if err := rows.Scan(&version); err != nil {
			return err
		}
		versions = append(versions, version)
		return nil
	}
	_, err := sqlutil.QueryContext(
		ctx,
		c.GetDB().PrimaryChooser(),
		sqlutil.Stmt{
			Name:  "SelectAllRequests",
			Query: "SELECT request_ext_id FROM cms.requests",
		},
		map[string]interface{}{},
		parser,
		l,
	)

	return versions, err

}

func TestCantReadDeletedRequests(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	tID := "0000-i-am-task-id"
	reqs := []cmsdb.RequestToCreate{{
		Name:        "redeploy",
		ExtID:       tID,
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
	}}
	_, _ = backend.CreateRequests(
		ctx,
		reqs)

	_, _ = backend.MarkRequestsDeletedByTaskID(ctx, []string{tID})

	requests, err := backend.GetRequests(ctx)
	require.NoError(t, err)
	require.Len(t, requests, 0)

	taskIDs, err := allTasksInCMSWithDeleted(ctx, backend, l)
	require.NoError(t, err)
	require.NotEmpty(t, taskIDs)
	require.Len(t, taskIDs, 1)
}

func TestCreateRequest(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	reqs := []cmsdb.RequestToCreate{{
		Name:        "redeploy",
		ExtID:       "0000-i-am-task-id",
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
	}, {
		Name:        "deactivate",
		ExtID:       "1000-i-am-task-id-2",
		Comment:     "fake to test",
		Author:      "chapson",
		RequestType: "manual",
		Fqnds:       []string{"google.com"},
	}}
	status, err := backend.CreateRequests(
		ctx,
		reqs)
	require.NoError(t, err)
	require.Equal(t, cmsModels.StatusInProcess, status)

	requests, err := backend.GetRequests(ctx)
	require.NoError(t, err)
	require.Len(t, requests, 2)

}

func TestGetRequests(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	requests, err := backend.GetRequests(ctx)
	require.NoError(t, err)
	require.Empty(t, requests)

	req := cmsdb.RequestToCreate{
		Name:        "redeploy",
		ExtID:       "0000-i-am-task-id",
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
	}
	_, _ = backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{req})

	requests, err = backend.GetRequests(ctx)
	require.NoError(t, err)
	require.Len(t, requests, 1)
	require.Equal(t, requests[0].ExtID, req.ExtID)
	require.Equal(t, requests[0].Fqnds, req.Fqnds)
	require.NotEqual(t, int64(0), requests[0].ID, "request.ID should be non zero value")
}

func TestIdempotentCreationOfDeletedRequestMakesItVisible(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	createMe := cmsdb.RequestToCreate{
		Name:        "redeploy",
		ExtID:       "0000-i-am-task-id",
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
	}
	_, _ = backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{createMe})
	_, _ = backend.MarkRequestsDeletedByTaskID(ctx, []string{createMe.ExtID})
	requests, _ := backend.GetRequests(ctx)
	require.Empty(t, requests)
	_, err := backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{createMe})
	require.NoError(t, err)
	requests, err = backend.GetRequests(ctx)
	require.NoError(t, err)
	require.NotEmpty(t, requests)
	require.Equal(t, createMe.ExtID, requests[0].ExtID)
	require.Equal(t, cmsModels.PersonUnknown, requests[0].AnalysedBy)
	require.Equal(t, time.Time{}, requests[0].ResolvedAt)
	require.True(t, requests[0].CameBackAt.IsZero())
}

func TestGetRequestsByTaskID(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	requests, err := backend.GetRequestsByTaskID(ctx, []string{})

	require.NoError(t, err)
	require.Empty(t, requests)

	req := cmsdb.RequestToCreate{
		Name:        "redeploy",
		ExtID:       "0000-i-am-task-id",
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
		Extra:       "hello",
	}
	_, _ = backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{req})

	requests, err = backend.GetRequestsByTaskID(ctx, []string{req.ExtID, "no-such-task-id"})
	require.NoError(t, err)
	require.Len(t, requests, 1)
	require.Contains(t, requests, req.ExtID)
	require.Equal(t, req.Fqnds, requests[req.ExtID].Fqnds)
	require.Equal(t, req.Extra, requests[req.ExtID].Extra)
	require.NotEqual(t, int64(0), requests[req.ExtID].ID, "request.ID should be non zero value")
}

func TestCountRequestsToConsider(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	consReqs, err := backend.GetRequestsToConsider(ctx, 0)
	require.NoError(t, err)
	require.Len(t, consReqs, 0)

	req := cmsdb.RequestToCreate{
		Name:        "redeploy",
		ExtID:       "0000-i-am-task-id",
		Comment:     "fake to test",
		Author:      "wall-e",
		RequestType: "automated",
		Fqnds:       []string{"yandex.ru", "yandex.com"},
		Extra:       "hello",
	}
	_, _ = backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{req})

	consReqs, err = backend.GetRequestsToConsider(ctx, 0)
	require.NoError(t, err)
	require.Len(t, consReqs, 1)
}

var anyReqCnt int

// non idempotent, can be called multiple times
func anyRequest(ctx context.Context, backend cmsdb.Client, t *testing.T) cmsModels.ManagementRequest {
	requestExtID := fmt.Sprintf("0%d-i-am-task-id", anyReqCnt)
	anyReqCnt++
	_, err := backend.CreateRequests(
		ctx,
		[]cmsdb.RequestToCreate{
			{
				Name:        "redeploy",
				ExtID:       requestExtID,
				Comment:     "fake to test",
				Author:      "wall-e",
				RequestType: "automated",
				Fqnds:       []string{"yandex.ru", "yandex.com"},
				Extra:       "hello",
			},
		})
	require.NoError(t, err)
	allReqs, err := backend.GetRequestsByTaskID(ctx, []string{requestExtID})
	require.NoError(t, err)
	require.Contains(t, allReqs, requestExtID)
	return allReqs[requestExtID]
}

type DecToProcessTestCase struct {
	In    cmsModels.DecisionStatus
	Error bool
}

func TestGetDecisionsToProcess(t *testing.T) {
	testCases := []DecToProcessTestCase{
		{
			In:    cmsModels.DecisionReject,
			Error: true,
		},
		{
			In:    cmsModels.DecisionApprove,
			Error: true,
		}, {
			In: cmsModels.DecisionEscalate,
		}, {
			In: cmsModels.DecisionWait,
		}, {
			In: cmsModels.DecisionNone,
		}, {
			In:    cmsModels.DecisionDone,
			Error: true,
		}, {
			In:    cmsModels.DecisionBeforeDone,
			Error: true,
		}, {
			In: cmsModels.DecisionProcessing,
		}}
	for _, tc := range testCases {
		t.Run(fmt.Sprintf("decision %q results in reqs for analysis", tc.In), func(t *testing.T) {
			ctx, backend, l := initPG(t)
			defer func() { require.NoError(t, backend.Close()) }()
			defer tearDown(t, backend)
			waitForBackend(ctx, t, backend, l)

			req := anyRequest(ctx, backend, t)
			_, err := backend.CreateDecision(ctx, req.ID, tc.In, "for tests")
			require.NoError(t, err)
			ctx, err = backend.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			defer func() { require.NoError(t, backend.Rollback(ctx)) }()
			_, err = backend.GetDecisionsToProcess(ctx, nil)
			if tc.Error {
				require.True(t, semerr.IsNotFound(err))
			} else {
				require.NoError(t, err)
			}
		})
	}
}

func TestGetDecisionsToFinish(t *testing.T) {
	t.Run("can get decisions to finish from db", func(t *testing.T) {
		ctx, backend, l := initPG(t)
		defer func() { require.NoError(t, backend.Close()) }()
		defer tearDown(t, backend)
		waitForBackend(ctx, t, backend, l)

		ctx, err := backend.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, backend.Rollback(ctx)) }()

		req := anyRequest(ctx, backend, t)
		_, err = backend.CreateDecision(ctx, req.ID, cmsModels.DecisionAtWalle, "for tests")
		require.NoError(t, err)
		_, err = backend.GetDecisionsToFinishAfterWalle(ctx, nil)
		require.True(t, semerr.IsNotFound(err))

		req = anyRequest(ctx, backend, t)
		_, err = backend.CreateDecision(ctx, req.ID, cmsModels.DecisionBeforeDone, "for tests")
		require.NoError(t, err)
		decision, err := backend.GetDecisionsToFinishAfterWalle(ctx, nil)
		require.NoError(t, err)
		require.Equal(t, cmsModels.DecisionBeforeDone, decision.Status)
		require.NoError(t, backend.Commit(ctx))
	})
}

func TestGetDecisionsToCleanup(t *testing.T) {
	t.Run("can get decisions to cleanup from db", func(t *testing.T) {
		ctx, backend, l := initPG(t)
		defer func() { require.NoError(t, backend.Close()) }()
		defer tearDown(t, backend)
		waitForBackend(ctx, t, backend, l)

		ctx, err := backend.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, backend.Rollback(ctx)) }()

		req := anyRequest(ctx, backend, t)
		_, err = backend.CreateDecision(ctx, req.ID, cmsModels.DecisionAtWalle, "for tests")
		require.NoError(t, err)
		_, err = backend.GetDecisionsToCleanup(ctx, nil)
		require.True(t, semerr.IsNotFound(err))

		req = anyRequest(ctx, backend, t)
		_, err = backend.CreateDecision(ctx, req.ID, cmsModels.DecisionCleanup, "for tests")
		require.NoError(t, err)
		decision, err := backend.GetDecisionsToCleanup(ctx, nil)
		require.NoError(t, err)
		require.Equal(t, cmsModels.DecisionCleanup, decision.Status)
		require.NoError(t, backend.Commit(ctx))
	})
}

func TestMoveDecisionsToStatus(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)
	for status := range cmsModels.KnownDecisionMap {
		for newStatus := range cmsModels.KnownDecisionMap {
			req := anyRequest(ctx, backend, t)
			ctx, err := backend.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			dID, err := backend.CreateDecision(ctx, req.ID, status, "to be processed")
			require.NoError(t, err)
			err = backend.MoveDecisionsToStatus(ctx, []int64{dID}, newStatus)
			require.NoError(t, err)
			require.NoError(t, backend.Commit(ctx))
		}
	}
}

func TestSetAutodutyResolution(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)
	for status := range cmsModels.KnownDecisionMap {
		req := anyRequest(ctx, backend, t)
		ctx, err := backend.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		dID, err := backend.CreateDecision(ctx, req.ID, status, "to be processed")
		require.NoError(t, err)
		require.NoError(t, backend.Commit(ctx))

		for res := range cmsModels.KnownResolutionMap {
			ctx, err = backend.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			err = backend.SetAutoDutyResolution(ctx, []int64{dID}, res)
			require.NoError(t, err)
			require.NoError(t, backend.Rollback(ctx))
		}
	}
}

func TestAddDecision(t *testing.T) {
	for status := range cmsModels.KnownDecisionMap {
		t.Run(fmt.Sprintf("for %q decision", status), func(t *testing.T) {
			ctx, backend, l := initPG(t)
			defer func() { require.NoError(t, backend.Close()) }()
			defer tearDown(t, backend)
			waitForBackend(ctx, t, backend, l)

			req := anyRequest(ctx, backend, t)

			expl := ""
			if status != cmsModels.DecisionNone && status != cmsModels.DecisionProcessing {
				expl = "for tests purposes"
			}

			ID, err := backend.CreateDecision(ctx, req.ID, status, expl)
			require.NoError(t, err)
			decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
			require.NoError(t, err)
			require.Contains(t, decs, ID)
			require.Equal(t, decs[ID].Status, status)
			require.NotEqual(t, decs[ID].AnalysisLog, "")
			require.Len(t, decs[ID].OpsLog.Downtimes, 0)
			require.Len(t, decs[ID].OpsLog.PreRestart, 0)
			require.Len(t, decs[ID].OpsLog.PostRestartContainers, 0)
		})
	}
}

func TestUpdateDecisionFields(t *testing.T) {
	t.Run("text fields", func(t *testing.T) {
		ctx, backend, l := initPG(t)
		defer func() { require.NoError(t, backend.Close()) }()
		defer tearDown(t, backend)
		waitForBackend(ctx, t, backend, l)

		req := anyRequest(ctx, backend, t)
		ID, err := backend.CreateDecision(ctx, req.ID, cmsModels.DecisionApprove, "expl")
		require.NoError(t, err)
		decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
		require.NoError(t, err)
		require.Contains(t, decs, ID)
		d := decs[ID]

		d.AfterWalleLog = "chapson af"
		d.MutationsLog = "chapson mu"
		d.AnalysisLog = "chapson an"
		d.CleanupLog = "velom was here"

		ctx, err = backend.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, backend.Rollback(ctx)) }()
		err = backend.UpdateDecisionFields(ctx, d)
		require.NoError(t, err)

		decs, err = backend.GetDecisionsByID(ctx, []int64{ID})
		require.NoError(t, err)
		require.Contains(t, decs, ID)
		d = decs[ID]
		require.Equal(t, "chapson af", d.AfterWalleLog)
		require.Equal(t, "chapson mu", d.MutationsLog)
		require.Equal(t, "chapson an", d.AnalysisLog)
	})
}

func TestOpMetaLog(t *testing.T) {
	type TestCase struct {
		Name   string
		In     opmetas.OpMeta
		Getter func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta
	}
	tcs := []TestCase{
		{
			"dom0 state",
			&opmetas.Dom0StateMeta{SwitchPort: opmetas.SwitchPort{Switch: "switch", Port: "port"}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.Dom0State
			},
		},
		{
			"downtimes",
			&opmetas.SetDowntimesStepMeta{DowntimeIDs: []string{"1"}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.Downtimes.Latest()
			},
		},
		{
			"pre_restart",
			&opmetas.PreRestartStepMeta{Shipments: opmetas.ShipmentsInfo{"cont.test": []deployModels.ShipmentID{deployModels.ShipmentID(1)}}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.PreRestart.Latest()
			},
		},
		{
			"stop containers",
			&opmetas.StopContainersMeta{Shipments: opmetas.ShipmentsInfo{"cont.test": []deployModels.ShipmentID{deployModels.ShipmentID(1)}}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.StopContainers.Latest()
			},
		},
		{
			"post_restart",
			&opmetas.PostRestartMeta{Shipments: opmetas.ShipmentsInfo{"cont.test": []deployModels.ShipmentID{deployModels.ShipmentID(1)}}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.PostRestartContainers.Latest()
			},
		},
		{
			"bring_metadata",
			&opmetas.BringMetadataMeta{Shipments: opmetas.ShipmentsInfo{"cont.test": []deployModels.ShipmentID{deployModels.ShipmentID(1)}}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.BringMetadataOnFQNDs
			},
		},
		{
			"locks state",
			&opmetas.LocksStateMeta{Locks: map[string]*lockcluster.State{"fqdn1": {LockID: "lock1"}, "fqdn2": {LockID: "lock2"}}},
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.LocksState
			},
		},
		{
			"periodic state",
			opmetas.NewPeriodicStateMeta(time.Date(2022, 06, 01, 12, 00, 00, 00, time.UTC)),
			func(metaLog *cmsModels.OpsMetaLog) opmetas.OpMeta {
				return metaLog.PeriodicState
			},
		},
	}
	for _, tc := range tcs {
		t.Run("op meta log "+tc.Name, func(t *testing.T) {
			ctx, backend, l := initPG(t)
			defer func() { require.NoError(t, backend.Close()) }()
			defer tearDown(t, backend)
			waitForBackend(ctx, t, backend, l)

			req := anyRequest(ctx, backend, t)
			ID, err := backend.CreateDecision(ctx, req.ID, cmsModels.DecisionApprove, "expl")
			require.NoError(t, err)
			decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
			require.NoError(t, err)
			require.Contains(t, decs, ID)
			d := decs[ID]

			meta := tc.Getter(decs[ID].OpsLog)
			require.Nil(t, meta)

			err = d.OpsLog.Add(tc.In)
			require.NoError(t, err)
			ctx, err = backend.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			defer func() { require.NoError(t, backend.Rollback(ctx)) }()
			err = backend.UpdateDecisionFields(ctx, d)
			require.NoError(t, err)
			decs, err = backend.GetDecisionsByID(ctx, []int64{ID})
			require.NoError(t, err)
			require.Contains(t, decs, ID)
			d = decs[ID]

			meta = tc.Getter(decs[ID].OpsLog)
			require.NotNil(t, meta)
			require.Equal(t, tc.In, meta)
		})
	}
}

type MarkRequestsDoneTC struct {
	Ads cmsModels.DecisionStatus
	Rs  cmsModels.RequestStatus
}

func TestMarkRequestsDone(t *testing.T) {
	for _, tc := range []MarkRequestsDoneTC{
		{cmsModels.DecisionApprove, cmsModels.StatusOK},
	} {
		t.Run(fmt.Sprintf("mark done if %q decision", tc.Ads), func(t *testing.T) {
			ctx, backend, l := initPG(t)
			defer func() { require.NoError(t, backend.Close()) }()
			defer tearDown(t, backend)
			waitForBackend(ctx, t, backend, l)

			req := anyRequest(ctx, backend, t)

			expl := "some text"

			ID, err := backend.CreateDecision(ctx, req.ID, tc.Ads, expl)
			require.NoError(t, err)
			decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
			require.NoError(t, err)
			err = backend.MarkRequestsResolvedByAutoDuty(ctx, []cmsModels.AutomaticDecision{decs[ID]})
			require.NoError(t, err)
			reqs, err := backend.GetRequestsByID(ctx, []int64{req.ID})
			require.NoError(t, err)
			require.Contains(t, reqs, req.ID)
			req = reqs[req.ID]
			require.Equal(t, tc.Rs, req.Status)
			require.Equal(t, "robot-mdb-cms-porto", string(req.ResolvedBy))
		})
	}
}

func TestMarkRequestsFinished(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	req := anyRequest(ctx, backend, t)

	ID, err := backend.CreateDecision(ctx, req.ID, cmsModels.DecisionBeforeDone, "some text")
	require.NoError(t, err)
	decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
	require.NoError(t, err)
	err = backend.MarkRequestsFinishedByAutoDuty(ctx, []cmsModels.AutomaticDecision{decs[ID]})
	require.NoError(t, err)
	reqs, err := backend.GetRequestsByID(ctx, []int64{req.ID})
	require.NoError(t, err)
	require.Contains(t, reqs, req.ID)
	req = reqs[req.ID]
}

func TestMarkUpdateRequestFields(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	req := anyRequest(ctx, backend, t)

	req.ResolveExplanation = "TestMarkUpdateRequestFields"

	err := backend.UpdateRequestFields(ctx, req)
	require.NoError(t, err)
	reqs, err := backend.GetRequestsByID(ctx, []int64{req.ID})
	require.NoError(t, err)
	require.Contains(t, reqs, req.ID)
	req = reqs[req.ID]
	require.Equal(t, "TestMarkUpdateRequestFields", req.ResolveExplanation)
}

func TestMarkRequestsAnalysed(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	req := anyRequest(ctx, backend, t)

	expl := "some text"

	ID, err := backend.CreateDecision(ctx, req.ID, cmsModels.DecisionApprove, expl)
	require.NoError(t, err)
	decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
	require.NoError(t, err)
	err = backend.MarkRequestsAnalysedByAutoDuty(ctx, []cmsModels.AutomaticDecision{decs[ID]})
	require.NoError(t, err)
	reqs, err := backend.GetRequestsByID(ctx, []int64{req.ID})
	require.NoError(t, err)
	require.Contains(t, reqs, req.ID)
	req = reqs[req.ID]
	require.Equal(t, "robot-mdb-cms-porto", string(req.AnalysedBy))
}

func TestMarkRequestsCameBack(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	req := anyRequest(ctx, backend, t)

	expl := "some text"

	ID, err := backend.CreateDecision(ctx, req.ID, cmsModels.DecisionApprove, expl)
	require.NoError(t, err)
	decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
	require.NoError(t, err)
	err = backend.MarkRequestsCameBack(ctx, []cmsModels.AutomaticDecision{decs[ID]})
	require.NoError(t, err)
	reqs, err := backend.GetRequestsByID(ctx, []int64{req.ID})
	require.NoError(t, err)
	require.Len(t, reqs, 0)
}

func TestStatGetRequestsInWindow(t *testing.T) {
	// test

	t.Run("1 awaiting, 1 done then 1 out", func(t *testing.T) {
		ctx, backend, l := initPG(t)
		defer func() { require.NoError(t, backend.Close()) }()
		defer tearDown(t, backend)
		waitForBackend(ctx, t, backend, l)

		req1 := anyRequest(ctx, backend, t)
		ID, err := backend.CreateDecision(ctx, req1.ID, cmsModels.DecisionApprove, "some test expl")
		require.NoError(t, err)
		decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
		require.NoError(t, err)
		err = backend.MarkRequestsResolvedByAutoDuty(ctx, []cmsModels.AutomaticDecision{decs[ID]})
		require.NoError(t, err)
		anyRequest(ctx, backend, t)

		reqs, err := backend.GetRequestsStatInWindow(ctx, time.Minute*2)
		require.NoError(t, err)
		require.Len(t, reqs, 2)
	})

	t.Run("respect window 1 minute", func(t *testing.T) {
		ctx, backend, l := initPG(t)
		defer func() { require.NoError(t, backend.Close()) }()
		defer tearDown(t, backend)
		waitForBackend(ctx, t, backend, l)

		r := anyRequest(ctx, backend, t)
		reqs, err := backend.GetRequestsStatInWindow(ctx, time.Minute*2)
		require.NoError(t, err)
		require.Len(t, reqs, 1)
		query := sqlutil.Stmt{
			Name: "UpdateReq",
			// language=PostgreSQL
			Query: `UPDATE cms.requests SET created_at = :created_at WHERE id = :request_id`,
		}
		_, err = sqlutil.QueryContext(
			ctx,
			backend.(*pg.Backend).GetDB().PrimaryChooser(),
			query,
			map[string]interface{}{
				"request_id": r.ID,
				"created_at": time.Now().Add(time.Duration(-10) * time.Hour),
			},
			sqlutil.NopParser,
			l,
		)
		require.NoError(t, err)

		reqs, err = backend.GetRequestsStatInWindow(ctx, time.Minute)
		require.NoError(t, err)
		require.Len(t, reqs, 0)
	})
}

func TestGetNotFinishedDecisionsByFQDN(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	req1 := anyRequest(ctx, backend, t)
	ID, err := backend.CreateDecision(ctx, req1.ID, cmsModels.DecisionBeforeDone, "some test expl")
	require.NoError(t, err)
	decs, err := backend.GetDecisionsByID(ctx, []int64{ID})
	require.NoError(t, err)
	err = backend.MarkRequestsCameBack(ctx, []cmsModels.AutomaticDecision{decs[ID]})
	require.NoError(t, err)

	decIDs, err := backend.GetNotFinishedDecisionsByFQDN(ctx, []string{"yandex.ru", "yandex.com"})
	require.NoError(t, err)
	require.Equal(t, 1, len(decIDs))
	require.Equal(t, ID, decIDs[0].ID)
}

func TestStatistics(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	queryRequest := sqlutil.Stmt{
		Name: "InsertTestRequests",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.requests
    (name, request_ext_id, status, comment, author, request_type, fqdns, created_at, resolved_at, resolved_by, is_deleted, came_back_at)
VALUES
    ('profile', :request_ext_id, 'in-process', '', 'wall-e', 'automated', '{fqdn1}', :created_at, :resolved_at, :resolved_by, :is_deleted, :came_back_at)`,
	}
	queryDecision := sqlutil.Stmt{
		Name: "InsertTestDecision",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.decisions
    (request_id, status, explanation, ad_resolution, ops_metadata_log, after_walle_log)
VALUES
    ((SELECT MAX(id) from cms.requests WHERE request_ext_id = :req_ext_id), :status, '', 'approved', '{}', '')`,
	}

	type Items struct {
		requestExtID string
		status       models.TaskStatus
		createdAt    time.Time
		resolvedAt   time.Time
		cameBackAt   time.Time
		isDeleted    bool
		deciStatus   string
	}

	items := []*Items{
		{
			requestExtID: "0001--fresh--at-wall-e",
			deciStatus:   "at-wall-e",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-1 * time.Hour),
			isDeleted:    false,
		},
		{
			requestExtID: "0002--old--at-wall-e",
			deciStatus:   "at-wall-e",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-4 * time.Hour),
			isDeleted:    false,
		},
		{
			requestExtID: "0003--fresh--before-done",
			deciStatus:   "before-done",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-1 * time.Hour),
			isDeleted:    false,
		},
		{
			requestExtID: "0004--old--before-done",
			deciStatus:   "before-done",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-4 * time.Hour),
			isDeleted:    false,
		},
		{
			requestExtID: "0005--fresh--cleanup--deleted",
			deciStatus:   "cleanup",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-1 * time.Hour),
			isDeleted:    true,
			cameBackAt:   time.Now().Add(-30 * time.Minute),
		},
		{
			requestExtID: "0006--old--cleanup--deleted",
			deciStatus:   "cleanup",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-4 * time.Hour),
			isDeleted:    true,
			cameBackAt:   time.Now().Add(-3*time.Hour - 30*time.Minute),
		},
		{
			requestExtID: "0007--fresh--before-done--deleted",
			deciStatus:   "before-done",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-1 * time.Hour),
			isDeleted:    true,
			cameBackAt:   time.Now().Add(-30 * time.Minute),
		},
		{
			requestExtID: "0008--old--before-done--deleted",
			deciStatus:   "before-done",
			createdAt:    time.Now().Add(-24 * time.Hour),
			resolvedAt:   time.Now().Add(-4 * time.Hour),
			isDeleted:    true,
			cameBackAt:   time.Now().Add(-3*time.Hour - 30*time.Minute),
		},
	}
	for _, item := range items {
		_, err := sqlutil.QueryContext(
			ctx,
			backend.(*pg.Backend).GetDB().PrimaryChooser(),
			queryRequest,
			map[string]interface{}{
				"request_ext_id": item.requestExtID,
				"created_at":     item.createdAt,
				"resolved_at":    item.resolvedAt,
				"resolved_by":    "somebody",
				"is_deleted":     item.isDeleted,
				"came_back_at":   item.cameBackAt,
			},
			sqlutil.NopParser,
			l,
		)
		require.NoError(t, err)
		_, err = sqlutil.QueryContext(
			ctx,
			backend.(*pg.Backend).GetDB().PrimaryChooser(),
			queryDecision,
			map[string]interface{}{
				"req_ext_id": item.requestExtID,
				"status":     item.deciStatus,
			},
			sqlutil.NopParser,
			l,
		)
		require.NoError(t, err)
	}

	window := time.Hour * 3

	resetups, err := backend.GetResetupRequests(ctx, window)
	require.NoError(t, err)
	resetupEids := make([]string, len(resetups))
	for idx, item := range resetups {
		resetupEids[idx] = item.ExtID
	}
	expResetupIds := []string{
		"0002--old--at-wall-e",
		"0004--old--before-done",
	}
	require.Equal(t, expResetupIds, resetupEids)

	unfinisheds, err := backend.GetUnfinishedRequests(ctx, window)
	require.NoError(t, err)
	unfinishedEids := make([]string, len(unfinisheds))
	for idx, item := range unfinisheds {
		unfinishedEids[idx] = item.ExtID
	}
	expUnfinishedEids := []string{
		"0006--old--cleanup--deleted",
		"0008--old--before-done--deleted",
	}
	require.Equal(t, expUnfinishedEids, unfinishedEids)

}

// server tests

var server *restapi.Server

func startServer(t *testing.T) {
	if server != nil {
		return
	}
	// which port to listen
	err := os.Setenv("PORT", "20001")
	if err != nil {
		panic("should not happen")
	}

	testApp, db, l := webservice.CustomSwaggerApp(dbteststeps.DBHostPort("CMSDB"), tvm.NewMock(authorization.WalleID))
	defer func() { _ = db.Close() }()
	restapi.App = testApp
	waitForBackend(context.Background(), t, db, l)
	swaggerSpec, err := loads.Embedded(restapi.SwaggerJSON, restapi.FlatSwaggerJSON)
	if err != nil {
		l.Errorf("could not load %v", err)
		os.Exit(1)
	}

	api := operations.NewMdbCmsapiAPI(swaggerSpec)
	// get server with flag values filled out
	server = restapi.NewServer(api)
	server.ConfigureAPI()
	if err := server.Serve(); err != nil {
		l.Errorf("could not serve http %v", err)
		err = server.Shutdown()
		l.Errorf("could not shutdown %v", err)
		restapi.App.Shutdown()
	}
}

func waitForClient(ctx context.Context, t *testing.T, client mdb_cms_client.Client, l log.Logger) {
	awaitCtx, cancel := context.WithTimeout(ctx, 10*time.Second)
	defer cancel()

	require.NoError(t, ready.Wait(awaitCtx, client, &ready.DefaultErrorTester{Name: "mdb_cms_client_test", L: l}, time.Second))
}

func getClient(t *testing.T) mdb_cms_client.Client {
	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	c, err := mdb_cms_client.New(
		"http://localhost:20001",
		"my-secret-ticket",
		httputil.TLSConfig{},
		httputil.LoggingConfig{},
		l)
	waitForClient(context.Background(), t, c, l)
	if err != nil {
		panic(fmt.Sprintf("cannot make client: %v", err))
	}
	return c
}

func TestServerStartsAndExecutesDelete(t *testing.T) {
	go startServer(t)

	client := getClient(t)
	err := client.DeleteTask(context.Background(), "ID-doesnt-exist")
	require.Error(t, err)
	//panic(fmt.Sprintf("error '%v'", err))
	require.True(t, xerrors.Is(err, mdb_cms_client.ErrNotFound))
}

func TestServerStartsAndExecutesGet(t *testing.T) {
	client := getClient(t)
	tasks, err := client.GetTasks(context.Background())
	require.NoError(t, err)
	require.Empty(t, tasks.Result)
}

func TestLock(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	err := backend.GetLock(ctx, cmsdb.Dom0AutodutyLockKey)
	require.Error(t, err)

	newCtx, err := backend.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	defer func() {
		err = backend.Rollback(newCtx)
		require.NoError(t, err)
	}()

	err = backend.GetLock(newCtx, cmsdb.Dom0AutodutyLockKey)
	require.NoError(t, err)
}

func TestCreateInstanceOperation(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	comment := "testing"
	author := "tests"
	instanceID := "fqdn1"
	opType := cmsModels.InstanceOperationMove
	externalID := "req1"

	opID, err := backend.CreateInstanceOperation(ctx, externalID, opType, instanceID, comment, author)
	require.NoError(t, err)
	u, err := uuid.FromString(opID)
	require.NoError(t, err)
	require.Equal(t, byte(4), u.Version())

	op, err := backend.GetInstanceOperation(ctx, opID)
	require.NoError(t, err)
	require.Equal(t, comment, op.Comment)
	require.Equal(t, author, string(op.Author))
	require.Equal(t, instanceID, op.InstanceID)
	require.Equal(t, opType, op.Type)
	require.Equal(t, []string{}, op.ExecutedStepNames)
	require.Equal(t, cmsModels.InstanceOperationStatusNew, op.Status)

	newOpID, err := backend.CreateInstanceOperation(ctx, externalID, opType, instanceID, comment, author)
	require.NoError(t, err)
	require.Equal(t, opID, newOpID)
}

func TestGetUnexistedOperation(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	opID, err := uuid.NewV4()
	require.NoError(t, err)
	_, err = backend.GetInstanceOperation(ctx, opID.String())
	require.True(t, semerr.IsNotFound(err))
}

func TestGetNewOperations(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	txCtx, err := backend.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	newOps, err := backend.InstanceOperationsToProcess(txCtx, 42)
	require.NoError(t, err)
	require.Empty(t, newOps)

	err = backend.Commit(txCtx)
	require.NoError(t, err)

	ops := []*struct {
		comment    string
		author     string
		instanceID string
		opType     cmsModels.InstanceOperationType
		externalID string
		id         string
	}{
		{
			comment:    "one",
			author:     "tests",
			instanceID: "fqdn1",
			opType:     cmsModels.InstanceOperationMove,
			externalID: "req1",
		},
		{
			comment:    "two",
			author:     "tests",
			instanceID: "fqdn2",
			opType:     cmsModels.InstanceOperationWhipPrimaryAway,
			externalID: "req2",
		},
	}

	for _, op := range ops {
		opID, err := backend.CreateInstanceOperation(ctx, op.externalID, op.opType, op.instanceID, op.comment, op.author)
		require.NoError(t, err)
		op.id = opID
	}

	txCtx, err = backend.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	newOps, err = backend.InstanceOperationsToProcess(txCtx, 1)
	require.NoError(t, err)

	err = backend.Commit(txCtx)
	require.NoError(t, err)
	require.Len(t, newOps, 1)

	expectedOp := ops[0]
	actualOp := newOps[0]
	require.Equal(t, expectedOp.id, actualOp.ID)
	require.Equal(t, expectedOp.comment, actualOp.Comment)
	require.Equal(t, expectedOp.author, string(actualOp.Author))
	require.Equal(t, expectedOp.instanceID, actualOp.InstanceID)
	require.Equal(t, expectedOp.opType, actualOp.Type)
	require.Equal(t, cmsModels.InstanceOperationStatusNew, actualOp.Status)
}

func TestUpdateOperations(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	ops := []*struct {
		comment     string
		author      string
		instanceID  string
		opType      cmsModels.InstanceOperationType
		externalID  string
		id          string
		createdAt   time.Time
		modifiedAt  time.Time
		status      cmsModels.InstanceOperationStatus
		explanation string
		log         string
		state       *cmsModels.OperationState
	}{
		{
			comment:    "one",
			author:     "tests",
			instanceID: "fqdn1",
			opType:     cmsModels.InstanceOperationMove,
			externalID: "req1",
		},
		{
			comment:    "two",
			author:     "tests",
			instanceID: "fqdn2",
			opType:     cmsModels.InstanceOperationWhipPrimaryAway,
			externalID: "req2",
		},
	}

	for i, op := range ops {
		t.Run(fmt.Sprintf("update %d", i), func(t *testing.T) {
			opID, err := backend.CreateInstanceOperation(ctx, op.externalID, op.opType, op.instanceID, op.comment, op.author)
			require.NoError(t, err)

			opFromDB, err := backend.GetInstanceOperation(ctx, opID)
			require.NoError(t, err)
			require.Equal(t, []string{}, opFromDB.ExecutedStepNames)
			op.id = opID
			op.createdAt = opFromDB.CreatedAt
			op.modifiedAt = opFromDB.ModifiedAt
			op.status = opFromDB.Status
			op.log = opFromDB.Log
			op.explanation = opFromDB.Explanation
			op.state = cmsModels.DefaultOperationState()
			op.state.MoveInstanceStep.TaskID = opFromDB.State.MoveInstanceStep.TaskID

			cmsdbCtx, err := backend.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			defer func() {
				err = backend.Rollback(cmsdbCtx)
				require.NoError(t, err)
			}()

			opFromDB.ExecutedStepNames = []string{"check if primary"}
			opFromDB.State.MoveInstanceStep.TaskID = "okay"
			opFromDB.Explanation = "it's okay"
			opFromDB.Log = "1. one\n2. two"
			opFromDB.Status = cmsModels.InstanceOperationStatusInProgress
			err = backend.UpdateInstanceOperationFields(cmsdbCtx, opFromDB)
			require.NoError(t, err)

			err = backend.Commit(cmsdbCtx)
			require.NoError(t, err)

			newOpFromDB, err := backend.GetInstanceOperation(ctx, opID)
			require.NoError(t, err)
			require.Equal(t, []string{"check if primary"}, newOpFromDB.ExecutedStepNames)
			require.Equal(t, op.id, newOpFromDB.ID)
			require.Equal(t, op.opType, newOpFromDB.Type)
			require.Equal(t, op.comment, newOpFromDB.Comment)
			require.Equal(t, op.author, string(newOpFromDB.Author))
			require.Equal(t, op.instanceID, newOpFromDB.InstanceID)
			require.Equal(t, op.createdAt, newOpFromDB.CreatedAt)
			require.NotEqual(t, op.modifiedAt, newOpFromDB.ModifiedAt)
			require.NotEqual(t, op.explanation, newOpFromDB.Explanation)
			require.NotEqual(t, op.log, newOpFromDB.Log)
			require.NotEqual(t, op.state, newOpFromDB.State)
			require.Equal(t, "", op.state.MoveInstanceStep.TaskID)
			require.Equal(t, "okay", newOpFromDB.State.MoveInstanceStep.TaskID)
		})
	}
}

type Waiter struct {
	cluster *sqlutil.Cluster
}

func (w *Waiter) IsReady(_ context.Context) error {
	node := w.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("postgresql not available")
	}

	return nil
}

func TestStaleOperations(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	cfg := pgConfig(t)
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	require.NoError(t, err)

	waiter := &Waiter{cluster: cluster}
	require.NoError(t, ready.Wait(ctx, waiter, &ready.DefaultErrorTester{Name: "cms database for insert", L: l}, time.Second))

	staleOps, err := backend.StaleInstanceOperations(ctx)
	require.NoError(t, err)
	require.Empty(t, staleOps)

	state, err := db_models.OperationStateToDB(cmsModels.DefaultOperationState())
	require.NoError(t, err)

	query := sqlutil.Stmt{
		Name: "InsertTestInstanceOperation",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.instance_operations (operation_type, status, comment, author, instance_id, idempotency_key, operation_state, created_at, modified_at)
VALUES ('move', :status, :comment, 'test', :instance_id, :idempotency_key, :operation_state, :created_at, :modified_at)`,
	}

	ops := []*struct {
		instanceID string
		comment    string
		createdAt  time.Time
		modifiedAt time.Time
		status     cmsModels.InstanceOperationStatus
	}{
		{
			instanceID: "fqdn1",
			comment:    "too long at work",
			createdAt:  time.Now().Add(-25 * time.Hour),
			modifiedAt: time.Now(),
			status:     cmsModels.InstanceOperationStatusInProgress,
		},
		{
			instanceID: "fqdn2",
			comment:    "too long with no work",
			createdAt:  time.Now().Add(-1 * time.Hour),
			modifiedAt: time.Now().Add(-1 * time.Hour),
			status:     cmsModels.InstanceOperationStatusNew,
		},
		{
			instanceID: "fqdn3",
			comment:    "just new",
			createdAt:  time.Now(),
			modifiedAt: time.Now(),
			status:     cmsModels.InstanceOperationStatusNew,
		},
		{
			instanceID: "fqdn4",
			comment:    "finished",
			createdAt:  time.Now().Add(-1 * time.Hour),
			modifiedAt: time.Now().Add(-1 * time.Hour),
			status:     cmsModels.InstanceOperationStatusOK,
		},
	}

	for _, op := range ops {
		idempotencyKey, err := uuid.NewV4()
		require.NoError(t, err)

		_, err = sqlutil.QueryContext(
			ctx,
			cluster.PrimaryChooser(),
			query,
			map[string]interface{}{
				"instance_id":     op.instanceID,
				"comment":         op.comment,
				"status":          op.status,
				"idempotency_key": idempotencyKey,
				"operation_state": string(state),
				"created_at":      op.createdAt,
				"modified_at":     op.modifiedAt,
			},
			sqlutil.NopParser,
			l,
		)
		require.NoError(t, err)
	}

	staleOps, err = backend.StaleInstanceOperations(ctx)
	require.NoError(t, err)
	require.Equal(t, 2, len(staleOps))
	require.Equal(t, ops[0].comment, staleOps[0].Comment)
	require.Equal(t, ops[1].comment, staleOps[1].Comment)
}

func TestFilterOperations(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	cfg := pgConfig(t)
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	require.NoError(t, err)

	waiter := &Waiter{cluster: cluster}
	require.NoError(t, ready.Wait(ctx, waiter, &ready.DefaultErrorTester{Name: "cms database for insert", L: l}, time.Second))

	filteredOps, err := backend.StaleInstanceOperations(ctx)
	require.NoError(t, err)
	require.Empty(t, filteredOps)

	state, err := db_models.OperationStateToDB(cmsModels.DefaultOperationState())
	require.NoError(t, err)

	query := sqlutil.Stmt{
		Name: "InsertTestInstanceOperation",
		// language=PostgreSQL
		Query: `
INSERT INTO cms.instance_operations (operation_type, status, comment, author, instance_id, idempotency_key, operation_state, created_at, modified_at, executed_step_names)
VALUES ('move', :status, :comment, 'test', :instance_id, :idempotency_key, :operation_state, :created_at, :modified_at, :executed_step_names)`,
	}

	ops := []*struct {
		instanceID string
		comment    string
		createdAt  time.Time
		modifiedAt time.Time
		status     cmsModels.InstanceOperationStatus
		stepNames  []string
	}{
		{
			instanceID: "fqdn1",
			comment:    "in progress",
			status:     cmsModels.InstanceOperationStatusInProgress,
			stepNames:  []string{"init", "whip master"},
		},
		{
			instanceID: "fqdn2",
			comment:    "new",
			status:     cmsModels.InstanceOperationStatusNew,
			stepNames:  []string{},
		},
		{
			instanceID: "fqdn3",
			comment:    "finished",
			status:     cmsModels.InstanceOperationStatusOK,
			stepNames:  []string{"init"},
		},
	}

	for _, op := range ops {
		idempotencyKey, err := uuid.NewV4()
		require.NoError(t, err)

		var stepNames pgtype.TextArray
		if err = stepNames.Set(op.stepNames); err != nil {
			require.NoError(t, err)
		}

		_, err = sqlutil.QueryContext(
			ctx,
			cluster.PrimaryChooser(),
			query,
			map[string]interface{}{
				"instance_id":         op.instanceID,
				"comment":             op.comment,
				"status":              op.status,
				"idempotency_key":     idempotencyKey,
				"operation_state":     string(state),
				"created_at":          op.createdAt,
				"modified_at":         op.modifiedAt,
				"executed_step_names": stepNames,
			},
			sqlutil.NopParser,
			l,
		)
		require.NoError(t, err)
	}

	filteredOps, err = backend.ListInstanceOperations(ctx)
	require.NoError(t, err)
	require.Equal(t, 1, len(filteredOps))
	require.Equal(t, ops[0].stepNames, filteredOps[0].ExecutedStepNames)
	require.Equal(t, ops[0].comment, filteredOps[0].Comment)

}

// This test checks that there is only one record with non-ok operation status for a given instance ID
func TestOperationsWithSameInstanceID(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()
	defer tearDown(t, backend)
	waitForBackend(ctx, t, backend, l)

	comment := "testing"
	author := "tests"
	instanceID := "fqdn"

	tests := []struct {
		name           string
		opsType        cmsModels.InstanceOperationType
		idempotenceKey string
		err            func(error) bool
	}{
		{
			name:           "first record",
			opsType:        cmsModels.InstanceOperationMove,
			idempotenceKey: "test1",
		},
		{
			name:           "another operation",
			opsType:        cmsModels.InstanceOperationWhipPrimaryAway,
			idempotenceKey: "test2",
		},
		{
			name:           "the same operation",
			opsType:        cmsModels.InstanceOperationWhipPrimaryAway,
			idempotenceKey: "allal",
			err:            semerr.IsFailedPrecondition,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			_, err := backend.CreateInstanceOperation(ctx, tt.idempotenceKey, tt.opsType, instanceID, comment, author)
			if tt.err == nil {
				require.NoError(t, err)
				return
			}
			require.True(t, tt.err(err))
		})
	}
}
