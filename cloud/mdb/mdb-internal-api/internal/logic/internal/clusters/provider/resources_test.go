package provider

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/intervals"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestValidateResourcesCheckDecommissionedZone(t *testing.T) {
	currentResourcePresetExtID := "currentResourcePresetExtID"

	typ := clusters.TypeKafka
	role := hosts.RoleKafka
	currentDiskSize := int64(100)
	ctx, f := newMocks(t)
	session := sessions.Session{Quota: quota.NewConsumption(quota.Resources{SSDSpace: 10 * currentDiskSize}, quota.Resources{})}
	zoneID := "zoneID"
	preset := resources.Preset{ExtID: "foo"}

	t.Run("Successful adding host", func(t *testing.T) {
		hostGroup := clusterslogic.HostGroup{
			Role:                       role,
			CurrentResourcePresetExtID: optional.NewString(currentResourcePresetExtID),
			DiskTypeExtID:              resources.LocalSSD,
			CurrentDiskSize:            optional.NewInt64(currentDiskSize),
			HostsToAdd: clusterslogic.ZoneHostsList{
				clusterslogic.ZoneHosts{
					ZoneID: zoneID,
					Count:  1,
				},
			},
		}
		f.MetaDB.EXPECT().ResourcePresetByExtID(ctx, currentResourcePresetExtID).Return(preset, nil)
		availableZones := []string{zoneID}
		mockMetaDBListZones(f, ctx, availableZones)
		mockMetaDBDiskTypesForLocalSSD(f, ctx)
		mockMetaDBValidResourcesForLocalSSD(f, ctx, typ, role, currentResourcePresetExtID, "", currentDiskSize)
		mockMetaDBValidResourcesForLocalSSD(f, ctx, typ, role, currentResourcePresetExtID, zoneID, currentDiskSize)

		cluster := NewClusters(f.MetaDB, nil, nil, nil, nil,
			nil, nil, nil, nil, logic.Config{}, nil)

		resolvedHostGroup, hasChanges, err := cluster.ValidateResources(ctx, session, typ, hostGroup)
		require.NoError(t, err)
		require.Equal(t, true, hasChanges)
		require.Equal(t, clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{{
			HostGroup:             hostGroup,
			CurrentResourcePreset: preset,
		}}), resolvedHostGroup)
	})

	t.Run("Add host to decommissioned zone should return error", func(t *testing.T) {
		checkDecommissionedZoneValidation(t, false, false)
	})

	t.Run("Add host to decommissioned zone without checking decommissioned zone should work successfully", func(t *testing.T) {
		checkDecommissionedZoneValidation(t, true, false)
	})

	t.Run("Add host to unavailable zone should return error", func(t *testing.T) {
		checkDecommissionedZoneValidation(t, false, true)
	})
}

func TestValidateQuota(t *testing.T) {
	currentResourcePresetExtID := "currentResourcePresetExtID"
	currentResourcePreset := resources.Preset{}
	newResourcePresetExtID := "newResourcePresetExtID"
	newResourcePreset := resources.Preset{}
	diskTypeExtID := "diskTypeExtID"
	dts := resources.NewDiskTypes(map[string]resources.DiskType{diskTypeExtID: resources.DiskTypeSSD})
	currentDiskSize := int64(100)
	newDiskSize := int64(200)
	session := sessions.Session{Quota: quota.NewConsumption(quota.Resources{}, quota.Resources{})}

	t.Run("Ok", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				CurrentResourcePresetExtID: optional.NewString(currentResourcePresetExtID),
				NewResourcePresetExtID:     optional.NewString(newResourcePresetExtID),
				DiskTypeExtID:              diskTypeExtID,
				CurrentDiskSize:            optional.NewInt64(currentDiskSize),
				NewDiskSize:                optional.NewInt64(newDiskSize),
				HostsToAdd:                 clusterslogic.ZoneHostsList{},
				HostsCurrent:               clusterslogic.ZoneHostsList{},
				HostsToRemove:              clusterslogic.ZoneHostsList{},
			},
			CurrentResourcePreset: currentResourcePreset,
			NewResourcePreset:     newResourcePreset,
		}
		require.NoError(t, validateQuota(session, dts, hg))
	})

	// TODO: implement actually useful test
}

