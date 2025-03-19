package tests_test

import (
	"strconv"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func TestNetworkConnections(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	type nc struct {
		networkConnectionID string
		networkID           string
		provider            models.Provider
		desc                string
		params              *aws.NetworkConnectionParams
	}
	type input struct {
		ncs []*nc
	}

	tcs := []struct {
		name  string
		input input
	}{
		{
			"simple",
			input{
				ncs: []*nc{
					{
						provider: models.ProviderAWS,
						desc:     "desc1",
						params: &aws.NetworkConnectionParams{
							Type:      aws.NetworkConnectionPeering,
							AccountID: "acc1",
							VpcID:     "vpc1",
							IPv4:      "127.0.0.1/32",
							IPv6:      "2a02:6b8::/96",
							Region:    "reg2",
						},
					},
					{
						provider: models.ProviderAWS,
						desc:     "desc2",
						params: &aws.NetworkConnectionParams{
							Type:      aws.NetworkConnectionPeering,
							AccountID: "acc2",
							VpcID:     "vpc2",
							IPv4:      "172.17.0.0/16",
							IPv6:      "::/128",
							Region:    "reg4",
						},
					},
				},
			},
		},
	}

	var (
		projectID = "prj1"
		region    = "reg1"
		peeringID = "peering1"
	)

	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			// empty at start
			ncs, err := db.NetworkConnectionsByProjectID(ctx, projectID)
			require.NoError(t, err)
			require.Empty(t, ncs)

			txCtx, err := db.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			defer func() { _ = db.Rollback(txCtx) }()

			// create new connections
			for i, nc := range tc.input.ncs {
				require.Empty(t, nc.networkConnectionID, "misconfiguration: networkConnectionID must be empty")
				require.Empty(t, nc.networkID, "misconfiguration: networkID must be empty")

				netID, err := db.CreateNetwork(txCtx, projectID, nc.provider, region, strconv.Itoa(i), "", "10.0.0.0/16", nil)
				require.NoError(t, err)
				nc.networkID = netID

				ncID, err := db.CreateNetworkConnection(txCtx, nc.networkID, projectID, nc.provider, region, nc.desc, nc.params)
				require.NoError(t, err)
				nc.networkConnectionID = ncID
			}

			// list connections
			require.NoError(t, db.Commit(txCtx))
			ncs, err = db.NetworkConnectionsByProjectID(ctx, projectID)
			require.NoError(t, err)
			require.NotEmpty(t, ncs)
			require.Len(t, ncs, len(tc.input.ncs))

			// get all connections one by one
			createdNCs := make([]models.NetworkConnection, len(tc.input.ncs))
			for i, nc := range tc.input.ncs {
				require.NotEmpty(t, nc.networkConnectionID)
				dbNC, err := db.NetworkConnectionByID(ctx, nc.networkConnectionID)
				require.NoError(t, err)
				createdNCs[i] = dbNC

				require.Equal(t, nc.networkID, dbNC.NetworkID)
				require.Equal(t, projectID, dbNC.ProjectID)
				require.Equal(t, nc.provider, dbNC.Provider)
				require.Equal(t, region, dbNC.Region)
				require.Equal(t, nc.desc, dbNC.Description)
				require.Equal(t, nc.params, dbNC.Params)
				require.Equal(t, models.NetworkConnectionStatusCreating, dbNC.Status)

				verifyNetworkConnectionParams(t, nc.provider, nc.params, dbNC.Params)
			}

			// verify list and get return the same
			require.ElementsMatch(t, ncs, createdNCs)

			// change statuses
			for _, nc := range tc.input.ncs {
				require.Empty(t, nc.params.PeeringConnectionID)
				nc.params.PeeringConnectionID = peeringID
				require.NoError(t, db.MarkNetworkConnectionPending(ctx, nc.networkConnectionID, peeringID, nc.params))

				dbNC, err := db.NetworkConnectionByID(ctx, nc.networkConnectionID)
				require.NoError(t, err)
				require.Equal(t, models.NetworkConnectionStatusPending, dbNC.Status)
				require.Equal(t, peeringID, dbNC.StatusReason)
				require.Equal(t, peeringID, dbNC.Params.(*aws.NetworkConnectionParams).PeeringConnectionID)

				require.NoError(t, db.FinishNetworkConnectionCreating(ctx, nc.networkConnectionID))
				dbNC, err = db.NetworkConnectionByID(ctx, nc.networkConnectionID)
				require.NoError(t, err)
				require.Equal(t, models.NetworkConnectionStatusActive, dbNC.Status)

				txCtx, err := db.Begin(ctx, sqlutil.Primary)
				require.NoError(t, err)
				require.NoError(t, db.MarkNetworkConnectionDeleting(txCtx, nc.networkConnectionID, "delete"))
				require.NoError(t, db.Commit(txCtx))

				dbNC, err = db.NetworkConnectionByID(ctx, nc.networkConnectionID)
				require.NoError(t, err)
				require.Equal(t, models.NetworkConnectionStatusDeleting, dbNC.Status)

				require.NoError(t, db.DeleteNetworkConnection(ctx, nc.networkConnectionID))
			}

			ncs, err = db.NetworkConnectionsByProjectID(ctx, projectID)
			require.NoError(t, err)
			require.Empty(t, ncs)
		})
	}
}

func verifyNetworkConnectionParams(t *testing.T, provider models.Provider, params1, params2 models.NetworkConnectionParams) {
	switch provider {
	case models.ProviderAWS:
		awsParams1, ok := params1.(*aws.NetworkConnectionParams)
		require.True(t, ok)
		awsParams2, ok := params2.(*aws.NetworkConnectionParams)
		require.True(t, ok)
		require.Equal(t, awsParams1, awsParams2)
	}
}
