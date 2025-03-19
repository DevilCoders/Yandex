package provider

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	FeatureFlagAllowDecommissionedResourcePreset  = "MDB_ALLOW_DECOMMISSIONED_FLAVOR_USE"
	featureFlagAllowDecommissionedZone            = "MDB_ALLOW_DECOMMISSIONED_ZONE_USE"
	featureFlagNetworkDiskTruncate                = "MDB_NETWORK_DISK_TRUNCATE"
	featureFlagAllowNetworkSsdNonreplicatedResize = "MDB_ALLOW_NETWORK_SSD_NONREPLICATED_RESIZE"
	featureFlagLocalDiskResize                    = "MDB_LOCAL_DISK_RESIZE"
)

func (c *Clusters) validateAvailableZones(ctx context.Context, hostGroups ...clusterslogic.ResolvedHostGroup) error {
	availableZones, err := c.metaDB.ListZones(ctx)
	if err != nil {
		return err
	}
	availableZonesMap := make(map[string]struct{}, len(availableZones))
	availableZonesNames := []string{}
	for _, availableZone := range availableZones {
		availableZonesMap[availableZone.Name] = struct{}{}
		if availableZone.Name != "" && !c.cfg.ResourceValidation.IsDecommissionedZone(availableZone.Name) {
			availableZonesNames = append(availableZonesNames, availableZone.Name)
		}
	}
	availableZonesStr := strings.Join(availableZonesNames, ", ")

	for _, hg := range hostGroups {
		for _, add := range hg.HostsToAdd {
			if _, ok := availableZonesMap[add.ZoneID]; !ok {
				return semerr.InvalidInputf("invalid zone %s, valid are: %s", add.ZoneID, availableZonesStr)
			}
		}
	}

	return nil
}

func (c *Clusters) ValidateResources(ctx context.Context, session sessions.Session, typ clusters.Type, hostGroups ...clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroups, bool, error) {
	var hasChanges bool
	for i, hg := range hostGroups {
		// Host group can be utterly invalid or simply inconsistent (current* and new* are both valid and equal for example)
		if err := hg.ValidateAndFix(); err != nil {
			return clusterslogic.ResolvedHostGroups{}, false, err
		}

		hostGroups[i] = hg
		hasChanges = hasChanges || hg.HasChanges()
	}

	if err := validateDecommissionedResourcePresets(c.cfg.ResourceValidation, session, hostGroups...); err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	resolvedHostGroups, err := loadHostGroupsResourcePresets(ctx, c.metaDB, hostGroups...)
	if err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	if err := c.validateAvailableZones(ctx, resolvedHostGroups...); err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	if err := validateDecommissionedZones(c.cfg.ResourceValidation, session, resolvedHostGroups...); err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	dts, err := c.metaDB.DiskTypes(ctx)
	if err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, xerrors.Errorf("disk types not loaded: %w", err)
	}

	if err := validateHostGroups(ctx, c.cfg.ResourceValidation, c.metaDB, session, typ, dts, resolvedHostGroups...); err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	if err = validateQuota(session, dts, resolvedHostGroups...); err != nil {
		return clusterslogic.ResolvedHostGroups{}, false, err
	}

	return clusterslogic.NewResolvedHostGroups(resolvedHostGroups), hasChanges, nil
}

