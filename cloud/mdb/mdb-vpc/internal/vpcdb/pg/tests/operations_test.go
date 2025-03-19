package tests_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func TestCreateOperation(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	tc := struct {
		project     string
		description string
		createdBy   string
		params      models.OperationParams
		action      models.OperationAction
		provider    models.Provider
		region      string
	}{
		project:     "p1",
		description: "tests",
		createdBy:   "test",
		params: &models.CreateNetworkOperationParams{
			NetworkID: "n1",
		},
		action:   models.OperationActionCreateVPC,
		provider: models.ProviderAWS,
		region:   "eu",
	}

	//empty, than create
	ctx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	_, err = db.OperationToProcess(ctx)
	require.True(t, semerr.IsNotFound(err))

	opID, err := db.InsertOperation(ctx, tc.project, tc.description, tc.createdBy, tc.params, tc.action, tc.provider, tc.region)
	require.NoError(t, err)

	err = db.Commit(ctx)
	require.NoError(t, err)

	// stored one operation
	ctx, err = db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	op, err := db.OperationToProcess(ctx)
	require.NoError(t, err)
	require.Equal(t, opID, op.ID)
	require.Equal(t, tc.region, op.Region)
	require.Equal(t, tc.provider, op.Provider)
	require.Equal(t, tc.params, op.Params)

	err = db.Rollback(ctx)
	require.NoError(t, err)
}

func TestOperationToProcess(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	tcs := []struct {
		project     string
		description string
		createdBy   string
		params      models.OperationParams
		action      models.OperationAction
		provider    models.Provider
		region      string
	}{
		{
			project:     "p1",
			description: "tests",
			createdBy:   "test",
			params: &models.CreateNetworkOperationParams{
				NetworkID: "n1",
			},
			action:   models.OperationActionCreateVPC,
			provider: models.ProviderAWS,
			region:   "eu",
		},
		{
			project:     "p2",
			description: "tests",
			createdBy:   "test",
			params: &models.CreateNetworkOperationParams{
				NetworkID: "n2",
			},
			action:   models.OperationActionCreateVPC,
			provider: models.ProviderAWS,
			region:   "eu",
		},
	}

	// empty, than create N new operations
	ctx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	_, err = db.OperationToProcess(ctx)
	require.True(t, semerr.IsNotFound(err))

	opIDs := make([]string, len(tcs))
	for i, tc := range tcs {
		opID, err := db.InsertOperation(ctx, tc.project, tc.description, tc.createdBy, tc.params, tc.action, tc.provider, tc.region)
		require.NoError(t, err)
		opIDs[i] = opID
		time.Sleep(time.Second)
	}

	err = db.Commit(ctx)
	require.NoError(t, err)

	// every N OperationToProcess calls return different operations in order of creating
	opCtxs := make([]context.Context, len(tcs))
	for i, tc := range tcs {
		opCtx, err := db.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		opCtxs[i] = opCtx

		op, err := db.OperationToProcess(opCtx)
		require.NoError(t, err)

		require.Equal(t, opIDs[i], op.ID)
		require.Equal(t, tc.params, op.Params)
	}

	// N+1st OperationToProcess call returns nothing
	opCtx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)

	_, err = db.OperationToProcess(opCtx)
	require.True(t, semerr.IsNotFound(err))
	err = db.Rollback(opCtx)
	require.NoError(t, err)

	for _, opCtx := range opCtxs {
		err = db.Rollback(opCtx)
		require.NoError(t, err)
	}
}

func TestUpdateOperationFields(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	for _, tc := range []struct {
		action models.OperationAction
		state  models.OperationState
		status models.OperationStatus
		params models.OperationParams
	}{
		{
			action: models.OperationActionCreateVPC,
			status: models.OperationStatusRunning,
			state: &aws.CreateNetworkOperationState{
				Vpc:     &aws.Vpc{},
				Network: aws.NetworkState{},
			},
			params: models.DefaultCreateNetworkOperationParams(),
		},
		{
			action: models.OperationActionDeleteVPC,
			status: models.OperationStatusDone,
			state:  aws.DefaultDeleteNetworkOperationState(),
			params: models.DefaultDeleteNetworkOperationParams(),
		},
		{
			action: models.OperationActionCreateNetworkConnection,
			status: models.OperationStatusPending,
			state: &aws.CreateNetworkConnectionOperationState{
				NetworkConnectionParams: &aws.NetworkConnectionParams{},
				Network:                 aws.NetworkState{},
				NetworkConnection:       models.NetworkConnection{},
			},
			params: models.DefaultCreateNetworkConnectionOperationParams(),
		},
		{
			action: models.OperationActionDeleteNetworkConnection,
			status: models.OperationStatusRunning,
			state: &aws.DeleteNetworkConnectionOperationState{
				NetworkConnectionParams: &aws.NetworkConnectionParams{},
				Network:                 aws.NetworkState{},
				NetworkConnection:       models.NetworkConnection{},
			},
			params: models.DefaultDeleteNetworkConnectionOperationParams(),
		},
		{
			action: models.OperationActionImportVPC,
			status: models.OperationStatusRunning,
			state: &aws.ImportVPCOperationState{
				CreateNetworkOperationState: aws.CreateNetworkOperationState{
					Vpc:     &aws.Vpc{},
					Network: aws.NetworkState{},
				},
			},
			params: aws.DefaultImportVPCOperationParams(),
		},
	} {
		t.Run(string(tc.action), func(t *testing.T) {
			txCtx, err := db.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			defer func() { require.NoError(t, db.Rollback(txCtx)) }()

			opID, err := db.InsertOperation(
				txCtx, "p", "", "",
				tc.params,
				tc.action,
				models.ProviderAWS,
				"r",
			)
			require.NoError(t, err)

			op, err := db.OperationByID(txCtx, opID)
			require.NoError(t, err)
			require.Equal(t, opID, op.ID)

			op.State = tc.state
			op.Status = tc.status
			require.NoError(t, db.UpdateOperationFields(txCtx, op))

			op, err = db.OperationByID(txCtx, opID)
			require.NoError(t, err)
			require.Equal(t, tc.state, op.State)
			require.Equal(t, tc.params, op.Params)
		})
	}
}
