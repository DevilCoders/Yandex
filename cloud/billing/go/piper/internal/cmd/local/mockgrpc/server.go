package mockgrpc

import (
	"context"
	"fmt"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/health"
	"google.golang.org/grpc/health/grpc_health_v1"

	rm "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	ti "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/team/integration/v1"
)

func RunMock(runCtx context.Context, endpoint string) error {
	server := grpc.NewServer()
	handler := &mockRMHandler{}
	rm.RegisterFolderServiceServer(server, handler)
	grpc_health_v1.RegisterHealthServer(server, health.NewServer())

	listener, err := net.Listen("tcp", endpoint)
	if err != nil {
		return fmt.Errorf("can not listen %s: %w", endpoint, err)
	}
	done, stop := context.WithCancel(context.Background())
	go func() {
		<-runCtx.Done()
		server.Stop()
		_ = listener.Close()
		stop()
	}()

	err = server.Serve(listener)
	if err != nil {
		stop()
	}
	<-done.Done()
	return err
}

type mockRMHandler struct {
	rm.UnimplementedFolderServiceServer
}

func (m *mockRMHandler) Resolve(ctx context.Context, req *rm.ResolveFoldersRequest) (*rm.ResolveFoldersResponse, error) {
	result := rm.ResolveFoldersResponse{}
	for _, fid := range req.GetFolderIds() {
		if rf, ok := knownFolders[fid]; ok {
			result.ResolvedFolders = append(result.ResolvedFolders, rf)
		}
	}
	return &result, nil
}

var knownFolders = map[string]*rm.ResolvedFolder{
	"aoe7o2nhjmomln63fdb0": {CloudId: "aoeeg7rh1qpmr19bkrfj", Id: "aoe7o2nhjmomln63fdb0"},
}

type mockTIHandler struct {
	ti.UnimplementedAbcServiceServer
}

func (m *mockTIHandler) Resolve(ctx context.Context, req *ti.ResolveRequest) (*ti.ResolveResponse, error) {
	abcID := req.GetAbcId()
	rf := knownABC[abcID]
	return rf, nil
}

var knownABC = map[int64]*ti.ResolveResponse{
	123: {
		CloudId:         "abc_cloud",
		AbcSlug:         "test_service",
		AbcId:           123,
		DefaultFolderId: "abc_folder",
		AbcFolderId:     "abc_folder",
	},
}