func validateQuota(session sessions.Session, dts resources.DiskTypes, hostGroups ...clusterslogic.ResolvedHostGroup) error {
	var quotaChange quota.Resources
	for _, hg := range hostGroups {
		if hg.IsResourcePresetChanged() {
			// Changing resource preset.

			// Deduce all current quota
			quotaChange = quotaChange.Sub(hg.CurrentResourcePreset.Resources().Mul(hg.HostsCurrent.Len()))

			// Add target quota
			quotaChange = quotaChange.Add(hg.NewResourcePreset.Resources().Mul(hg.TargetHostsCount()))
		} else {
			// We are not changing resource preset. Simply deduce and add whats due.

			if len(hg.HostsToRemove) != 0 {
				// Deduce quota for hosts to remove
				quotaChange = quotaChange.Sub(hg.TargetResourcePreset().Resources().Mul(hg.HostsToRemove.Len()))
			}

			if len(hg.HostsToAdd) != 0 {
				// Add quota for hosts to add
				quotaChange = quotaChange.Add(hg.TargetResourcePreset().Resources().Mul(hg.HostsToAdd.Len()))
			}
		}

		// Disk size change
		if hg.IsDiskSizeChanged() {
			// Changing disk size.

			var err error
			// Deduce all current disk size quota
			quotaChange, err = dts.Apply(quotaChange, -hg.HostsCurrent.Len()*hg.CurrentDiskSize.Int64, hg.DiskTypeExtID)
			if err != nil {
				return err
			}

			// Add target disk size quota
			quotaChange, err = dts.Apply(quotaChange, hg.TargetHostsCount()*hg.NewDiskSize.Int64, hg.DiskTypeExtID)
			if err != nil {
				return err
			}
		} else {
			// We are not changing disk size. Simply deduce and add whats due.

			if len(hg.HostsToRemove) != 0 {
				// Deduce disk size quota for hosts to remove
				var err error
				quotaChange, err = dts.Apply(quotaChange, -hg.HostsToRemove.Len()*hg.TargetDiskSize(), hg.DiskTypeExtID)
				if err != nil {
					return err
				}
			}

			if len(hg.HostsToAdd) != 0 {
				// Add disk size quota for hosts to add
				var err error
				quotaChange, err = dts.Apply(quotaChange, hg.HostsToAdd.Len()*hg.TargetDiskSize(), hg.DiskTypeExtID)
				if err != nil {
					return err
				}
			}
		}
	}

	// Validate quota
	session.Quota.RequestChange(quotaChange)
	return session.Quota.Validate()
}

type resourcePresetLoader interface {
	ResourcePresetByExtID(ctx context.Context, name string) (resources.Preset, error)
}

func loadHostGroupsResourcePresets(ctx context.Context, loader resourcePresetLoader, hostGroups ...clusterslogic.HostGroup) ([]clusterslogic.ResolvedHostGroup, error) {
	res := make([]clusterslogic.ResolvedHostGroup, 0, len(hostGroups))
	for _, hg := range hostGroups {
		resolvedHostGroup, err := loadHostGroupResourcePresets(ctx, loader, hg)
		if err != nil {
			return nil, err
		}

		res = append(res, resolvedHostGroup)
	}

	return res, nil
}

func loadHostGroupResourcePresets(ctx context.Context, loader resourcePresetLoader, hg clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroup, error) {
	var err error
	var currentResourcePreset resources.Preset
	if hg.CurrentResourcePresetExtID.Valid {
		currentResourcePreset, err = loader.ResourcePresetByExtID(ctx, hg.CurrentResourcePresetExtID.String)
		if err != nil {
			if xerrors.Is(err, sqlerrors.ErrNotFound) {
				// Something fishy is going on - current resource preset not found.
				// Do not return semantic error.
				return clusterslogic.ResolvedHostGroup{}, xerrors.Errorf("current resource preset %q is not available: %w", hg.CurrentResourcePresetExtID.String, err)
			}

			return clusterslogic.ResolvedHostGroup{}, xerrors.Errorf("resource preset %q load: %w", hg.CurrentResourcePresetExtID.String, err)
		}
	}

	var newResourcePreset resources.Preset
	if hg.NewResourcePresetExtID.Valid {
		newResourcePreset, err = loader.ResourcePresetByExtID(ctx, hg.NewResourcePresetExtID.String)
		if err != nil {
			if xerrors.Is(err, sqlerrors.ErrNotFound) {
				return clusterslogic.ResolvedHostGroup{}, semerr.WrapWithInvalidInputf(err, "resource preset %q is not available", hg.TargetResourcePresetExtID())
			}

			return clusterslogic.ResolvedHostGroup{}, xerrors.Errorf("resource preset %q load: %w", hg.NewResourcePresetExtID.String, err)
		}
	}

	return clusterslogic.ResolvedHostGroup{HostGroup: hg, CurrentResourcePreset: currentResourcePreset, NewResourcePreset: newResourcePreset}, nil
}

