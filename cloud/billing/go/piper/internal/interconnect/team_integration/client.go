package teamintegration

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/grpc"
	healthpb "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/errtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	ti "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/team/integration/v1"
)

type tiClient struct {
	conn grpc.ClientConnInterface

	folder ti.AbcServiceClient
}

func NewTIClient(ctx context.Context, conn grpc.ClientConnInterface) TIClient {
	return &tiClient{
		conn:   conn,
		folder: ti.NewAbcServiceClient(conn),
	}
}

func (c *tiClient) HealthCheck(ctx context.Context) error {
	ctx, cancel := context.WithTimeout(ctx, time.Second)
	defer cancel()

	// Check only common server aviability
	resp, err := healthpb.NewHealthClient(c.conn).Check(ctx, &healthpb.HealthCheckRequest{})
	if err != nil {
		return fmt.Errorf("grpc healthcheck: %w", err)
	}
	if status := resp.GetStatus(); status != healthpb.HealthCheckResponse_SERVING {
		return fmt.Errorf("incorrect service status %s", status.String())
	}
	return nil
}

func (c *tiClient) ResolveABC(ctx context.Context, abcID int64) (result ResolvedFolder, err error) {
	ctx = tooling.ICRequestStarted(ctx, systemName)
	defer func() {
		err = errtool.MapGRPCErr(ctx, err)
		tooling.ICRequestDone(ctx, err)
	}()

	req := &ti.ResolveRequest{
		Abc: &ti.ResolveRequest_AbcId{AbcId: abcID},
	}

	var resp *ti.ResolveResponse
	resp, err = c.folder.Resolve(ctx, req)
	if err != nil {
		return
	}
	if resp == nil {
		return
	}

	result.ID = resp.AbcId
	result.FolderID = resp.AbcFolderId
	result.CloudID = resp.CloudId
	return
}
