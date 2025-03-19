package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/network/porto"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

const maxSecurityGroups = 3

func translateNetworkErrors(err error, notFoundMessage string) error {
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

func (c *Compute) Network(ctx context.Context, networkID string) (networkProvider.Network, error) {
	network, err := c.network.GetNetwork(ctx, networkID)
	if err != nil {
		return networkProvider.Network{}, translateNetworkErrors(err, fmt.Sprintf("network %q not found", networkID))
	}
	return network, nil
}

func (c *Compute) Networks(ctx context.Context, projectID string, regionID string) ([]networkProvider.Network, error) {
	return c.network.GetNetworks(ctx, projectID, regionID)
}

func (c *Compute) CreateDefaultNetwork(ctx context.Context, projectID string, region string) (networkProvider.Network, error) {
	return c.network.CreateDefaultNetwork(ctx, projectID, region)
}

func (c *Compute) Subnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	subnet, err := c.network.GetSubnet(ctx, subnetID)
	if err != nil {
		return networkProvider.Subnet{}, translateNetworkErrors(err, fmt.Sprintf("subnet %q not found", subnetID))
	}
	return subnet, nil
}

func (c *Compute) Subnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	subnets, err := c.network.GetSubnets(ctx, network)
	if err != nil {
		return nil, translateNetworkErrors(err, fmt.Sprintf("network %q not found", network.ID))
	}
	return subnets, nil
}

func (c *Compute) NetworkAndSubnets(ctx context.Context, networkID string) (networkProvider.Network, []networkProvider.Subnet, error) {
	network, err := c.Network(ctx, networkID)
	if err != nil {
		return networkProvider.Network{}, nil, err
	}

	var subnets []networkProvider.Subnet
	// in the porto case network may be blank
	if network.ID != "" {
		// TODO: validate that network cloud correspond to the request cloud
		subnets, err = c.Subnets(ctx, network)
		if err != nil {
			return networkProvider.Network{}, nil, err
		}
	}
	return network, subnets, nil
}

func (c *Compute) PickSubnet(ctx context.Context, subnets []networkProvider.Subnet, vtype environment.VType, geo string, publicIP bool, requestedSubnetID optional.String, clusterFolderExtID string) (networkProvider.Subnet, error) {
	var geoSubnets []networkProvider.Subnet
	for _, sn := range subnets {
		if sn.ZoneID == geo {
			geoSubnets = append(geoSubnets, sn)
		}
	}
	if requestedSubnetID.Valid {
		req := requestedSubnetID.String
		for _, sn := range geoSubnets {
			if sn.ID == req {
				err := c.checkNetPermissions(ctx, sn.FolderID, publicIP)
				if err != nil {
					return networkProvider.Subnet{}, err
				}
				return sn, nil
			}
		}
		return networkProvider.Subnet{}, semerr.FailedPreconditionf("subnet %q not found", req)
	}

	if vtype == environment.VTypeCompute {
		if len(geoSubnets) < 1 {
			return networkProvider.Subnet{}, semerr.FailedPreconditionf("no subnets in geo %q", geo)
		}

		if len(geoSubnets) > 1 {
			return networkProvider.Subnet{}, semerr.FailedPreconditionf("geo %q has multiple subnets, need to specify one", geo)
		}
		targetSn := geoSubnets[0]

		err := c.checkNetPermissions(ctx, targetSn.FolderID, publicIP)
		if err != nil {
			return networkProvider.Subnet{}, err
		}
		return targetSn, nil
	}

	if vtype == environment.VTypePorto || vtype == environment.VTypeAWS {
		if len(geoSubnets) < 1 {
			return networkProvider.Subnet{}, nil
		}

		if len(geoSubnets) > 1 {
			return networkProvider.Subnet{}, semerr.FailedPreconditionf("geo %q has multiple subnets, need to specify one", geo)
		}

		targetSn := geoSubnets[0]

		if targetSn.NetworkID == porto.NetworkPgaasInternalNets {
			return targetSn, nil
		}

		if targetSn.FolderID == clusterFolderExtID {
			return targetSn, nil
		} else {
			return networkProvider.Subnet{}, semerr.Authorization("cluster does not have rights to the specified network")
		}
	}

	return networkProvider.Subnet{}, nil
}

func (c *Compute) CheckPublicIPPermissions(ctx context.Context, folderID string, publicIP bool) error {
	if publicIP {
		_, err := c.auth.Authenticate(ctx, models.PermVPCAddressesCreateExternal, cloudauth.Resource{
			ID:   folderID,
			Type: cloudauth.ResourceTypeFolder,
		})
		if err != nil {
			return err
		}
	}
	return nil
}

func (c *Compute) checkNetPermissions(ctx context.Context, folderID string, publicIP bool) error {
	_, err := c.auth.Authenticate(ctx, models.PermVPCSubnetsUse, cloudauth.Resource{
		ID:   folderID,
		Type: cloudauth.ResourceTypeFolder,
	})
	if err != nil {
		return err
	}
	err = c.CheckPublicIPPermissions(ctx, folderID, publicIP)
	return err
}

func (c *Compute) ValidateSecurityGroups(ctx context.Context, securityGroupIDs []string, networkID string) error {
	if len(securityGroupIDs) > maxSecurityGroups {
		return semerr.InvalidInputf("too many security groups (%d is the maximum)", maxSecurityGroups)
	}
	for _, sgID := range securityGroupIDs {
		sg, err := c.vpc.GetSecurityGroup(ctx, sgID)
		if err != nil {
			return translateNetworkErrors(err, fmt.Sprintf("security group %q not found", sgID))
		}
		if sg.NetworkID != networkID {
			return semerr.FailedPreconditionf(
				"network id %q and security group %q network id %q are not equal",
				networkID,
				sgID,
				sg.NetworkID)
		}
	}
	return nil
}