func validateDecommissionedResourcePresets(cfg logic.ResourceValidationConfig, session sessions.Session, hostGroups ...clusterslogic.HostGroup) error {
	if session.FeatureFlags.Has(FeatureFlagAllowDecommissionedResourcePreset) {
		return nil
	}

	for _, hg := range hostGroups {
		var presetToCheck string

		if hg.NewResourcePresetExtID.Valid {
			// Check new preset
			presetToCheck = hg.NewResourcePresetExtID.String
		} else if len(hg.HostsToAdd) != 0 {
			// If new preset is not set and we add hosts, check current preset
			presetToCheck = hg.CurrentResourcePresetExtID.String
		}

		if presetToCheck == "" {
			continue
		}

		if cfg.IsDecommissionedResourcePresets(presetToCheck) {
			return semerr.InvalidInputf("resource preset %q is decommissioned", presetToCheck)
		}
	}

	return nil
}

func validateDecommissionedZones(cfg logic.ResourceValidationConfig, session sessions.Session, hostGroups ...clusterslogic.ResolvedHostGroup) error {
	if session.FeatureFlags.Has(featureFlagAllowDecommissionedZone) {
		return nil
	}

	for _, hg := range hostGroups {
		if hg.SkipValidations.DecommissionedZone {
			continue
		}
		for _, add := range hg.HostsToAdd {
			if cfg.IsDecommissionedZone(add.ZoneID) {
				return semerr.InvalidInputf("no new resources can be created in zone %q", add.ZoneID)
			}
		}

		if hg.IsUpscale() {
			for _, current := range hg.HostsCurrent {
				if cfg.IsDecommissionedZone(current.ZoneID) {
					return semerr.InvalidInputf("no resources can be upscaled in zone %q", current.ZoneID)
				}
			}
		}
	}

	return nil
}

type validResourcesLoader interface {
	ValidResources(ctx context.Context, featureFlags []string, typ clusters.Type, role hosts.Role, resourcePresetExtID, diskTypeExtID, zoneID optional.String) ([]resources.Valid, error)
}

func validateHostGroups(ctx context.Context, cfg logic.ResourceValidationConfig, loader validResourcesLoader, session sessions.Session, typ clusters.Type, dts resources.DiskTypes, hostGroups ...clusterslogic.ResolvedHostGroup) error {
	for _, hg := range hostGroups {
		if err := validateHostGroup(ctx, cfg, loader, session, typ, dts, hg); err != nil {
			return err
		}
	}

	return nil
}

func validateHostGroup(ctx context.Context, cfg logic.ResourceValidationConfig, loader validResourcesLoader, session sessions.Session, typ clusters.Type, dts resources.DiskTypes, hg clusterslogic.ResolvedHostGroup) error {
	// Validate disk type if we are adding new hosts
	if len(hg.HostsToAdd) != 0 {
		if err := dts.Validate(hg.DiskTypeExtID); err != nil {
			return err
		}
	}

	if hg.CurrentResourcePreset.VType == environment.VTypeCompute {
		if err := validateComputeResourceChanges(session.FeatureFlags, dts, hg.HostGroup, typ); err != nil {
			return err
		}
	}

	if hg.CurrentResourcePreset.VType == environment.VTypeAWS {
		if err := validateAwsResourceChanges(hg.HostGroup); err != nil {
			return err
		}
	}

	if err := validateHostsCount(ctx, loader, session.FeatureFlags, typ, hg); err != nil {
		return err
	}

	if err := validateHostsDiskSize(ctx, cfg, loader, session.FeatureFlags, typ, hg.HostGroup); err != nil {
		return err
	}

	return nil
}

