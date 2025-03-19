package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Compute) HostGroup(ctx context.Context, hostGroupID string) (compute.HostGroup, error) {
	if c.hostGroups == nil {
		return compute.HostGroup{}, semerr.InvalidInput("this installation does not support host groups")
	}
	hostGroup, err := c.hostGroups.Get(ctx, hostGroupID)
	if err != nil {
		return compute.HostGroup{}, translateComputeError(err, fmt.Sprintf("host group %q not found", hostGroupID))
	}
	return hostGroup, nil
}

func translateComputeError(err error, notFoundMessage string) error {
	if semErr := semerr.AsSemanticError(err); semErr != nil {
		if semerr.IsNotFound(err) {
			return semerr.FailedPrecondition(notFoundMessage)
		}
		// we should not transparently show errors to user
		// - Authentication, Authorization - it's problems with our creds
		// - FailedPrecondition, AlreadyExists - unexpected (in our Get calls)
		return semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticInvalidInput)
	}
	return err
}

func (c *Compute) ValidateHostGroups(ctx context.Context, hostGroupIDs []string, folderExtID string, cloudExtID string, zones []string) error {
	if len(hostGroupIDs) == 0 {
		return nil
	}

	zoneHasHostGroup := make(map[string]bool, len(zones)+len(hostGroupIDs))
	for _, zone := range zones {
		zoneHasHostGroup[zone] = false
	}

	for _, hostGroupID := range hostGroupIDs {
		hostGroup, err := c.HostGroup(ctx, hostGroupID)
		if err != nil {
			return err
		}

		if hostGroup.FolderID != folderExtID {
			resolvedFolder, err := resmanager.ResolveFolder(ctx, c.rm, folderExtID)
			if err != nil {
				return xerrors.Errorf("resolve folder: %w", err)
			}

			if resolvedFolder.CloudExtID != cloudExtID {
				return semerr.FailedPreconditionf("host group %s belongs to the cloud distinct from the cluster's cloud", hostGroupID)
			}
		}

		_, err = c.auth.Authenticate(ctx, models.PermComputeHostGroupsUse,
			cloudauth.Resource{Type: models.ResourceTypeHostGroup, ID: hostGroupID},
			cloudauth.Resource{Type: cloudauth.ResourceTypeFolder, ID: hostGroup.FolderID})
		if err != nil {
			if semErr := semerr.AsSemanticError(err); semErr != nil {
				if semerr.IsAuthorization(err) {
					return semerr.WrapWithAuthorizationf(err, "permission to use host group %s is denied", hostGroupID)
				}
				return semerr.WhitelistErrors(err, semerr.SemanticUnavailable)
			}
			return err
		}

		zoneHasHostGroup[hostGroup.ZoneID] = true
	}

	zonesWithoutHostGroup := make([]string, 0, len(zoneHasHostGroup))
	for zone, hasHostGroup := range zoneHasHostGroup {
		if !hasHostGroup {
			zonesWithoutHostGroup = append(zonesWithoutHostGroup, zone)
		}
	}
	if len(zonesWithoutHostGroup) > 0 {
		return semerr.InvalidInputf("the list of host groups must contain at least one host group"+
			" for each of the availability zones where the cluster is hosted. The provided list of host groups"+
			" does not contain host groups in the following availability zones: %+v", zonesWithoutHostGroup)
	}

	return nil
}
