package porto

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	zoneIDs = []string{"sas", "vla", "myt", "man", "iva"}

	ErrOwnerServiceForMacroNotSpecified = xerrors.New("owner service for macro not specified")
	ErrMacroIDIsTooLong                 = xerrors.New("macro ID is longer than 8 symbols")
)

const NetworkPgaasInternalNets = "_PGAASINTERNALNETS_"

type Client struct {
	l log.Logger

	rt  racktables.Client
	abc iam.AbcService
}

var _ networkProvider.Client = &Client{}

func NewClient(rt racktables.Client, abc iam.AbcService, l log.Logger) *Client {
	return &Client{
		l:   l,
		rt:  rt,
		abc: abc,
	}
}

func (c *Client) GetSubnets(ctx context.Context, network networkProvider.Network) ([]networkProvider.Subnet, error) {
	macro, err := c.rt.GetMacro(ctx, network.ID)
	if err != nil {
		// TODO: temporary workaround, remove later
		if semerr.AsSemanticError(err).Semantic == semerr.SemanticNotFound {
			c.l.Warnf("Bad network_id: %s", network.ID)
			return nil, nil
		}

		return nil, xerrors.Errorf("getting macro %q from racktables: %w", network.ID, err)
	}

	formattedProjectID, err := getFormattedProjectID(macro.IDs[0].ID)
	if err != nil {
		return nil, xerrors.Errorf("getting project_id from macro id %q: %w", macro.IDs[0].ID, err)
	}

	subnets := make([]networkProvider.Subnet, 0, len(zoneIDs))
	for _, zoneID := range zoneIDs {
		subnet := networkProvider.Subnet{
			ID:          formattedProjectID,
			FolderID:    network.FolderID,
			CreatedAt:   network.CreatedAt,
			Name:        network.Name,
			Description: network.Description,
			Labels:      network.Labels,
			NetworkID:   network.ID,
			ZoneID:      zoneID,
		}
		subnets = append(subnets, subnet)
	}
	return subnets, nil
}

func (c *Client) GetNetwork(ctx context.Context, networkID string) (networkProvider.Network, error) {
	if networkID == "" || networkID == "0" {
		return networkProvider.Network{}, nil
	}

	macro, err := c.rt.GetMacro(ctx, networkID)
	if err != nil {
		// TODO: temporary workaround, remove later
		if semerr.AsSemanticError(err).Semantic == semerr.SemanticNotFound {
			c.l.Warnf("Bad network_id: %s", networkID)
			return networkProvider.Network{}, nil
		}

		return networkProvider.Network{}, xerrors.Errorf("getting macro %q from racktables: %w", networkID, err)
	}

	if macro.OwnerService == "" {
		return networkProvider.Network{}, xerrors.Errorf("macro %q: %w", macro.Name, ErrOwnerServiceForMacroNotSpecified)
	}

	abcSlug := strings.TrimPrefix(macro.OwnerService, "svc_")

	abc, err := c.abc.ResolveByABCSlug(ctx, abcSlug)
	if err != nil {
		return networkProvider.Network{}, err
	}

	return networkProvider.Network{
		ID:          networkID,
		FolderID:    abc.DefaultFolderID,
		Name:        macro.Name,
		Description: macro.Description,
	}, nil
}

func (c *Client) GetNetworks(ctx context.Context, projectID, regionID string) ([]networkProvider.Network, error) {
	return nil, nil
}

func (c *Client) CreateDefaultNetwork(ctx context.Context, projectID, regionID string) (networkProvider.Network, error) {
	return networkProvider.Network{}, xerrors.Errorf("CreateDefaultNetwork unimplemented in porto")
}

func (c *Client) GetSubnet(ctx context.Context, subnetID string) (networkProvider.Subnet, error) {
	return networkProvider.Subnet{}, nil
}

func (c *Client) GetNetworksByCloudID(ctx context.Context, cloudID string) ([]string, error) {
	macrosWithOwners, err := c.rt.GetMacrosWithOwners(ctx)
	if err != nil {
		return nil, xerrors.Errorf("getting macros with owners from racktables: %w", err)
	}

	abc, err := c.abc.ResolveByCloudID(ctx, cloudID)
	if err != nil {
		semErr := semerr.AsSemanticError(err)
		if semErr != nil && semErr.Semantic != semerr.SemanticUnknown {
			return nil, err
		}
		return nil, xerrors.Errorf("resolve ABC by CloudID: %w", err)
	}

	matchedNetworks := GetNetworksWithMatchedOwners(macrosWithOwners, abc.AbcSlug)

	networks := []string{NetworkPgaasInternalNets}

	for _, matchedNetwork := range matchedNetworks {
		if matchedNetwork == NetworkPgaasInternalNets {
			continue
		}

		network, err := c.GetNetwork(ctx, matchedNetwork)
		if err != nil && !xerrors.Is(err, ErrOwnerServiceForMacroNotSpecified) {
			return nil, xerrors.Errorf("get network %q: %w", matchedNetwork, err)
		}

		if network.FolderID == abc.DefaultFolderID {
			networks = append(networks, network.Name)
		}
	}

	return networks, nil
}

func getFormattedProjectID(macroID string) (string, error) {
	l := len(macroID)
	if l > 8 {
		return "", ErrMacroIDIsTooLong
	}
	if l > 4 {
		return fmt.Sprintf("%s:%s", macroID[:l-4], macroID[l-4:]), nil
	}
	return fmt.Sprintf("0:%s", macroID), nil
}
