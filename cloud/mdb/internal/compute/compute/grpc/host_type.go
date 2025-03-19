package grpc

import (
	"context"

	"google.golang.org/grpc/credentials"

	cloudCompute "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	internalCompute "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ internalCompute.HostTypeService = &HostTypeServiceClient{}

type HostTypeServiceClient struct {
	hostTypeAPI cloudCompute.HostTypeServiceClient
}

func NewHostTypeServiceClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*HostTypeServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to host type API at %q: %w", target, err)
	}

	return &HostTypeServiceClient{
		hostTypeAPI: cloudCompute.NewHostTypeServiceClient(conn),
	}, nil
}

func (c *HostTypeServiceClient) Get(ctx context.Context, hostTypeID string) (internalCompute.HostType, error) {
	resp, err := c.hostTypeAPI.Get(ctx, &cloudCompute.GetHostTypeRequest{
		HostTypeId: hostTypeID,
	})
	if err != nil {
		return internalCompute.HostType{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	return hostTypeFromGRPC(resp)
}

func hostTypeFromGRPC(hostType *cloudCompute.HostType) (internalCompute.HostType, error) {
	resHT := internalCompute.HostType{
		ID:       hostType.GetId(),
		Cores:    hostType.GetCores(),
		Memory:   hostType.GetMemory(),
		Disks:    hostType.GetDisks(),
		DiskSize: hostType.GetDiskSize(),
	}
	return resHT, nil
}
