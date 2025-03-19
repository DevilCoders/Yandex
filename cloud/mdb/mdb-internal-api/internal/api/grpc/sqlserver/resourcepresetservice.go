package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type ResourcePresetService struct {
	ssv1.UnimplementedResourcePresetServiceServer
	Console console.Console
}

func NewResourcePresetService(console console.Console) *ResourcePresetService {
	return &ResourcePresetService{
		Console: console,
	}
}

func (rps *ResourcePresetService) List(ctx context.Context, _ *ssv1.ListResourcePresetsRequest) (*ssv1.ListResourcePresetsResponse, error) {
	resourcePresets, err := rps.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeSQLServer, "", false)
	if err != nil {
		return nil, err
	}
	crps, err := resourcePresetsToGRPC(resourcePresets, map[string]compute.HostGroupHostType{})
	if err != nil {
		return nil, err
	}
	return &ssv1.ListResourcePresetsResponse{ResourcePresets: consoleResourcePresetsToSimple(crps)}, nil
}

func (rps *ResourcePresetService) Get(ctx context.Context, req *ssv1.GetResourcePresetRequest) (*ssv1.ResourcePreset, error) {
	resourcePresets, err := rps.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeSQLServer, "", false)
	if err != nil {
		return nil, err
	}
	crps, err := resourcePresetsToGRPC(resourcePresets, map[string]compute.HostGroupHostType{})
	if err != nil {
		return nil, err
	}
	for _, crp := range crps {
		if crp.PresetId == req.ResourcePresetId {
			return consoleResourcePresetToSimple(crp), nil
		}
	}
	return nil, semerr.NotFoundf("resource preset %q not found", req.ResourcePresetId)
}