func TestLoadHostGroupResourcePresets(t *testing.T) {
	ctx := context.Background()

	t.Run("Current", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("1")}
		preset := resources.Preset{ExtID: "foo"}
		mock.EXPECT().ResourcePresetByExtID(ctx, hg.CurrentResourcePresetExtID.String).
			Return(preset, nil)
		rhg, err := loadHostGroupResourcePresets(ctx, mock, hg)
		require.NoError(t, err)
		require.Equal(t, hg, rhg.HostGroup)
		require.Equal(t, preset, rhg.CurrentResourcePreset)
		require.Equal(t, resources.Preset{}, rhg.NewResourcePreset)
	})

	t.Run("New", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{NewResourcePresetExtID: optional.NewString("1")}
		preset := resources.Preset{ExtID: "foo"}
		mock.EXPECT().ResourcePresetByExtID(ctx, hg.NewResourcePresetExtID.String).
			Return(preset, nil)
		rhg, err := loadHostGroupResourcePresets(ctx, mock, hg)
		require.NoError(t, err)
		require.Equal(t, hg, rhg.HostGroup)
		require.Equal(t, preset, rhg.NewResourcePreset)
		require.Equal(t, resources.Preset{}, rhg.CurrentResourcePreset)
	})

	t.Run("CurrentAndNew", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("1"), NewResourcePresetExtID: optional.NewString("2")}
		currentPreset := resources.Preset{ExtID: "foo"}
		newPreset := resources.Preset{ExtID: "bar"}
		mock.EXPECT().ResourcePresetByExtID(ctx, hg.CurrentResourcePresetExtID.String).
			Return(currentPreset, nil)
		mock.EXPECT().ResourcePresetByExtID(ctx, hg.NewResourcePresetExtID.String).
			Return(newPreset, nil)
		rhg, err := loadHostGroupResourcePresets(ctx, mock, hg)
		require.NoError(t, err)
		require.Equal(t, hg, rhg.HostGroup)
		require.Equal(t, currentPreset, rhg.CurrentResourcePreset)
		require.Equal(t, newPreset, rhg.NewResourcePreset)
	})
}

func TestValidateDecommissionedResourcePresets(t *testing.T) {
	cfg := logic.ResourceValidationConfig{DecommissionedResourcePresets: []string{"dec"}}

	t.Run("OkCurrentWithoutAddHosts", func(t *testing.T) {
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("notdec")}
		require.NoError(t, validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg))
	})

	t.Run("OkCurrentWithAddHosts", func(t *testing.T) {
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("notdec"), HostsToAdd: clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "foo", Count: 1}}}
		require.NoError(t, validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg))
	})

	t.Run("OkNew", func(t *testing.T) {
		hg := clusterslogic.HostGroup{NewResourcePresetExtID: optional.NewString("notdec")}
		require.NoError(t, validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg))
	})

	t.Run("DecommissionedCurrentWithoutAddHosts", func(t *testing.T) {
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("dec")}
		require.NoError(t, validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg))
	})

	t.Run("DecommissionedCurrentWithAddHosts", func(t *testing.T) {
		hg := clusterslogic.HostGroup{CurrentResourcePresetExtID: optional.NewString("dec"), HostsToAdd: clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "foo", Count: 1}}}
		err := validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("resource preset %q is decommissioned", hg.CurrentResourcePresetExtID.String))
	})

	t.Run("DecommissionedNew", func(t *testing.T) {
		hg := clusterslogic.HostGroup{NewResourcePresetExtID: optional.NewString("dec")}
		err := validateDecommissionedResourcePresets(cfg, sessions.Session{}, hg)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("resource preset %q is decommissioned", hg.NewResourcePresetExtID.String))
	})

	t.Run("FeatureFlag", func(t *testing.T) {
		hg := clusterslogic.HostGroup{NewResourcePresetExtID: optional.NewString("dec")}
		require.NoError(t, validateDecommissionedResourcePresets(cfg, sessions.Session{FeatureFlags: featureflags.NewFeatureFlags([]string{FeatureFlagAllowDecommissionedResourcePreset})}, hg))
	})
}

