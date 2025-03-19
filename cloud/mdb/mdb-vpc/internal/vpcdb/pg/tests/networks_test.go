package tests_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func TestNetwork(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	tcs := []struct {
		tcName            string
		vpcID             string
		ipv4              string
		projectID         string
		provider          models.Provider
		region            string
		name              string
		description       string
		externalResources models.ExternalResources
		statusReason      string
	}{
		{
			tcName:      "simple",
			vpcID:       "vpcID1",
			ipv4:        "0.0.0.0/8",
			projectID:   "p1",
			provider:    models.ProviderAWS,
			region:      "eu",
			name:        "test net",
			description: "desc",
			externalResources: &aws.NetworkExternalResources{
				VpcID: "vpcID1",
			},
			statusReason: "opID1",
		},
	}
	for _, tc := range tcs {
		t.Run(tc.tcName, func(t *testing.T) {
			txCtx, err := db.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			networkID, err := db.CreateNetwork(txCtx, tc.projectID, tc.provider, tc.region, tc.name, tc.description, tc.ipv4, nil)
			require.NoError(t, err)
			require.NoError(t, db.Commit(txCtx))
			net, err := db.NetworkByID(ctx, networkID)
			require.NoError(t, err)
			require.Equal(t, models.NetworkStatusCreating, net.Status)
			require.Equal(t, &aws.NetworkExternalResources{}, net.ExternalResources)

			err = db.FinishNetworkCreating(ctx, networkID, "::1", tc.externalResources)
			require.NoError(t, err)

			net, err = db.NetworkByID(ctx, networkID)
			require.NoError(t, err)
			require.Equal(t, tc.ipv4, net.IPv4)
			require.Equal(t, tc.projectID, net.ProjectID)
			require.Equal(t, tc.provider, net.Provider)
			require.Equal(t, tc.region, net.Region)
			require.Equal(t, tc.name, net.Name)
			require.Equal(t, tc.description, net.Description)
			require.Equal(t, models.NetworkStatusActive, net.Status)
			require.Equal(t, tc.externalResources, net.ExternalResources)

			txCtx, err = db.Begin(ctx, sqlutil.Primary)
			require.NoError(t, err)
			err = db.MarkNetworkDeleting(txCtx, networkID, tc.statusReason)
			require.NoError(t, err)
			require.NoError(t, db.Commit(txCtx))

			net, err = db.NetworkByID(ctx, networkID)
			require.NoError(t, err)
			require.Equal(t, tc.projectID, net.ProjectID)
			require.Equal(t, tc.name, net.Name)
			require.Equal(t, models.NetworkStatusDeleting, net.Status)
			require.Equal(t, tc.statusReason, net.StatusReason)

			err = db.DeleteNetwork(ctx, networkID)
			require.NoError(t, err)

			err = db.DeleteNetwork(ctx, networkID)
			require.True(t, semerr.IsNotFound(err))

			_, err = db.NetworkByID(ctx, networkID)
			require.True(t, semerr.IsNotFound(err))
		})
	}
}

func TestNetworkList(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	cidr := "10.0.0.0/8"

	res, err := db.NetworksByProjectID(ctx, "asd")
	require.NoError(t, err)
	require.Len(t, res, 0)

	txCtx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	_, err = db.CreateNetwork(txCtx, "asd", models.ProviderAWS, "", "n1", "", cidr, nil)
	require.NoError(t, err)
	_, err = db.CreateNetwork(txCtx, "asd", models.ProviderAWS, "", "n2", "", cidr, nil)
	require.NoError(t, err)
	_, err = db.CreateNetwork(txCtx, "qwe", models.ProviderAWS, "", "n1", "", cidr, nil)
	require.NoError(t, err)
	require.NoError(t, db.Commit(txCtx))

	txCtx, err = db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	_, err = db.CreateNetwork(txCtx, "asd", models.ProviderAWS, "qwe", "n1", "", cidr, nil)
	require.Error(t, err)
	require.NoError(t, db.Rollback(txCtx))

	res, err = db.NetworksByProjectID(ctx, "asd")
	require.NoError(t, err)
	require.Len(t, res, 2)

	res, err = db.NetworksByProjectID(ctx, "qwe")
	require.NoError(t, err)
	require.Len(t, res, 1)
}

func TestCreateDuplicatedNetworkName(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	txCtx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	defer func() { require.NoError(t, db.Rollback(txCtx)) }()

	_, err = db.CreateNetwork(txCtx, "p1", "p1", "r1", "n1", "", "10.0.0.0/8", nil)
	require.NoError(t, err)

	_, err = db.CreateNetwork(txCtx, "p2", "p1", "r1", "n1", "", "10.0.0.0/8", nil)
	require.NoError(t, err)

	_, err = db.CreateNetwork(txCtx, "p1", "p2", "r2", "n1", "", "10.42.0.0/16", nil)
	require.Error(t, err)
	require.True(t, semerr.IsFailedPrecondition(err))
}

