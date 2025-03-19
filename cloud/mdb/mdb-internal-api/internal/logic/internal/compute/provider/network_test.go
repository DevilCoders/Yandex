package provider

import (
	"context"
	"errors"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	vpcMock "a.yandex-team.ru/cloud/mdb/internal/compute/vpc/mocks"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	networkMock "a.yandex-team.ru/cloud/mdb/internal/network/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	authMock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestGetNetworkFound(t *testing.T) {
	network := networkProvider.Network{
		ID:       "networkID",
		FolderID: "folderID",
		Name:     "network",
	}
	ctrl := gomock.NewController(t)
	vpcClient := vpcMock.NewMockClient(ctrl)

	networkClient := networkMock.NewMockClient(ctrl)
	networkClient.EXPECT().GetNetwork(gomock.Any(), network.ID).Return(network, nil)

	auth := authMock.NewMockAuthenticator(ctrl)
	cl := NewCompute(vpcClient, networkClient, auth, nil, nil, nil)
	res, err := cl.Network(context.Background(), network.ID)

	require.NoError(t, err)
	require.Equal(t, res.ID, network.ID)
}

func TestGetNetworkNotFound(t *testing.T) {
	network := networkProvider.Network{
		ID:       "networkID",
		FolderID: "folderID",
		Name:     "network",
	}
	ctrl := gomock.NewController(t)
	vpcClient := vpcMock.NewMockClient(ctrl)

	networkClient := networkMock.NewMockClient(ctrl)
	networkClient.EXPECT().GetNetwork(gomock.Any(), network.ID).Return(networkProvider.Network{}, semerr.NotFound("network not found"))

	cl := NewCompute(vpcClient, networkClient, authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.Network(context.Background(), network.ID)

	require.Error(t, err)
	require.True(t, semerr.IsFailedPrecondition(err))
}

func TestGetNetworkTransparentSemantic(t *testing.T) {
	ctrl := gomock.NewController(t)
	vpcClient := vpcMock.NewMockClient(ctrl)

	networkClient := networkMock.NewMockClient(ctrl)
	networkClient.EXPECT().GetNetwork(gomock.Any(), "badNetwork").Return(networkProvider.Network{}, semerr.InvalidInput("bad network id"))

	cl := NewCompute(vpcClient, networkClient, authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.Network(context.Background(), "badNetwork")

	require.Error(t, err)
	require.True(t, semerr.IsInvalidInput(err))
}

func TestGetNetworkFailUnexpectedly(t *testing.T) {
	network := networkProvider.Network{
		ID:       "networkID",
		FolderID: "folderID",
		Name:     "network",
	}
	ctrl := gomock.NewController(t)
	vpcClient := vpcMock.NewMockClient(ctrl)

	networkClient := networkMock.NewMockClient(ctrl)
	networkClient.EXPECT().GetNetwork(gomock.Any(), network.ID).Return(networkProvider.Network{}, xerrors.Errorf("something sad happens"))

	cl := NewCompute(vpcClient, networkClient, authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.Network(context.Background(), network.ID)

	require.Error(t, err)
	require.Nil(t, semerr.AsSemanticError(err), "err shouldn't be a semantic")
}

func TestPickSubnetComputeFound(t *testing.T) {
	vtype := environment.VTypeCompute
	geo := "sas"
	subnets := []networkProvider.Subnet{
		{ID: "sn1", ZoneID: "sas", FolderID: "folder1"},
		{ID: "sn2", ZoneID: "myt", FolderID: "folder2"},
	}

	ctrl := gomock.NewController(t)
	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermVPCSubnetsUse, cloudauth.Resource{
		ID:   "folder1",
		Type: cloudauth.ResourceTypeFolder,
	}).Return(as.Subject{}, nil)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), auth, nil, nil, nil)
	sn, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.String{}, "")

	require.NoError(t, err)
	require.Equal(t, sn.ID, "sn1")

	// with specified subnetID
	auth.EXPECT().Authenticate(gomock.Any(), gomock.Any(), gomock.Any()).Return(as.Subject{}, nil)
	sn, err = cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.NewString("sn1"), "")

	require.NoError(t, err)
	require.Equal(t, sn.ID, "sn1")
}

func TestPickSubnetComputeNotUnique(t *testing.T) {
	vtype := environment.VTypeCompute
	geo := "myt"
	subnets := []networkProvider.Subnet{
		{ID: "sn1", ZoneID: "sas"},
		{ID: "sn2", ZoneID: "myt"},
		{ID: "sn3", ZoneID: "myt"},
	}

	ctrl := gomock.NewController(t)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.String{}, "")

	require.Error(t, err)
}