func TestValidateDecommissionedZones(t *testing.T) {
	cfg := logic.ResourceValidationConfig{DecommissionedZones: []string{"decommissioned"}}

	t.Run("Ok", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				HostsToAdd:                 clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "available", Count: 1}},
				CurrentResourcePresetExtID: optional.NewString("current"),
				NewResourcePresetExtID:     optional.NewString("new"),
			},
		}
		require.NoError(t, validateDecommissionedZones(cfg, sessions.Session{}, hg))
	})

	t.Run("HostAddDecommissioned", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{HostsToAdd: clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}}},
		}
		err := validateDecommissionedZones(cfg, sessions.Session{}, hg)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("no new resources can be created in zone %q", hg.HostsToAdd[0].ZoneID))
	})

	t.Run("HostAddDecommissionedDFeatureFlag", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{HostsToAdd: clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}}},
		}
		require.NoError(t, validateDecommissionedZones(cfg, sessions.Session{FeatureFlags: featureflags.NewFeatureFlags([]string{featureFlagAllowDecommissionedZone})}, hg))
	})

	bigPreset := resources.Preset{MemoryLimit: 2}
	smallPreset := resources.Preset{MemoryLimit: 1}

	t.Run("UpscaleDecommissioned", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				HostsCurrent:               clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}},
				CurrentResourcePresetExtID: optional.NewString("current"),
				NewResourcePresetExtID:     optional.NewString("new"),
			},
			CurrentResourcePreset: smallPreset,
			NewResourcePreset:     bigPreset,
		}
		err := validateDecommissionedZones(cfg, sessions.Session{}, hg)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("no resources can be upscaled in zone %q", hg.HostsCurrent[0].ZoneID))
	})

	t.Run("DownscaleDecommissioned", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				HostsCurrent:               clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}},
				CurrentResourcePresetExtID: optional.NewString("current"),
				NewResourcePresetExtID:     optional.NewString("new"),
			},
			CurrentResourcePreset: bigPreset,
			NewResourcePreset:     smallPreset,
		}
		require.NoError(t, validateDecommissionedZones(cfg, sessions.Session{}, hg))
	})

	t.Run("ResourcePresetNotChangedDecommissioned", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				HostsCurrent:               clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}},
				CurrentResourcePresetExtID: optional.NewString("current"),
				NewResourcePresetExtID:     optional.NewString("current"),
			},
		}
		require.NoError(t, validateDecommissionedZones(cfg, sessions.Session{}, hg))
	})

	t.Run("UpscaleDecommissionedFeatureFlag", func(t *testing.T) {
		hg := clusterslogic.ResolvedHostGroup{
			HostGroup: clusterslogic.HostGroup{
				HostsCurrent:               clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: "decommissioned", Count: 1}},
				CurrentResourcePresetExtID: optional.NewString("current"),
				NewResourcePresetExtID:     optional.NewString("new"),
			},
			CurrentResourcePreset: smallPreset,
			NewResourcePreset:     bigPreset,
		}
		require.NoError(t, validateDecommissionedZones(cfg, sessions.Session{FeatureFlags: featureflags.NewFeatureFlags([]string{featureFlagAllowDecommissionedZone})}, hg))
	})
}