func TestImportedNetworks(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	netsToCreate := []*models.Network{
		{
			ProjectID: "p1",
			Provider:  models.ProviderAWS,
			Region:    "r1",
			Name:      "net1",
			IPv4:      "0.0.0.0/0",
			IPv6:      "::/0",
			ExternalResources: &aws.NetworkExternalResources{
				IamRoleArn: optional.NewString("role"),
			},
		},
		{
			ProjectID:         "p1",
			Provider:          models.ProviderAWS,
			Region:            "r1",
			Name:              "net2",
			IPv4:              "0.0.0.0/0",
			IPv6:              "::/0",
			ExternalResources: &aws.NetworkExternalResources{},
		},
		{
			ProjectID: "p1",
			Provider:  models.ProviderAWS,
			Region:    "r2",
			Name:      "net3",
			IPv4:      "0.0.0.0/0",
			IPv6:      "::/0",
			ExternalResources: &aws.NetworkExternalResources{
				IamRoleArn: optional.NewString("role"),
			},
		},
		{
			ProjectID: "p2",
			Provider:  models.ProviderAWS,
			Region:    "r1",
			Name:      "net4",
			IPv4:      "0.0.0.0/0",
			IPv6:      "::/0",
			ExternalResources: &aws.NetworkExternalResources{
				IamRoleArn: optional.NewString("role"),
			},
		},
		{
			ProjectID: "p2",
			Provider:  models.ProviderAWS,
			Region:    "r1",
			Name:      "net5",
			IPv4:      "0.0.0.0/0",
			IPv6:      "::/0",
			ExternalResources: &aws.NetworkExternalResources{
				IamRoleArn: optional.NewString("role"),
			},
		},
		{
			ProjectID: "p2",
			Provider:  models.ProviderAWS,
			Region:    "r1",
			Name:      "net6",
			IPv4:      "0.0.0.0/0",
			IPv6:      "::/0",
			ExternalResources: &aws.NetworkExternalResources{
				IamRoleArn: optional.NewString("role"),
			},
		},
	}
	for _, net := range netsToCreate {
		txCtx, err := db.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		netID, err := db.CreateNetwork(txCtx, net.ProjectID, net.Provider, net.Region, net.Name, "", net.IPv4, nil)
		require.NoError(t, err)
		net.ID = netID

		require.NoError(t, db.Commit(txCtx))

		require.NoError(t, db.FinishNetworkCreating(ctx, netID, net.IPv6, net.ExternalResources))
	}

	txCtx, err := db.Begin(ctx, sqlutil.Primary)
	require.NoError(t, err)
	require.NoError(t, db.MarkNetworkDeleting(txCtx, netsToCreate[5].ID, ""))
	require.NoError(t, db.Commit(txCtx))

	importedNets, err := db.ImportedNetworks(ctx, "p1", "r1", models.ProviderAWS)
	require.NoError(t, err)
	require.Len(t, importedNets, 1)
	require.Equal(t, netsToCreate[0].ID, importedNets[0].ID)

	importedNets, err = db.ImportedNetworks(ctx, "p2", "r1", models.ProviderAWS)
	require.NoError(t, err)
	require.Len(t, importedNets, 2)
	require.ElementsMatch(t, []string{netsToCreate[3].ID, netsToCreate[4].ID}, []string{importedNets[0].ID, importedNets[1].ID})
}

func TestAwsVpcUniqueness(t *testing.T) {
	ctx, db, l := initPG(t)
	defer func() { require.NoError(t, db.Close()) }()
	defer tearDown(t, db)
	waitForBackend(ctx, t, db, l)

	project := "p"
	region := "r"
	v4 := "0.0.0.0/0"
	vpc1 := "vpc1"
	vpc2 := "vpc2"
	name1 := "n1"
	name2 := "n2"

	t.Run("empty er", func(t *testing.T) {
		txCtx, err := db.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, db.Rollback(txCtx)) }()
		netID, err := db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name1, "", v4,
			nil,
		)
		require.NoError(t, err)
		net, err := db.NetworkByID(txCtx, netID)
		require.NoError(t, err)
		require.Equal(t, "", net.ExternalResources.(*aws.NetworkExternalResources).VpcID)

		_, err = db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name2, "", v4,
			nil,
		)
		require.NoError(t, err)
	})

	t.Run("different VPC", func(t *testing.T) {
		txCtx, err := db.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, db.Rollback(txCtx)) }()
		netID, err := db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name1, "", v4,
			&aws.NetworkExternalResources{VpcID: vpc1},
		)
		require.NoError(t, err)
		net, err := db.NetworkByID(txCtx, netID)
		require.NoError(t, err)
		require.Equal(t, vpc1, net.ExternalResources.(*aws.NetworkExternalResources).VpcID)

		_, err = db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name2, "", v4,
			&aws.NetworkExternalResources{VpcID: vpc2},
		)
		require.NoError(t, err)
	})

	t.Run("same VPC error", func(t *testing.T) {
		txCtx, err := db.Begin(ctx, sqlutil.Primary)
		require.NoError(t, err)
		defer func() { require.NoError(t, db.Rollback(txCtx)) }()
		netID, err := db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name1, "", v4,
			&aws.NetworkExternalResources{VpcID: vpc1},
		)
		require.NoError(t, err)
		net, err := db.NetworkByID(txCtx, netID)
		require.NoError(t, err)
		require.Equal(t, vpc1, net.ExternalResources.(*aws.NetworkExternalResources).VpcID)

		_, err = db.CreateNetwork(txCtx, project, models.ProviderAWS, region, name2, "", v4,
			&aws.NetworkExternalResources{VpcID: vpc1},
		)
		require.Error(t, err)
		require.True(t, semerr.IsFailedPrecondition(err))
	})
}