func validateComputeResourceChanges(featureFlags featureflags.FeatureFlags, dts resources.DiskTypes, hg clusterslogic.HostGroup, typ clusters.Type) error {
	if hg.IsDiskSizeChanged() {
		if dts.IsNetworkDisk(hg.DiskTypeExtID) && hg.NewDiskSize.Int64 < hg.CurrentDiskSize.Int64 {
			if typ == clusters.TypeSQLServer {
				return semerr.Authorization("requested feature is not available")
			}
			if err := featureFlags.EnsurePresent(featureFlagNetworkDiskTruncate); err != nil {
				return err
			}
		}

		if hg.DiskTypeExtID == resources.NetworkSSDNonreplicated {
			if err := featureFlags.EnsurePresent(featureFlagAllowNetworkSsdNonreplicatedResize); err != nil {
				return err
			}
		}
	}

	if dts.IsLocalDisk(hg.DiskTypeExtID) && (hg.IsResourcePresetChanged() || hg.IsDiskSizeChanged()) {
		if err := featureFlags.EnsurePresent(featureFlagLocalDiskResize); err != nil {
			return err
		}
	}

	return nil
}

func validateAwsResourceChanges(hg clusterslogic.HostGroup) error {
	if hg.IsDiskSizeChanged() {
		if hg.NewDiskSize.Int64 < hg.CurrentDiskSize.Int64 {
			return semerr.InvalidInput("unable to decrease disk size")
		}
	}

	return nil
}

// validateHostsDiskSize in specific zone. All if resource preset or disk size is changed. Only the ones being added otherwise.
func validateHostsDiskSize(ctx context.Context, cfg logic.ResourceValidationConfig, loader validResourcesLoader, ffs featureflags.FeatureFlags, typ clusters.Type, hg clusterslogic.HostGroup) error {
	if hg.SkipValidations.DiskSize {
		return nil
	}
	zoneHosts := make([]clusterslogic.ZoneHosts, 0, hg.TargetHostsCount())
	if hg.NewResourcePresetExtID.Valid || hg.IsDiskSizeChanged() {
		for _, zh := range hg.HostsCurrent {
			zoneHosts = append(zoneHosts, zh)
		}
	}
	for _, zh := range hg.HostsToAdd {
		zoneHosts = append(zoneHosts, zh)
	}
	for _, zh := range zoneHosts {
		if err := validateHostDiskSize(ctx, cfg, loader, ffs, typ, hg.Role, hg.TargetResourcePresetExtID(), hg.DiskTypeExtID, zh.ZoneID, hg.TargetDiskSize()); err != nil {
			return err
		}
	}

	return nil
}

func validateHostDiskSize(ctx context.Context, cfg logic.ResourceValidationConfig, loader validResourcesLoader, ffs featureflags.FeatureFlags, typ clusters.Type, role hosts.Role, resourcePresetExtID, diskTypeExtID, zoneID string, diskSize int64) error {
	vrs, err := loader.ValidResources(
		ctx, ffs.List(), typ, role,
		optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.NewString(zoneID),
	)
	if err != nil {
		return xerrors.Errorf("load valid resources: %w", err)
	}

	if len(vrs) == 0 {
		return semerr.InvalidInputf("resource preset %q and disk type %q are not available in zone %q", resourcePresetExtID, diskTypeExtID, zoneID)
	}

	if len(vrs) > 1 {
		return xerrors.Errorf("more than one valid resources combination: %+v", vrs)
	}

	vr := vrs[0]

	if err = vr.Validate(); err != nil {
		return err
	}

	if vr.DiskSizeRange.Valid {
		if !vr.DiskSizeRange.IntervalInt64.Includes(diskSize) {
			return semerr.InvalidInputf("invalid disk size, must be inside %s range", vr.DiskSizeRange.IntervalInt64)
		}

		if diskSize%cfg.MinimalDiskUnit != 0 {
			return semerr.InvalidInputf("invalid disk size, must be multiple of %d bytes", cfg.MinimalDiskUnit)
		}
	}
	if len(vr.DiskSizes) > 0 {
		var found bool
		for _, ds := range vr.DiskSizes {
			if diskSize == ds {
				found = true
				break
			}
		}

		if !found {
			return semerr.InvalidInputf("invalid disk size, it must be one of %v", vr.DiskSizes)
		}
	}

	return nil
}