func TestValidateHostGroup(t *testing.T) {
	ctx := context.Background()
	cfg := logic.ResourceValidationConfig{}
	var ffs featureflags.FeatureFlags
	typ := clusters.TypePostgreSQL
	role := hosts.RolePostgreSQL
	resourcePresetExtID := "resourcePresetExtID"
	diskTypeExtID := "diskTypeExtID"
	zoneID := "zoneID"
	dts := resources.NewDiskTypes(map[string]resources.DiskType{diskTypeExtID: resources.DiskTypeSSD})

	t.Run("Ok", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{DiskTypeExtID: diskTypeExtID, Role: role, CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID)}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}}}, nil)
		require.NoError(t, validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg))
	})

	t.Run("ChecksDiskTypeWithAddedHosts", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{DiskTypeExtID: "invalid", HostsToAdd: clusterslogic.ZoneHostsList{{}}}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
		require.Error(t, validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg))
	})

	t.Run("NotChecksDiskTypeWithoutAddedHosts", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{DiskTypeExtID: "invalid", Role: role, CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID)}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID), optional.NewString("invalid"), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}}}, nil)
		require.NoError(t, validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg))
	})

	t.Run("ValidateComputeResourceChanges", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		hg := clusterslogic.HostGroup{
			DiskTypeExtID:   "network-ssd",
			CurrentDiskSize: optional.NewInt64(2),
			NewDiskSize:     optional.NewInt64(1),
		}
		preset := resources.Preset{VType: environment.VTypeCompute}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg, CurrentResourcePreset: preset}
		err := validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg)
		require.Error(t, err)
		semanticErr := semerr.AsSemanticError(err)
		require.Equal(t, semerr.SemanticAuthorization, semanticErr.Semantic)
	})

	t.Run("ChecksValidateHostsCount", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		expectedErr := xerrors.New("error")
		hg := clusterslogic.HostGroup{DiskTypeExtID: diskTypeExtID, Role: role, CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID), HostsToAdd: clusterslogic.ZoneHostsList{{Count: 1}}}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.String{}).
			Return(nil, expectedErr)
		err := validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
	})

	t.Run("ChecksValidateHostInZone", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		diskSize := int64(1)
		expectedErr := xerrors.New("error")
		hg := clusterslogic.HostGroup{DiskTypeExtID: diskTypeExtID, Role: role, CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID), CurrentDiskSize: optional.NewInt64(diskSize), HostsToAdd: clusterslogic.ZoneHostsList{{ZoneID: zoneID, Count: 1}}}
		rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
		hostsCountCall := mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.String{}).
			Return([]resources.Valid{{MinHosts: 1, MaxHosts: 1, DiskSizes: []int64{1}}}, nil)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.NewString(zoneID)).
			Return(nil, expectedErr).After(hostsCountCall)
		err := validateHostGroup(ctx, cfg, mock, sessions.Session{}, typ, dts, rhg)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
	})
}

func TestValidateHostsDiskSize(t *testing.T) {
	ctx := context.Background()
	cfg := logic.ResourceValidationConfig{MinimalDiskUnit: 4}
	var ffs featureflags.FeatureFlags
	typ := clusters.TypePostgreSQL
	role := hosts.RolePostgreSQL
	resourcePresetExtID := "resourcePresetExtID"
	resourcePresetExtID2 := "resourcePresetExtID2"
	diskTypeExtID := "diskTypeExtID"
	zoneID := "zoneID"
	zoneID2 := "zoneID2"

	t.Run("Ok", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		diskSize := int64(4)
		hg := clusterslogic.HostGroup{
			DiskTypeExtID:              diskTypeExtID,
			Role:                       role,
			CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID),
			NewResourcePresetExtID:     optional.NewString(resourcePresetExtID2),
			CurrentDiskSize:            optional.NewInt64(diskSize),
			HostsCurrent:               clusterslogic.ZoneHostsList{{ZoneID: zoneID, Count: 1}},
			HostsToAdd:                 clusterslogic.ZoneHostsList{{ZoneID: zoneID2, Count: 1}},
		}
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID2), optional.NewString(diskTypeExtID), optional.NewString(zoneID)).
			Return([]resources.Valid{{DiskSizes: []int64{diskSize}}}, nil)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID2), optional.NewString(diskTypeExtID), optional.NewString(zoneID2)).
			Return([]resources.Valid{{DiskSizes: []int64{diskSize}}}, nil)

		require.NoError(t, validateHostsDiskSize(ctx, cfg, mock, ffs, typ, hg))
	})

	t.Run("NoChecksNeeded", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		diskSize := int64(4)
		hg := clusterslogic.HostGroup{
			DiskTypeExtID:              diskTypeExtID,
			Role:                       role,
			CurrentResourcePresetExtID: optional.NewString(resourcePresetExtID),
			CurrentDiskSize:            optional.NewInt64(diskSize),
			HostsCurrent:               clusterslogic.ZoneHostsList{{ZoneID: zoneID, Count: 1}},
		}

		require.NoError(t, validateHostsDiskSize(ctx, cfg, mock, ffs, typ, hg))
	})
}

