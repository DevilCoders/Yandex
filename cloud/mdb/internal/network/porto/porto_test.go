package porto

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamMocks "a.yandex-team.ru/cloud/mdb/internal/compute/iam/mocks"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	racktablesMocks "a.yandex-team.ru/cloud/mdb/internal/racktables/mocks"
)

func TestClient_GetNetworksByCloudID(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)
	mockABC := iamMocks.NewMockAbcService(ctrl)

	testClient := Client{
		rt:  mockRacktables,
		abc: mockABC,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacrosWithOwners(ctx).
		Return(racktables.MacrosWithOwners{
			"_DBAASEXTERNALNETS_": []racktables.Owner{
				{Type: "servicerole", Name: "svc_ycsecurity_administration"},
			},
			"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
				{Type: "user", Name: "velom"},
				{Type: "service", Name: "svc_internalmdb"},
				{Type: "servicerole", Name: "svc_internalmdb_administration"},
			},
			"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
				{Type: "service", Name: "svc_dataprocessing"},
				{Type: "service", Name: "svc_ycsecurity"},
			},
		}, nil)

	mockABC.EXPECT().ResolveByCloudID(ctx, "fooe9dt3lm4bvrup1las").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	mockRacktables.EXPECT().GetMacro(ctx, "_MDB_CONTROLPLANE_PORTO_TEST_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "524d", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "svc_internalmdb",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	mockABC.EXPECT().ResolveByABCSlug(ctx, "internalmdb").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	actual, err := testClient.GetNetworksByCloudID(ctx, "fooe9dt3lm4bvrup1las")
	require.NoError(t, err)

	expected := []string{"_PGAASINTERNALNETS_", "_MDB_CONTROLPLANE_PORTO_TEST_NETS_"}

	assert.Equal(t, expected, actual)
}

func TestClient_GetNetworksByCloudIDDoublePgaasInternalNets(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)
	mockABC := iamMocks.NewMockAbcService(ctrl)

	testClient := Client{
		rt:  mockRacktables,
		abc: mockABC,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacrosWithOwners(ctx).
		Return(racktables.MacrosWithOwners{
			"_DBAASEXTERNALNETS_": []racktables.Owner{
				{Type: "servicerole", Name: "svc_ycsecurity_administration"},
			},
			"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
				{Type: "user", Name: "velom"},
				{Type: "service", Name: "svc_internalmdb"},
				{Type: "servicerole", Name: "svc_internalmdb_administration"},
			},
			"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
				{Type: "service", Name: "svc_dataprocessing"},
				{Type: "service", Name: "svc_ycsecurity"},
			},
			"_PGAASINTERNALNETS_": []racktables.Owner{
				{Type: "service", Name: "svc_internalmdb"},
				{Type: "servicerole", Name: "svc_internalmdb_administration"},
			},
		}, nil)

	mockABC.EXPECT().ResolveByCloudID(ctx, "fooe9dt3lm4bvrup1las").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	mockRacktables.EXPECT().GetMacro(ctx, "_MDB_CONTROLPLANE_PORTO_TEST_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "524d", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "svc_internalmdb",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	mockABC.EXPECT().ResolveByABCSlug(ctx, "internalmdb").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	actual, err := testClient.GetNetworksByCloudID(ctx, "fooe9dt3lm4bvrup1las")
	require.NoError(t, err)

	expected := []string{"_PGAASINTERNALNETS_", "_MDB_CONTROLPLANE_PORTO_TEST_NETS_"}

	assert.Equal(t, expected, actual)
}

func TestClient_GetNetworksByCloudIDNoMatches(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)
	mockABC := iamMocks.NewMockAbcService(ctrl)

	testClient := Client{
		rt:  mockRacktables,
		abc: mockABC,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacrosWithOwners(ctx).
		Return(racktables.MacrosWithOwners{
			"_DBAASEXTERNALNETS_": []racktables.Owner{
				{Type: "servicerole", Name: "svc_ycsecurity_administration"},
			},
			"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
				{Type: "service", Name: "svc_dataprocessing"},
				{Type: "service", Name: "svc_ycsecurity"},
			},
		}, nil)

	mockABC.EXPECT().ResolveByCloudID(ctx, "fooe9dt3lm4bvrup1las").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	actual, err := testClient.GetNetworksByCloudID(ctx, "fooe9dt3lm4bvrup1las")
	require.NoError(t, err)

	expected := []string{"_PGAASINTERNALNETS_"}

	assert.Equal(t, expected, actual)
}

