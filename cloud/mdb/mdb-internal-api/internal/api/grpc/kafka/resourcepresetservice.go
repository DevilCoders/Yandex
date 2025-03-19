package kafka

import (
	"context"
	"sort"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/slices"
)

type ResourcePresetService struct {
	kfv1.UnimplementedResourcePresetServiceServer

	l       log.Logger
	Kafka   kafka.Kafka
	console console.Console
}

func NewResourcePresetService(kafka kafka.Kafka, console console.Console, l log.Logger) *ResourcePresetService {
	return &ResourcePresetService{Kafka: kafka, console: console, l: l}
}

func groupResourcePresetsByResourcePresetID(resourcePresets []consolemodels.ResourcePreset) map[string][]consolemodels.ResourcePreset {
	res := make(map[string][]consolemodels.ResourcePreset)

	for _, resourcePreset := range resourcePresets {
		res[resourcePreset.ExtID] = append(res[resourcePreset.ExtID], resourcePreset)
	}

	return res
}

func getZonesFromPresets(resourcePresets []consolemodels.ResourcePreset) []string {
	zones := make([]string, 0)
	for _, resourcePreset := range resourcePresets {
		zones = append(zones, resourcePreset.Zone)
	}
	return slices.DedupStrings(zones)
}

func consoleResourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset) *kfv1.ResourcePreset {
	zones := getZonesFromPresets(resourcePresets)
	preset := resourcePresets[0]
	return &kfv1.ResourcePreset{
		Id:      preset.ExtID,
		Cores:   preset.CPULimit,
		Memory:  preset.MemoryLimit,
		ZoneIds: zones,
	}
}

func (rps *ResourcePresetService) Get(ctx context.Context, req *kfv1.GetResourcePresetRequest) (*kfv1.ResourcePreset, error) {
	resourcePresetsAll, err := rps.console.GetResourcePresetsByClusterType(ctx, clusters.TypeKafka, "", false)
	if err != nil {
		return nil, err
	}

	resourcePresetsByID := groupResourcePresetsByResourcePresetID(resourcePresetsAll)
	resourcePresets, found := resourcePresetsByID[req.GetResourcePresetId()]
	if !found {
		return nil, semerr.NotFoundf("resource preset %q not found", req.GetResourcePresetId())
	}

	return consoleResourcePresetsToGRPC(resourcePresets), nil
}

func (rps *ResourcePresetService) List(ctx context.Context, req *kfv1.ListResourcePresetsRequest) (*kfv1.ListResourcePresetsResponse, error) {
	consoleResourcePresets, err := rps.console.GetResourcePresetsByClusterType(ctx, clusters.TypeKafka, "", false)
	if err != nil {
		return nil, err
	}

	resourcePresetsByID := groupResourcePresetsByResourcePresetID(consoleResourcePresets)
	var sortedResourcePresetIDs []string
	for resourcePresetID := range resourcePresetsByID {
		sortedResourcePresetIDs = append(sortedResourcePresetIDs, resourcePresetID)
	}
	sort.Strings(sortedResourcePresetIDs)

	var pageToken pagination.OffsetPageToken
	err = api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	page := pagination.NewPage(int64(len(sortedResourcePresetIDs)), req.GetPageSize(), pageToken.Offset)

	resourcePresetIDsToList := sortedResourcePresetIDs[page.LowerIndex:page.UpperIndex]

	grpcResourcePresets := make([]*kfv1.ResourcePreset, len(resourcePresetIDsToList))
	for idx, resourcePresetID := range resourcePresetIDsToList {
		resourcePresets := resourcePresetsByID[resourcePresetID]
		grpcResourcePresets[idx] = consoleResourcePresetsToGRPC(resourcePresets)
	}

	resourcePresetPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(resourcePresetPageToken, false)
	if err != nil {
		return nil, err
	}

	res := kfv1.ListResourcePresetsResponse{
		ResourcePresets: grpcResourcePresets,
		NextPageToken:   nextPageToken,
	}

	return &res, nil
}
