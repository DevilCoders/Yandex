package main

import (
	"context"
	"log"
	"math/rand"
	"net"
	"os"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	rmgrpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
)

type Server struct {
	Args *Args
}

type FolderService struct {
	rmgrpc.UnimplementedFolderServiceServer
	Server
}

func (server *FolderService) Resolve(ctx context.Context, req *rmgrpc.ResolveFoldersRequest) (*rmgrpc.ResolveFoldersResponse, error) {
	result := make([]*rmgrpc.ResolvedFolder, 0, len(req.FolderIds))
	for _, folderID := range req.FolderIds {
		result = append(result, &rmgrpc.ResolvedFolder{Id: folderID, CloudId: strings.Split(folderID, "F")[0]})
	}
	return &rmgrpc.ResolveFoldersResponse{
		ResolvedFolders: result,
	}, nil
}

func main() {
	rand.Seed(time.Now().UnixNano())
	args := ParseArgs(os.Args[1:])

	lCfg := &net.ListenConfig{}
	lis, err := lCfg.Listen(context.Background(), "tcp", args.Addr)

	if err != nil {
		log.Fatalf("Failed to listen: %v", err)
	}

	s := grpc.NewServer()

	server := Server{
		Args: &args,
	}
	rmgrpc.RegisterFolderServiceServer(s, &FolderService{
		rmgrpc.UnimplementedFolderServiceServer{},
		server,
	})
	// Register reflection service on gRPC server.
	reflection.Register(s)

	log.Printf("Server will start on %v address\n", args.Addr)
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