func TestPickSubnetComputeNoPermission(t *testing.T) {
	vtype := environment.VTypeCompute
	geo := "myt"
	subnets := []networkProvider.Subnet{
		{ID: "sn1", ZoneID: "sas"},
		{ID: "sn2", ZoneID: "myt"},
	}

	ctrl := gomock.NewController(t)

	auth := authMock.NewMockAuthenticator(ctrl)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermVPCSubnetsUse, gomock.Any()).Return(as.Subject{}, errors.New("not authorized"))

	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), auth, nil, nil, nil)
	_, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.String{}, "")

	require.Error(t, err)

	auth.EXPECT().Authenticate(gomock.Any(), models.PermVPCSubnetsUse, gomock.Any()).Return(as.Subject{}, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermVPCAddressesCreateExternal, gomock.Any()).Return(as.Subject{}, errors.New("not authorized"))
	_, err = cl.PickSubnet(context.Background(), subnets, vtype, geo, true, optional.String{}, "")

	require.Error(t, err)
}

func TestPickSubnetComputeGeoNotFound(t *testing.T) {
	vtype := environment.VTypeCompute
	geo := "myt"
	subnets := []networkProvider.Subnet{
		{ID: "sn1", ZoneID: "sas"},
	}

	ctrl := gomock.NewController(t)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.String{}, "")

	require.Error(t, err)
	require.True(t, semerr.IsFailedPrecondition(err))
}

func TestPickSubnetComputeIDNotFound(t *testing.T) {
	vtype := environment.VTypeCompute
	geo := "myt"
	subnets := []networkProvider.Subnet{
		{ID: "sn1", ZoneID: "sas"},
		{ID: "sn2", ZoneID: "myt"},
		{ID: "sn3", ZoneID: "myt"},
	}

	ctrl := gomock.NewController(t)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.NewString("sn4"), "")

	require.Error(t, err)
	require.True(t, semerr.IsFailedPrecondition(err))
}

func TestPickSubnetPorto(t *testing.T) {
	vtype := environment.VTypePorto
	geo := "myt"
	var subnets []networkProvider.Subnet

	ctrl := gomock.NewController(t)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	sn, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.String{}, "")

	require.NoError(t, err)
	require.Equal(t, networkProvider.Subnet{}, sn)
}

func TestPickSubnetPortoIDNotFound(t *testing.T) {
	vtype := environment.VTypePorto
	geo := "myt"
	var subnets []networkProvider.Subnet

	ctrl := gomock.NewController(t)
	cl := NewCompute(vpcMock.NewMockClient(ctrl), networkMock.NewMockClient(ctrl), authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
	_, err := cl.PickSubnet(context.Background(), subnets, vtype, geo, false, optional.NewString("sn1"), "")

	require.Error(t, err)
	require.True(t, semerr.IsFailedPrecondition(err))
}

func TestValidateSecurityGroups(t *testing.T) {
	cases := []struct {
		name   string
		handle func(t *testing.T, vm *vpcMock.MockClient, cl *Compute)
	}{
		{
			"empty SGs are valid",
			func(t *testing.T, vm *vpcMock.MockClient, cl *Compute) {
				require.NoError(t, cl.ValidateSecurityGroups(context.Background(), nil, "net1"))
			},
		},
		{
			"one valid SG",
			func(t *testing.T, vm *vpcMock.MockClient, cl *Compute) {
				vm.EXPECT().GetSecurityGroup(gomock.Any(), "sg1").Return(vpc.SecurityGroup{NetworkID: "net1"}, nil)
				require.NoError(t, cl.ValidateSecurityGroups(context.Background(), []string{"sg1"}, "net1"))
			},
		},
		{
			"more that 3 SGs",
			func(t *testing.T, vm *vpcMock.MockClient, cl *Compute) {
				err := cl.ValidateSecurityGroups(context.Background(), []string{"sg1", "sg2", "sg3", "sg4"}, "net1")
				require.Error(t, err)
				require.True(t, semerr.IsInvalidInput(err))
			},
		},
		{
			"non existed SG",
			func(t *testing.T, vm *vpcMock.MockClient, cl *Compute) {
				vm.EXPECT().GetSecurityGroup(gomock.Any(), "sg1").Return(vpc.SecurityGroup{}, semerr.NotFound("it not exists"))
				err := cl.ValidateSecurityGroups(context.Background(), []string{"sg1"}, "net1")
				require.Error(t, err)
				require.True(t, semerr.IsFailedPrecondition(err))
			},
		},
		{
			"SG from different network",
			func(t *testing.T, vm *vpcMock.MockClient, cl *Compute) {
				vm.EXPECT().GetSecurityGroup(gomock.Any(), "sg1").Return(vpc.SecurityGroup{NetworkID: "net2"}, nil)
				err := cl.ValidateSecurityGroups(context.Background(), []string{"sg1"}, "net1")
				require.Error(t, err)
				require.True(t, semerr.IsFailedPrecondition(err))
			},
		},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			vm := vpcMock.NewMockClient(ctrl)
			nm := networkMock.NewMockClient(ctrl)
			cl := NewCompute(vm, nm, authMock.NewMockAuthenticator(ctrl), nil, nil, nil)
			defer ctrl.Finish()
			c.handle(t, vm, cl)
		})
	}
}