func validateHostsCount(ctx context.Context, loader validResourcesLoader, ffs featureflags.FeatureFlags, typ clusters.Type, rhg clusterslogic.ResolvedHostGroup) error {
	hg := rhg.HostGroup
	var (
		role                = hg.Role
		resourcePresetExtID = hg.TargetResourcePresetExtID()
		diskTypeExtID       = hg.DiskTypeExtID
		count               = hg.TargetHostsCount()
	)
	vrs, err := loader.ValidResources(
		ctx, ffs.List(), typ, role,
		optional.NewString(resourcePresetExtID), optional.NewString(diskTypeExtID), optional.String{},
	)
	if err != nil {
		return xerrors.Errorf("load valid resources: %w", err)
	}

	if len(vrs) == 0 {
		return semerr.InvalidInputf("resource preset %q and disk type %q are not available", resourcePresetExtID, diskTypeExtID)
	}

	if err = validateHostCountLimitsUniqueness(vrs); err != nil {
		return err
	}

	for _, vr := range vrs {
		if err = vr.Validate(); err != nil {
			return err
		}

		lessThanMin := count < vr.MinHosts
		if typ == clusters.TypeRedis && rhg.TargetResourcePreset().VType == environment.VTypeCompute && diskTypeExtID == resources.LocalSSD {
			lessThanMin = count < 2
		}

		if lessThanMin && count != 0 { // Allow shard/subcluster deletion
			return semerr.InvalidInput(formatHostCountError(typ, role, resourcePresetExtID, diskTypeExtID, vr.MinHosts, false))
		}

		if !hg.SkipValidations.MaxHosts && count > vr.MaxHosts {
			return semerr.InvalidInput(formatHostCountError(typ, role, resourcePresetExtID, diskTypeExtID, vr.MaxHosts, true))
		}
	}

	return nil
}

func formatHostCountError(typ clusters.Type, role hosts.Role, resourcePresetExtID, diskTypeExtID string, neededCount int64, invalidMaximum bool) string {
	entity := entityForHostCountError(typ, role)

	hostsPlural := "host"
	if neededCount > 1 {
		hostsPlural = "hosts"
	}

	action := "requires at least"
	if invalidMaximum {
		action = "allows at most"
	}

	return fmt.Sprintf(
		"%s %s with resource preset %q and disk type %q %s %d %s",
		entity,
		role,
		resourcePresetExtID,
		diskTypeExtID,
		action,
		neededCount,
		hostsPlural,
	)
}

func entityForHostCountError(typ clusters.Type, role hosts.Role) string {
	entity := "cluster"
	if typ == clusters.TypeClickHouse || typ == clusters.TypeMongoDB || typ == clusters.TypeRedis {
		entity = "shard"
		if role == hosts.RoleClickHouse || role == hosts.RoleMongoD {
			entity = "subcluster"
		}
	}

	return entity
}

func validateHostCountLimitsUniqueness(vrs []resources.Valid) error {
	type MinMax struct {
		Min int64
		Max int64
	}

	unique := make(map[MinMax]struct{}, 1)
	for _, vr := range vrs {
		unique[MinMax{Min: vr.MinHosts, Max: vr.MaxHosts}] = struct{}{}
	}

	if len(unique) > 1 {
		// This should never happen
		return xerrors.Errorf("different host count limits for different geos: unique %+v, valid resources %+v", unique, vrs)
	}

	return nil
}

func (c *Clusters) RegionByName(ctx context.Context, region string) (environment.Region, error) {
	return c.metaDB.RegionByName(ctx, region)
}

