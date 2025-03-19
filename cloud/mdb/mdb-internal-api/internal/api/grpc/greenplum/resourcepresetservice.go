package greenplum

import (
	"context"
	"sort"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/slices"
)

type ResourcePresetService struct {
	gpv1.UnimplementedResourcePresetServiceServer
	Console console.Console
}

func NewResourcePresetService(console console.Console) *ResourcePresetService {
	return &ResourcePresetService{
		Console: console,
	}
}

func ResourcePresetType2Role(rpType gpv1.ResourcePreset_Type) hosts.Role {
	switch rpType {
	case gpv1.ResourcePreset_MASTER:
		return hosts.RoleGreenplumMasterNode
	case gpv1.ResourcePreset_SEGMENT:
		return hosts.RoleGreenplumSegmentNode
	}
	return hosts.RoleUnknown
}

func resourcePresetsToGRPC(rpRole hosts.Role, resourcePresets map[string][]consolemodels.ResourcePreset) *gpv1.ResourcePreset {
	zones := make([]string, 0)
	for zone := range resourcePresets {
		zones = append(zones, zone)
	}
	zones = slices.DedupStrings(zones)

	diskTypes := make([]string, 0)
	for _, rp := range resourcePresets[zones[0]] {
		diskTypes = append(diskTypes, rp.DiskTypeExtID)
	}
	diskTypes = slices.DedupStrings(diskTypes)

	res := resourcePresets[zones[0]][0]

	rpType := gpv1.ResourcePreset_MASTER
	var hostCountDivider, maxSegmentInHost int64 = 1, 0
	if rpRole == hosts.RoleGreenplumSegmentNode {
		hostCountDivider = 2
		maxSegmentInHost = greenplum.MaxSegmentInHostCountCalc(res.MemoryLimit, false)
		rpType = gpv1.ResourcePreset_SEGMENT
	}

	return &gpv1.ResourcePreset{
		Id:                    res.ExtID,
		ZoneIds:               zones,
		Cores:                 res.CPULimit,
		Memory:                res.MemoryLimit,
		Type:                  rpType,
		DiskTypeIds:           diskTypes,
		HostCountDivider:      hostCountDivider,
		MaxSegmentInHostCount: maxSegmentInHost,
	}
}

func (rps *ResourcePresetService) Get(ctx context.Context, req *gpv1.GetResourcePresetRequest) (*gpv1.ResourcePreset, error) {
	rp, err := rps.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeGreenplumCluster, "", false)
	if err != nil {
		return nil, err
	}

	rpRole := ResourcePresetType2Role(req.GetType())
	if rpRole == hosts.RoleUnknown {
		return nil, semerr.InvalidInput("resource preset type must be MASTER or SEGMENT")
	}

	rpAggregated := common.AggregateResourcePresets(rp)
	rpByIds := rpAggregated[rpRole]

	resourcePresets, found := rpByIds[req.GetResourcePresetId()]
	if found {
		return resourcePresetsToGRPC(rpRole, resourcePresets), nil
	}

	return nil, semerr.NotFoundf("resource preset %q not found", req.ResourcePresetId)
}

func (rps *ResourcePresetService) List(ctx context.Context, req *gpv1.ListResourcePresetsRequest) (*gpv1.ListResourcePresetsResponse, error) {
	rp, err := rps.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeGreenplumCluster, "", false)
	if err != nil {
		return nil, err
	}

	var pageToken pagination.OffsetPageToken
	err = api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	rpRole := ResourcePresetType2Role(req.GetType())
	if rpRole == hosts.RoleUnknown {
		return nil, semerr.InvalidInput("resource preset type must be MASTER or SEGMENT")
	}

	rpAggregated := common.AggregateResourcePresets(rp)
	resourcesGRPC := make([]*gpv1.ResourcePreset, 0)

	for _, rpByZones := range rpAggregated[rpRole] {
		resourcesGRPC = append(resourcesGRPC, resourcePresetsToGRPC(rpRole, rpByZones))
	}

	sort.Slice(resourcesGRPC, func(i, j int) bool {
		return resourcesGRPC[i].Id < resourcesGRPC[j].Id
	})

	page := pagination.NewPage(int64(len(resourcesGRPC)), req.GetPageSize(), pageToken.Offset)
	resourcePresetPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(resourcePresetPageToken, false)
	if err != nil {
		return nil, err
	}

	return &gpv1.ListResourcePresetsResponse{
		ResourcePresets: resourcesGRPC[page.LowerIndex:page.UpperIndex],
		NextPageToken:   nextPageToken,
	}, nil
}
