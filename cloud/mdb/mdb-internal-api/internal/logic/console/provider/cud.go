package provider

import (
	"context"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Console) GetPlatforms(_ context.Context) ([]consolemodels.Platform, error) {
	// We don't check auth in this method
	return c.cfg.PlatformsInfo, nil
}

func (c *Console) GetConnectionDomain(_ context.Context) (string, error) {
	// We don't check auth in this method
	return c.cfg.GetDomain(c.cfg.EnvironmentVType)
}

func (c *Console) GetUsedResources(ctx context.Context, clouds []string) ([]consolemodels.UsedResources, error) {
	if len(clouds) == 0 {
		return nil, semerr.InvalidInputf("%ss list should not be empty", c.cfg.ResourceModel.Cloud())
	}

	// We don't use PreHandle here since we need to check auth for several clouds
	for i := range clouds {
		// TODO: use single call with all clouds? check with IAM people
		_, err := c.auth.Authenticate(ctx, models.PermMDBAllRead, as.ResourceCloud(clouds[i]))
		if err != nil {
			return nil, err
		}
	}

	ctx, err := c.metaDB.Begin(ctx, sqlutil.Alive)
	if err != nil {
		return nil, err
	}
	defer func() { _ = c.metaDB.Rollback(ctx) }()

	usedResources, err := c.metaDB.GetUsedResources(ctx, clouds)
	if err != nil {
		if xerrors.Is(err, sqlerrors.ErrNotFound) {
			return nil, semerr.NotFoundf("used resources for specified %ss not found", c.cfg.ResourceModel.Cloud())
		}
		return nil, err
	}

	return usedResources, nil
}

func (c *Console) setResourcePresetExtraFields(r *consolemodels.ResourcePreset) error {
	if c.cfg.ResourceValidation.IsDecommissionedResourcePresets(r.ExtID) {
		r.Decommissioned = true
	}
	if c.cfg.ResourceValidation.IsDecommissionedZone(r.Zone) {
		r.Decommissioned = true
	}
	generationName, err := c.cfg.GetGenerationName(r.Generation)
	if err != nil {
		return err
	}
	r.GenerationName = generationName
	return nil
}

func (c *Console) GetResourcePresetsByClusterType(ctx context.Context, clusterType clusters.Type, folderID string, withDecomissioned bool) ([]consolemodels.ResourcePreset, error) {
	var err error
	var featureFlags []string
	if folderID != "" { // feature flag aware
		var session sessions.Session
		ctx, session, err = c.sessions.Begin(ctx, sessions.ResolveByFolder(folderID, models.PermMDBAllRead), sessions.WithPreferStandby())
		if err != nil {
			return nil, err
		}
		defer func() { c.sessions.Rollback(ctx) }()
		featureFlags = session.FeatureFlags.List()
	} else { // feature flag unaware
		ctx, err = c.metaDB.Begin(ctx, sqlutil.Alive)
		if err != nil {
			return nil, err
		}
		defer func() { _ = c.metaDB.Rollback(ctx) }()
		featureFlags = []string{}
	}

	resourcePresets, err := c.metaDB.GetResourcePresetsByClusterType(ctx, clusterType, featureFlags)
	if err != nil {
		return nil, err
	}

	result := make([]consolemodels.ResourcePreset, 0, len(resourcePresets))
	for _, resourcePreset := range resourcePresets {
		err := c.setResourcePresetExtraFields(&resourcePreset)
		if err != nil {
			return nil, err
		}
		if resourcePreset.Decommissioned && !withDecomissioned {
			continue
		}
		result = append(result, resourcePreset)
	}

	return result, nil
}