func TestValidateComputeResourceChanges(t *testing.T) {
	dts := resources.NewDiskTypes(map[string]resources.DiskType{})
	checkError := func(err error) {
		require.Error(t, err)
		semanticErr := semerr.AsSemanticError(err)
		require.NotNil(t, semanticErr)
		require.Equal(t, semerr.SemanticAuthorization, semanticErr.Semantic)
		require.Equal(t, "requested feature is not available", semanticErr.Message)
	}

	for _, typ := range clusters.Types() {
		t.Run("UnableToShrinkNetworkDiskWithoutFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:   "network-hdd",
				CurrentDiskSize: optional.NewInt64(2),
				NewDiskSize:     optional.NewInt64(1),
			}
			checkError(validateComputeResourceChanges(ffs, dts, hg, typ))
		})

		t.Run("AllowedToShrinkNetworkDiskWithFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{featureFlagNetworkDiskTruncate})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:   "network-hdd",
				CurrentDiskSize: optional.NewInt64(2),
				NewDiskSize:     optional.NewInt64(1),
			}
			if typ == clusters.TypeSQLServer {
				checkError(validateComputeResourceChanges(ffs, dts, hg, typ))
			} else {
				require.NoError(t, validateComputeResourceChanges(ffs, dts, hg, typ))
			}
		})

		t.Run("UnableToResizeNonreplicatedSsdWithoutFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:   "network-ssd-nonreplicated",
				CurrentDiskSize: optional.NewInt64(1),
				NewDiskSize:     optional.NewInt64(2),
			}
			checkError(validateComputeResourceChanges(ffs, dts, hg, typ))
		})

		t.Run("AllowedToResizeNonreplicatedSsdWithFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{featureFlagAllowNetworkSsdNonreplicatedResize})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:   "network-ssd-nonreplicated",
				CurrentDiskSize: optional.NewInt64(1),
				NewDiskSize:     optional.NewInt64(2),
			}
			require.NoError(t, validateComputeResourceChanges(ffs, dts, hg, typ))
		})

		t.Run("UnableToResizeLocalDiskWithoutFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:   "local-ssd",
				CurrentDiskSize: optional.NewInt64(1),
				NewDiskSize:     optional.NewInt64(2),
			}
			checkError(validateComputeResourceChanges(ffs, dts, hg, typ))
		})

		t.Run("UnableToChangeResourcesIfDiskIsLocalWithoutFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:              "local-ssd",
				CurrentResourcePresetExtID: optional.NewString("small"),
				NewResourcePresetExtID:     optional.NewString("large"),
			}
			checkError(validateComputeResourceChanges(ffs, dts, hg, typ))
		})

		t.Run("UnableToChangeResourcesIfDiskIsLocalWithoutFF_"+typ.Stringified(), func(t *testing.T) {
			ffs := featureflags.NewFeatureFlags([]string{featureFlagLocalDiskResize})
			hg := clusterslogic.HostGroup{
				DiskTypeExtID:              "local-ssd",
				CurrentDiskSize:            optional.NewInt64(1),
				NewDiskSize:                optional.NewInt64(2),
				CurrentResourcePresetExtID: optional.NewString("small"),
				NewResourcePresetExtID:     optional.NewString("large"),
			}
			require.NoError(t, validateComputeResourceChanges(ffs, dts, hg, typ))
		})
	}
}

func TestValidateHostDiskSize(t *testing.T) {
	ctx := context.Background()
	cfg := logic.ResourceValidationConfig{MinimalDiskUnit: 4}
	var ffs featureflags.FeatureFlags
	typ := clusters.TypePostgreSQL
	role := hosts.RolePostgreSQL
	preset := "resorce preset"
	diskType := "disk type"
	zone := "zone"

	t.Run("ValidDiskSizes", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		diskSize := int64(1)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{DiskSizes: []int64{diskSize}}}, nil)

		require.NoError(t, validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, diskSize))
	})

	t.Run("ValidDisk", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		interval, err := intervals.NewInt64(1, intervals.Inclusive, 1024, intervals.Inclusive)
		require.NoError(t, err)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{DiskSizeRange: optional.NewIntervalInt64(interval)}}, nil)

		require.NoError(t, validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 8))
	})

	t.Run("ChecksLoadError", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		expectedErr := xerrors.New("error")
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return(nil, expectedErr)

		err := validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 1)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
	})

	t.Run("ChecksValidResourcesExistence", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return(nil, nil)

		err := validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 1)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("resource preset %q and disk type %q are not available in zone %q", preset, diskType, zone))
	})

	t.Run("ChecksMultipleCombinations", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{}, {}}, nil)

		require.Error(t, validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 1))
	})

	t.Run("ChecksDiskSizeRange", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		interval, err := intervals.NewInt64(0, intervals.Inclusive, 1, intervals.Inclusive)
		require.NoError(t, err)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{DiskSizeRange: optional.NewIntervalInt64(interval)}}, nil)

		err = validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 2)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("invalid disk size, must be inside %s range", interval))

	})

	t.Run("ChecksMinimalDiskUnit", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		interval, err := intervals.NewInt64(0, intervals.Inclusive, 1, intervals.Inclusive)
		require.NoError(t, err)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{DiskSizeRange: optional.NewIntervalInt64(interval)}}, nil)

		err = validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 1)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("invalid disk size, must be multiple of %d bytes", cfg.MinimalDiskUnit))
	})

	t.Run("ChecksDiskSizes", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		diskSizes := []int64{1024, 2048}
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.NewString(zone)).
			Return([]resources.Valid{{DiskSizes: diskSizes}}, nil)

		err := validateHostDiskSize(ctx, cfg, mock, ffs, typ, role, preset, diskType, zone, 512)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("invalid disk size, it must be one of %v", diskSizes))

	})
}

