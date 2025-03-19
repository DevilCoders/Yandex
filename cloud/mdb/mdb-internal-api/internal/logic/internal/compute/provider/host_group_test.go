package provider

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	computeMock "a.yandex-team.ru/cloud/mdb/internal/compute/compute/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanMock "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	authMock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

func TestValidateHostGroupsSuccess(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup := compute.HostGroup{ID: "hg1", FolderID: "folder1"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{})
	require.NoError(t, err)
}

func TestValidateHostGroupsNotFound(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup := compute.HostGroup{ID: "hg1", FolderID: "folder1"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil)
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg2").Return(compute.HostGroup{}, semerr.NotFound(""))

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1", "hg2"}, "folder1", "cloud1", []string{})
	semErr := semerr.AsSemanticError(err)
	require.Error(t, semErr)
	require.Equal(t, semerr.SemanticFailedPrecondition, semErr.Semantic)
	require.Equal(t, "host group \"hg2\" not found", semErr.Message)
}

func TestValidateHostGroupsInPorto(t *testing.T) {
	ctrl := gomock.NewController(t)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, nil, rmClient, hostTypeClient)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{})
	require.Error(t, err)
	require.Equal(t, "this installation does not support host groups", err.Error())
}

func TestValidateHostGroupsDistinctFolder(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup := compute.HostGroup{ID: "hg1", FolderID: "folder2"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup, nil)
	rmClient.EXPECT().ResolveFolders(gomock.Any(), []string{"folder1"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud1", FolderExtID: "folder1"}}, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{})
	require.NoError(t, err)
}

func TestValidateHostGroupsDistinctCloud(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup := compute.HostGroup{ID: "hg1", FolderID: "folder2"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup, nil)
	rmClient.EXPECT().ResolveFolders(gomock.Any(), []string{"folder1"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud2", FolderExtID: "folder1"}}, nil)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{})
	semErr := semerr.AsSemanticError(err)
	require.Error(t, semErr)
	require.Equal(t, semerr.SemanticFailedPrecondition, semErr.Semantic)
	require.Equal(t, "host group hg1 belongs to the cloud distinct from the cluster's cloud", semErr.Message)
}

func TestValidateHostGroupsNoPermissionToUse(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup := compute.HostGroup{ID: "hg1", FolderID: "folder1"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, semerr.Authorization("not authorized"))

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{})
	semErr := semerr.AsSemanticError(err)
	require.Error(t, semErr)
	require.Equal(t, semerr.SemanticAuthorization, semErr.Semantic)
	require.Equal(t, "permission to use host group hg1 is denied", semErr.Message)
}

func TestValidateHostGroupsWithinAllRequiredZones(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup1 := compute.HostGroup{ID: "hg1", FolderID: "folder1", ZoneID: "myt"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup1, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil).Times(1)

	hostGroup2 := compute.HostGroup{ID: "hg2", FolderID: "folder1", ZoneID: "sas"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg2").Return(hostGroup2, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg2", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1", "hg2"}, "folder1", "cloud1", []string{"myt", "sas"})
	require.NoError(t, err)
}

func TestValidateHostGroupsSomeZonesAreMissing(t *testing.T) {
	ctrl := gomock.NewController(t)
	hostGroupClient := computeMock.NewMockHostGroupService(ctrl)
	rmClient := resmanMock.NewMockClient(ctrl)
	auth := authMock.NewMockAuthenticator(ctrl)
	hostTypeClient := computeMock.NewMockHostTypeService(ctrl)
	cl := NewCompute(nil, nil, auth, hostGroupClient, rmClient, hostTypeClient)

	hostGroup1 := compute.HostGroup{ID: "hg1", FolderID: "folder1", ZoneID: "myt"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hostGroup1, nil)
	auth.EXPECT().Authenticate(gomock.Any(), models.PermComputeHostGroupsUse, cloudauth.Resource{ID: "hg1", Type: models.ResourceTypeHostGroup}).
		Return(accessservice.Subject{}, nil)

	err := cl.ValidateHostGroups(context.Background(), []string{"hg1"}, "folder1", "cloud1", []string{"myt", "sas"})
	semErr := semerr.AsSemanticError(err)
	require.Error(t, semErr)
	require.Equal(t, semerr.SemanticInvalidInput, semErr.Semantic)
	require.Contains(t, semErr.Message, "The provided list of host groups does not contain host groups in the following availability zones: [sas]")
}