func TestClient_GetNetworksByCloudIDNoOwnerService(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)
	mockABC := iamMocks.NewMockAbcService(ctrl)

	testClient := Client{
		rt:  mockRacktables,
		abc: mockABC,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacrosWithOwners(ctx).
		Return(racktables.MacrosWithOwners{
			"_DBAASEXTERNALNETS_": []racktables.Owner{
				{Type: "servicerole", Name: "svc_ycsecurity_administration"},
			},
			"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
				{Type: "user", Name: "velom"},
				{Type: "service", Name: "svc_internalmdb"},
				{Type: "servicerole", Name: "svc_internalmdb_administration"},
			},
			"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
				{Type: "service", Name: "svc_dataprocessing"},
				{Type: "service", Name: "svc_ycsecurity"},
			},
		}, nil)

	mockABC.EXPECT().ResolveByCloudID(ctx, "fooe9dt3lm4bvrup1las").
		Return(iam.ABC{
			CloudID:         "fooe9dt3lm4bvrup1las",
			AbcSlug:         "internalmdb",
			AbcID:           0,
			DefaultFolderID: "fooi5vu9rdejqc3p4b60",
			AbcFolderID:     "",
		}, nil)

	mockRacktables.EXPECT().GetMacro(ctx, "_MDB_CONTROLPLANE_PORTO_TEST_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "524d", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	actual, err := testClient.GetNetworksByCloudID(ctx, "fooe9dt3lm4bvrup1las")
	require.NoError(t, err)

	expected := []string{"_PGAASINTERNALNETS_"}

	assert.Equal(t, expected, actual)
}

func TestClient_GetSubnets(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)

	testClient := Client{
		rt: mockRacktables,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacro(ctx, "_MDB_CONTROLPLANE_PORTO_TEST_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "524d", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "svc_internalmdb",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	network := networkProvider.Network{
		ID:          "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
		FolderID:    "fooi5vu9rdejqc3p4b60",
		Name:        "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
		Description: "",
	}

	actual, err := testClient.GetSubnets(ctx, network)
	require.NoError(t, err)

	expected := []networkProvider.Subnet{
		{
			ID:           "0:524d",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			ZoneID:       "sas",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:          "0:524d",
			FolderID:    "fooi5vu9rdejqc3p4b60",
			CreatedAt:   time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:        "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Description: "", Labels: map[string]string(nil),
			NetworkID: "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			ZoneID:    "vla", V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "0:524d",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			ZoneID:       "myt",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "0:524d",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			ZoneID:       "man",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "0:524d",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			ZoneID:       "iva",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
	}

	assert.Equal(t, expected, actual)
}

func TestClient_GetSubnetsCloudYandexNet(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)

	testClient := Client{
		rt: mockRacktables,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacro(ctx, "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "1007c", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "svc_internalmdb",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	network := networkProvider.Network{
		ID:          "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
		FolderID:    "fooi5vu9rdejqc3p4b60",
		Name:        "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
		Description: "",
	}

	actual, err := testClient.GetSubnets(ctx, network)
	require.NoError(t, err)

	expected := []networkProvider.Subnet{
		{
			ID:           "1:007c",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			ZoneID:       "sas",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:          "1:007c",
			FolderID:    "fooi5vu9rdejqc3p4b60",
			CreatedAt:   time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:        "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			Description: "", Labels: map[string]string(nil),
			NetworkID: "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			ZoneID:    "vla", V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "1:007c",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			ZoneID:       "myt",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "1:007c",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			ZoneID:       "man",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
		{
			ID:           "1:007c",
			FolderID:     "fooi5vu9rdejqc3p4b60",
			CreatedAt:    time.Date(1, time.January, 1, 0, 0, 0, 0, time.UTC),
			Name:         "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			Description:  "",
			Labels:       map[string]string(nil),
			NetworkID:    "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
			ZoneID:       "iva",
			V4CIDRBlocks: []string(nil),
			V6CIDRBlocks: []string(nil),
		},
	}

	assert.Equal(t, expected, actual)
}

func TestClient_GetSubnetsTooLongMacroID(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockRacktables := racktablesMocks.NewMockClient(ctrl)

	testClient := Client{
		rt: mockRacktables,
	}

	ctx := context.Background()

	mockRacktables.EXPECT().GetMacro(ctx, "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_").
		Return(racktables.Macro{
			IDs:              []racktables.MacroID{{ID: "123456789", Description: ""}},
			Name:             "_MDB_CONTROLPLANE_PORTO_TEST_NETS_",
			Owners:           []string{"velom", "svc_internalmdb_services_management"},
			OwnerService:     "svc_internalmdb",
			Parent:           "_PGAASINTERNALNETS_",
			Description:      "",
			Internet:         0,
			Secured:          0,
			CanCreateNetwork: 1,
		}, nil)

	network := networkProvider.Network{
		ID:          "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
		FolderID:    "fooi5vu9rdejqc3p4b60",
		Name:        "_CLOUD_YANDEX_CLIENTS_VERTISDEV_LB_NETS_",
		Description: "",
	}

	actual, err := testClient.GetSubnets(ctx, network)
	require.Error(t, err)
	assert.ErrorIs(t, err, ErrMacroIDIsTooLong)

	assert.Equal(t, []networkProvider.Subnet(nil), actual)
}