func TestValidateHostsCount(t *testing.T) {
	ctx := context.Background()
	var ffs featureflags.FeatureFlags
	typ := clusters.TypePostgreSQL
	role := hosts.RolePostgreSQL
	preset := "resorce preset"
	diskType := "disk type"
	hg := clusterslogic.HostGroup{
		DiskTypeExtID:          diskType,
		NewResourcePresetExtID: optional.NewString(preset),
		Role:                   role,
		HostsCurrent: []clusterslogic.ZoneHosts{{
			ZoneID: "zone",
			Count:  1,
		}},
	}
	rhg := clusterslogic.ResolvedHostGroup{HostGroup: hg}
	t.Run("Valid", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 1, MaxHosts: 1}}, nil)

		require.NoError(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	t.Run("ChecksLoadError", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		expectedErr := xerrors.New("error")
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return(nil, expectedErr)

		err := validateHostsCount(ctx, mock, ffs, typ, rhg)
		require.Error(t, err)
		require.True(t, xerrors.Is(err, expectedErr))
	})

	t.Run("ChecksValidResourcesExistence", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return(nil, nil)

		err := validateHostsCount(ctx, mock, ffs, typ, rhg)
		require.Error(t, err)
		assert.True(t, semerr.IsInvalidInput(err))
		require.EqualError(t, err, fmt.Sprintf("resource preset %q and disk type %q are not available", preset, diskType))
	})

	t.Run("ChecksUniqueness", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 1, MaxHosts: 1}, {DiskSizes: []int64{1}, MinHosts: 2, MaxHosts: 2}}, nil)

		require.Error(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	t.Run("ChecksMin", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 2, MaxHosts: 2}}, nil)

		require.Error(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	rhg.HostGroup.HostsToAdd = []clusterslogic.ZoneHosts{
		{
			ZoneID: "zone",
			Count:  2,
		},
	}
	t.Run("ChecksMax", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 2, MaxHosts: 2}}, nil)

		require.Error(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	rhg.HostGroup.SkipValidations.MaxHosts = true
	t.Run("ChecksMax", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 2, MaxHosts: 2}}, nil)

		require.NoError(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	typ = clusters.TypeRedis
	rhg.NewResourcePreset = resources.Preset{VType: environment.VTypeCompute, ExtID: preset}
	role = hosts.RoleRedis
	rhg.HostGroup.Role = role
	t.Run("ChecksMinRedisComputeNetworkOk", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 3, MaxHosts: 3}}, nil)

		require.NoError(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	rhg.HostGroup.HostsToAdd[0].Count = 1
	t.Run("ChecksMinRedisComputeNetworkError", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 3, MaxHosts: 3}}, nil)

		require.Error(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})

	diskType = resources.LocalSSD
	rhg.HostGroup.DiskTypeExtID = diskType
	t.Run("ChecksMinRedisComputeLocalssdOk", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		mock := mocks.NewMockBackend(ctrl)
		mock.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(preset), optional.NewString(diskType), optional.String{}).
			Return([]resources.Valid{{DiskSizes: []int64{1}, MinHosts: 3, MaxHosts: 3}}, nil)

		require.NoError(t, validateHostsCount(ctx, mock, ffs, typ, rhg))
	})
}

