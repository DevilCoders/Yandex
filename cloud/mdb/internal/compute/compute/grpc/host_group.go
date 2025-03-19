package grpc

import (
	"context"

	"github.com/golang/protobuf/ptypes"
	"google.golang.org/grpc/credentials"

	cloudCompute "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	internalCompute "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ internalCompute.HostGroupService = &HostGroupServiceClient{}

type HostGroupServiceClient struct {
	hostGroupAPI cloudCompute.HostGroupServiceClient
}

func NewHostGroupServiceClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*HostGroupServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to host group API at %q: %w", target, err)
	}

	return &HostGroupServiceClient{
		hostGroupAPI: cloudCompute.NewHostGroupServiceClient(conn),
	}, nil
}

func (c *HostGroupServiceClient) Get(ctx context.Context, hostGroupID string) (internalCompute.HostGroup, error) {
	resp, err := c.hostGroupAPI.Get(ctx, &cloudCompute.GetHostGroupRequest{
		HostGroupId: hostGroupID,
	})
	if err != nil {
		return internalCompute.HostGroup{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return hostGroupFromGRPC(resp)
}

func hostGroupFromGRPC(hostGroup *cloudCompute.HostGroup) (internalCompute.HostGroup, error) {
	resHG := internalCompute.HostGroup{
		ID:                hostGroup.GetId(),
		FolderID:          hostGroup.GetFolderId(),
		Name:              hostGroup.GetName(),
		Description:       hostGroup.GetDescription(),
		Labels:            hostGroup.GetLabels(),
		ZoneID:            hostGroup.GetZoneId(),
		Status:            hostGroup.GetStatus().String(),
		TypeID:            hostGroup.GetTypeId(),
		MaintenancePolicy: hostGroup.GetMaintenancePolicy().String(),
	}
	createdAt, err := ptypes.Timestamp(hostGroup.GetCreatedAt())
	if err != nil {
		return resHG, xerrors.Errorf("createdAt: %w", err)
	}
	resHG.CreatedAt = createdAt
	return resHG, nil
}
