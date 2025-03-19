package iam

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/grpc"
	healthpb "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/errtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	rm "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
)

type rmClient struct {
	conn grpc.ClientConnInterface

	folder rm.FolderServiceClient
}

func NewRMClient(ctx context.Context, conn grpc.ClientConnInterface) RMClient {
	return &rmClient{
		conn:   conn,
		folder: rm.NewFolderServiceClient(conn),
	}
}

func (c *rmClient) HealthCheck(ctx context.Context) error {
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

func (c *rmClient) ResolveFolder(ctx context.Context, folderIDs ...string) (result []ResolvedFolder, err error) {
	ctx = tooling.ICRequestStarted(ctx, systemName)
	defer func() {
		err = errtool.MapGRPCErr(ctx, err)
		tooling.ICRequestDone(ctx, err)
	}()

	req := &rm.ResolveFoldersRequest{
		FolderIds: folderIDs,
	}

	var resp *rm.ResolveFoldersResponse
	resp, err = c.folder.Resolve(ctx, req)
	if err != nil {
		return
	}
	if resp == nil {
		return
	}

	result = make([]ResolvedFolder, 0, len(resp.ResolvedFolders))
	for _, f := range resp.ResolvedFolders {
		result = append(result, ResolvedFolder{
			ID:      f.Id,
			CloudID: f.CloudId,
		})
	}
	return
}