func TestFormatHostCountError(t *testing.T) {
	require.Equal(
		t,
		"cluster SQL Server with resource preset \"db1.xlarge\" and disk type \"local-ssd\" allows at most 10 hosts",
		formatHostCountError(clusters.TypeSQLServer, hosts.RoleSQLServer, "db1.xlarge", "local-ssd", 10, true),
	)
}

func TestValidateHostCountLimitsUniqueness(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		require.NoError(t, validateHostCountLimitsUniqueness(nil))
	})

	t.Run("One", func(t *testing.T) {
		require.NoError(t, validateHostCountLimitsUniqueness([]resources.Valid{{MinHosts: 1, MaxHosts: 2}}))
	})

	t.Run("Multiple", func(t *testing.T) {
		require.NoError(t, validateHostCountLimitsUniqueness([]resources.Valid{{MinHosts: 1, MaxHosts: 2}, {MinHosts: 1, MaxHosts: 2}}))
	})

	t.Run("NotUnique", func(t *testing.T) {
		require.Error(t, validateHostCountLimitsUniqueness([]resources.Valid{{MinHosts: 1, MaxHosts: 2}, {MinHosts: 1, MaxHosts: 3}}))
	})
}

func TestListAvailableZonesForCloudAndRegion(t *testing.T) {
	session := sessions.Session{}
	ctx, f := newMocks(t)

	zones := []environment.Zone{
		{
			Name:      "zoneName11",
			RegionID:  "regionId1",
			CloudType: environment.CloudTypeAWS,
		},
		{
			Name:      "zoneName12",
			RegionID:  "regionId1",
			CloudType: environment.CloudTypeAWS,
		},
		{
			Name:      "zoneName21",
			RegionID:  "regionId2",
			CloudType: environment.CloudTypeAWS,
		},
		{
			Name:      "zoneName22",
			RegionID:  "regionId2",
			CloudType: environment.CloudTypeAWS,
		},
	}

	t.Run("When metaDB return error should return error", func(t *testing.T) {
		f.MetaDB.EXPECT().ListZones(ctx).Return(nil, xerrors.New("some error"))

		res, err := f.Cluster.ListAvailableZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "regionId", true)

		require.Error(t, err)
		require.Nil(t, res)
	})

	t.Run("When get zones by region should return only region zones", func(t *testing.T) {
		f.MetaDB.EXPECT().ListZones(ctx).Return(zones, nil)

		res, err := f.Cluster.ListAvailableZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "regionId1", true)

		require.NoError(t, err)
		require.ElementsMatch(t, res, []string{"zoneName11", "zoneName12"})
	})

	t.Run("When get zones by not existed region should return empty list", func(t *testing.T) {
		f.MetaDB.EXPECT().ListZones(ctx).Return(zones, nil)

		res, err := f.Cluster.ListAvailableZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "NOT_EXISTED_REGION_ID", true)

		require.NoError(t, err)
		require.ElementsMatch(t, res, []string{})
	})
}

func TestKafka_SelectHostZones(t *testing.T) {
	session := sessions.Session{}
	ctx, f := newMocks(t)

	f.MetaDB.EXPECT().ListZones(ctx).Return([]environment.Zone{
		{Name: "man"}, {Name: "vla"}, {Name: "sas", CloudType: environment.CloudTypeAWS, RegionID: "region1"},
	}, nil).AnyTimes()

	t.Run("When no available zones Select z all available", func(t *testing.T) {
		_, err := f.Cluster.SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "wrongRegionID", true, 3)
		require.EqualError(t, err, "no available zones in region \"wrongRegionID\"")
	})

	t.Run("Select only from region", func(t *testing.T) {
		zones, err := f.Cluster.SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "region1", true, 1)
		require.NoError(t, err)
		require.Len(t, zones, 1)
		require.ElementsMatch(t, zones, []string{"sas"})
	})

	t.Run("Select only from region", func(t *testing.T) {
		_, err := f.Cluster.SelectZonesForCloudAndRegion(ctx, session, environment.CloudTypeAWS, "region1", true, 3)
		require.Error(t, err, "not found enough available zones (was needed 3 but found 1) for region \"region1\"")
	})
}