func (c *Clusters) ListAvailableZones(ctx context.Context, session sessions.Session, forceFilterDecommissionedZone bool) ([]environment.Zone, error) {
	zones, err := c.metaDB.ListZones(ctx)
	if err != nil {
		return nil, err
	}

	if session.FeatureFlags.Has(featureFlagAllowDecommissionedZone) && !forceFilterDecommissionedZone {
		return zones, nil
	}

	result := make([]environment.Zone, 0, len(zones))
	for _, zone := range zones {
		if !c.cfg.ResourceValidation.IsDecommissionedZone(zone.Name) {
			result = append(result, zone)
		}
	}

	return result, nil
}

func (c *Clusters) ListAvailableZonesForCloudAndRegion(ctx context.Context, session sessions.Session,
	cloudType environment.CloudType, regionID string, forceFilterDecommissionedZone bool) ([]string, error) {
	availableZones, err := c.ListAvailableZones(ctx, session, forceFilterDecommissionedZone)
	if err != nil {
		return nil, err
	}

	availableZonesNames := make([]string, 0, len(availableZones))
	for _, zone := range availableZones {
		if zone.CloudType == cloudType && zone.RegionID == regionID {
			availableZonesNames = append(availableZonesNames, zone.Name)
		}
	}

	return availableZonesNames, nil
}

func (c *Clusters) SelectZonesForCloudAndRegion(ctx context.Context, session sessions.Session,
	cloudType environment.CloudType, regionID string, forceFilterDecommissionedZone bool, zoneCount int) ([]string, error) {
	availableZones, err := c.ListAvailableZonesForCloudAndRegion(ctx, session, cloudType, regionID, forceFilterDecommissionedZone)
	if err != nil {
		return nil, err
	}

	if len(availableZones) == 0 {
		return nil, semerr.NotFoundf("no available zones in region %q", regionID)
	}

	if len(availableZones) < zoneCount {
		return nil, semerr.NotFoundf("not found enough available zones (was needed %d but found %d) for region %q",
			zoneCount, len(availableZones), regionID)
	}

	return availableZones[0:zoneCount], nil
}

func (c *Clusters) ResourcePresetByCPU(ctx context.Context, clusterType clusters.Type, role hosts.Role, flavorType optional.String,
	generation optional.Int64, minCPU float64, zones, featureFlags []string) (resources.DefaultPreset, error) {
	return c.metaDB.GetResourcePreset(
		ctx,
		clusterType,
		role,
		flavorType,
		optional.String{},
		optional.String{},
		generation,
		optional.NewFloat64(minCPU),
		zones,
		featureFlags,
		c.cfg.ResourceValidation.DecommissionedResourcePresets,
	)
}

func (c *Clusters) ResourcePresetFromDefaultConfig(clusterType clusters.Type, role hosts.Role) (resources.DefaultPreset, error) {
	configResources, err := c.cfg.GetDefaultResources(clusterType, role)
	if err != nil {
		return resources.DefaultPreset{}, err
	}

	return resources.DefaultPreset{
		DiskTypeExtID: configResources.DiskTypeExtID,
		DiskSizes:     []int64{configResources.DiskSize},
		ExtID:         configResources.ResourcePresetExtID,
	}, nil
}

func (c *Clusters) DiskTypeExtIDByResourcePreset(ctx context.Context, clusterType clusters.Type, role hosts.Role, resourcePreset string,
	zones []string, featureFlags []string) (string, error) {
	return c.metaDB.DiskTypeExtIDByResourcePreset(ctx, clusterType, role, resourcePreset, zones, featureFlags)
}

func (c *Clusters) SetDefaultVersionCluster(ctx context.Context, cid string,
	ctype clusters.Type, env environment.SaltEnv, majorVersion string, edition string, revision int64) error {
	return c.metaDB.SetDefaultVersionCluster(ctx, cid, ctype, env, majorVersion, edition, revision)
}
