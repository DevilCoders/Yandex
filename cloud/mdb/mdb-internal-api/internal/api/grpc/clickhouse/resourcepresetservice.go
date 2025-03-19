package clickhouse

import (
	"context"
	"sort"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/slices"
)

type ResourcePresetService struct {
	chv1.UnimplementedResourcePresetServiceServer

	l       log.Logger
	ch      clickhouse.ClickHouse
	console console.Console
}

var _ chv1.ResourcePresetServiceServer = &ResourcePresetService{}

func NewResourcePresetService(ch clickhouse.ClickHouse, console console.Console, l log.Logger) *ResourcePresetService {
	return &ResourcePresetService{ch: ch, console: console, l: l}
}

func groupResourcePresetsByResourcePresetID(resourcePresets []consolemodels.ResourcePreset) map[string][]consolemodels.ResourcePreset {
	res := make(map[string][]consolemodels.ResourcePreset)

	for _, resourcePreset := range resourcePresets {
		res[resourcePreset.ExtID] = append(res[resourcePreset.ExtID], resourcePreset)
	}

	return res
}

// resourcePresetFromPartialResourcePresets assumes that all partial resource presets have the same ExtID
func resourcePresetFromPartialResourcePresets(partialResourcePresets []consolemodels.ResourcePreset) chmodels.ResourcePreset {
	res := chmodels.ResourcePreset{
		ID:     partialResourcePresets[0].ExtID,
		Cores:  partialResourcePresets[0].CPULimit,
		Memory: partialResourcePresets[0].MemoryLimit,
	}

	for _, resourcePreset := range partialResourcePresets {
		res.ZoneIds = append(res.ZoneIds, resourcePreset.Zone)
		res.HostRoles = append(res.HostRoles, resourcePreset.Role)
	}

	res.ZoneIds = slices.DedupStrings(res.ZoneIds)
	sort.Strings(res.ZoneIds)

	res.HostRoles = hosts.DedupRoles(res.HostRoles)
	sort.Slice(res.HostRoles, func(i, j int) bool {
		return res.HostRoles[i].String() < res.HostRoles[j].String()
	})

	return res
}

func (rps *ResourcePresetService) Get(ctx context.Context, req *chv1.GetResourcePresetRequest) (*chv1.ResourcePreset, error) {
	resourcePresets, err := rps.console.GetResourcePresetsByClusterType(ctx, clusters.TypeClickHouse, "", false)
	if err != nil {
		return nil, err
	}

	resourcePresetsByID := groupResourcePresetsByResourcePresetID(resourcePresets)
	partialResourcePresets, found := resourcePresetsByID[req.GetResourcePresetId()]
	if !found {
		return nil, semerr.NotFoundf("resource preset %q not found", req.GetResourcePresetId())
	}

	return ResourcePresetToGRPC(resourcePresetFromPartialResourcePresets(partialResourcePresets)), nil
}

func (rps *ResourcePresetService) List(ctx context.Context, req *chv1.ListResourcePresetsRequest) (*chv1.ListResourcePresetsResponse, error) {
	consoleResourcePresets, err := rps.console.GetResourcePresetsByClusterType(ctx, clusters.TypeClickHouse, "", false)
	if err != nil {
		return nil, err
	}

	resourcePresetsByID := groupResourcePresetsByResourcePresetID(consoleResourcePresets)
	var sortedResourcePresetIDs []string
	for resourcePresetID := range resourcePresetsByID {
		sortedResourcePresetIDs = append(sortedResourcePresetIDs, resourcePresetID)
	}
	sort.Strings(sortedResourcePresetIDs)

	var resourcePresets []chmodels.ResourcePreset
	for _, resourcePresetID := range sortedResourcePresetIDs {
		resourcePresets = append(resourcePresets, resourcePresetFromPartialResourcePresets(resourcePresetsByID[resourcePresetID]))
	}

	var pageToken pagination.OffsetPageToken
	err = api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	page := pagination.NewPage(int64(len(resourcePresets)), req.GetPageSize(), pageToken.Offset)

	grpcResourcePresets := ResourcePresetsToGRPC(resourcePresets[page.LowerIndex:page.UpperIndex])

	resourcePresetPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(resourcePresetPageToken, false)
	if err != nil {
		return nil, err
	}

	res := chv1.ListResourcePresetsResponse{
		ResourcePresets: grpcResourcePresets,
		NextPageToken:   nextPageToken,
	}

	return &res, nil
}