func checkDecommissionedZoneValidation(t *testing.T, skipDecommissionedZoneValidation bool, unavailableZone bool) {
	ctx, f := newMocks(t)
	currentResourcePresetExtID := "currentResourcePresetExtID"
	typ := clusters.TypeKafka
	role := hosts.RoleKafka
	currentDiskSize := int64(100)
	session := sessions.Session{Quota: quota.NewConsumption(quota.Resources{SSDSpace: 10 * currentDiskSize}, quota.Resources{})}
	decomissionedZoneID := "decomissionedZoneID"
	availableZoneID := "availableZoneID"
	unavailableZoneID := "noZoneID"
	preset := resources.Preset{ExtID: "foo"}

	usedZoneid := decomissionedZoneID
	if unavailableZone {
		usedZoneid = unavailableZoneID
	}
	hostGroup := clusterslogic.HostGroup{
		Role:                       role,
		CurrentResourcePresetExtID: optional.NewString(currentResourcePresetExtID),
		DiskTypeExtID:              resources.LocalSSD,
		CurrentDiskSize:            optional.NewInt64(currentDiskSize),
		HostsToAdd: clusterslogic.ZoneHostsList{
			clusterslogic.ZoneHosts{
				ZoneID: usedZoneid,
				Count:  1,
			},
		},
		SkipValidations: clusterslogic.SkipValidations{
			DecommissionedZone: skipDecommissionedZoneValidation,
		},
	}
	f.MetaDB.EXPECT().ResourcePresetByExtID(ctx, currentResourcePresetExtID).Return(preset, nil)
	availableZones := []string{availableZoneID, decomissionedZoneID}
	mockMetaDBListZones(f, ctx, availableZones)
	if skipDecommissionedZoneValidation {
		mockMetaDBDiskTypesForLocalSSD(f, ctx)
		mockMetaDBValidResourcesForLocalSSD(f, ctx, typ, role, currentResourcePresetExtID, "", currentDiskSize)
		mockMetaDBValidResourcesForLocalSSD(f, ctx, typ, role, currentResourcePresetExtID, decomissionedZoneID, currentDiskSize)
	}

	cfg := logic.Config{ResourceValidation: logic.ResourceValidationConfig{DecommissionedZones: []string{decomissionedZoneID}}}
	cluster := NewClusters(f.MetaDB, nil, nil, nil, nil,
		nil, nil, nil, nil, cfg, nil)

	resolvedHostGroup, hasChanged, err := cluster.ValidateResources(ctx, session, typ, hostGroup)

	if unavailableZone {
		require.EqualError(t, err, fmt.Sprintf("invalid zone %s, valid are: %s", unavailableZoneID, availableZoneID))
	} else if skipDecommissionedZoneValidation {
		require.NoError(t, err)
		require.Equal(t, true, hasChanged)
		require.Equal(t, clusterslogic.NewResolvedHostGroups([]clusterslogic.ResolvedHostGroup{{
			HostGroup:             hostGroup,
			CurrentResourcePreset: preset,
		}}), resolvedHostGroup)
	} else {
		require.EqualError(t, err, "no new resources can be created in zone \"decomissionedZoneID\"")
	}
}

func mockMetaDBListZones(f clusterFixture, ctx context.Context, zones []string) {
	zonesObj := make([]environment.Zone, len(zones))
	for _, zone := range zones {
		zonesObj = append(zonesObj, environment.Zone{Name: zone})
	}
	f.MetaDB.EXPECT().ListZones(ctx).Return(zonesObj, nil)
}

func mockMetaDBDiskTypesForLocalSSD(f clusterFixture, ctx context.Context) {
	dts := resources.NewDiskTypes(map[string]resources.DiskType{resources.LocalSSD: resources.DiskTypeSSD})
	f.MetaDB.EXPECT().DiskTypes(ctx).Return(dts, nil)
}

func mockMetaDBValidResourcesForLocalSSD(f clusterFixture, ctx context.Context, typ clusters.Type, role hosts.Role,
	resourcePresetExtID, zoneID string, returnedDiskSize int64) {
	var ffs featureflags.FeatureFlags
	var zoneIDOptional optional.String
	if zoneID != "" {
		zoneIDOptional = optional.NewString(zoneID)
	} else {
		zoneIDOptional = optional.String{}
	}
	f.MetaDB.EXPECT().ValidResources(ctx, ffs.List(), typ, role, optional.NewString(resourcePresetExtID),
		optional.NewString(resources.LocalSSD), zoneIDOptional).
		Return([]resources.Valid{{DiskSizes: []int64{returnedDiskSize}, MaxHosts: 10}}, nil)
}