func (c *Console) GetResourcePresetsByCloudRegion(ctx context.Context, cloudType environment.CloudType, region string, clusterType clusters.Type, projectID string) ([]consolemodels.ResourcePreset, error) {
	var err error
	var featureFlags []string
	if projectID != "" { // feature flag aware
		var session sessions.Session
		ctx, session, err = c.sessions.Begin(ctx, sessions.ResolveByFolder(projectID, models.PermMDBAllRead), sessions.WithPreferStandby())
		if err != nil {
			return nil, err
		}
		defer func() { c.sessions.Rollback(ctx) }()
		featureFlags = session.FeatureFlags.List()
	} else { // feature flag unaware
		ctx, err = c.metaDB.Begin(ctx, sqlutil.Alive)
		if err != nil {
			return nil, err
		}
		defer func() { _ = c.metaDB.Rollback(ctx) }()
		featureFlags = []string{}
	}

	resourcePresets, err := c.metaDB.GetResourcePresetsByCloudRegion(ctx, cloudType, region, clusterType, featureFlags)
	if err != nil {
		return nil, err
	}

	result := make([]consolemodels.ResourcePreset, 0, len(resourcePresets))
	for _, resourcePreset := range resourcePresets {
		err := c.setResourcePresetExtraFields(&resourcePreset)
		if err != nil {
			return nil, err
		}
		result = append(result, resourcePreset)
	}

	return result, nil
}

func (c *Console) GetDefaultResourcesByClusterType(clusterType clusters.Type, role hosts.Role) (consolemodels.DefaultResources, error) {
	defaultResources, err := c.cfg.GetDefaultResources(clusterType, role)
	if err != nil {
		return consolemodels.DefaultResources{}, err
	}
	generationName, err := c.cfg.GetGenerationName(defaultResources.Generation)
	if err != nil {
		return consolemodels.DefaultResources{}, err
	}
	return consolemodels.DefaultResources{
		ResourcePresetExtID: defaultResources.ResourcePresetExtID,
		DiskTypeExtID:       defaultResources.DiskTypeExtID,
		DiskSize:            defaultResources.DiskSize,
		Generation:          defaultResources.Generation,
		GenerationName:      generationName,
	}, nil
}

func (c *Console) InitResourcesIfEmpty(resources *models.ClusterResources, role hosts.Role, clusterType clusters.Type) error {
	if *resources != (models.ClusterResources{}) {
		return nil
	}
	defaultResources, err := c.GetDefaultResourcesByClusterType(clusterType, role)
	if err != nil {
		return xerrors.Errorf("failed to get default resources for %s host with role %s: %w", clusterType.String(), role.String(), err)
	}
	resources.ResourcePresetExtID = defaultResources.ResourcePresetExtID
	resources.DiskTypeExtID = defaultResources.DiskTypeExtID
	resources.DiskSize = defaultResources.DiskSize
	return nil
}

func (c *Console) GetDefaultVersions(ctx context.Context, clusterType clusters.Type, env environment.SaltEnv, component string) ([]consolemodels.DefaultVersion, error) {
	var err error
	ctx, err = c.metaDB.Begin(ctx, sqlutil.Alive)
	if err != nil {
		return nil, err
	}
	defer func() { _ = c.metaDB.Rollback(ctx) }()

	versions, err := c.metaDB.GetDefaultVersions(ctx, clusterType, env, component)
	if err != nil {
		return nil, err
	}
	return versions, err
}

func (c *Console) ResourcePresetByExtID(ctx context.Context, extID string) (resources.Preset, error) {
	return c.metaDB.ResourcePresetByExtID(ctx, extID)
}

func (c *Console) GetFeatureFlags(ctx context.Context, folderID string) (featureflags.FeatureFlags, error) {
	var err error
	var session sessions.Session
	if folderID != "" {
		ctx, session, err = c.sessions.Begin(ctx, sessions.ResolveByFolder(folderID, models.PermMDBAllRead), sessions.WithPreferStandby())
		if err != nil {
			return featureflags.FeatureFlags{}, err
		}
		defer func() { c.sessions.Rollback(ctx) }()
		return session.FeatureFlags, nil
	}
	return featureflags.FeatureFlags{}, nil
}

func (c *Console) GetCloudsByClusterType(ctx context.Context, clusterType clusters.Type, pageSize, offset int64) ([]consolemodels.Cloud, int64, error) {
	var err error
	ctx, err = c.metaDB.Begin(ctx, sqlutil.Alive)
	if err != nil {
		return nil, 0, err
	}
	defer func() { _ = c.metaDB.Rollback(ctx) }()

	return c.metaDB.GetCloudsByClusterType(ctx, clusterType, pageSize, offset)
}
