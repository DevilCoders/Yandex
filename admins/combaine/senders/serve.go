package senders

import (
	"net"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/admins/combaine/senders/pb"
	"a.yandex-team.ru/library/go/core/log"
)

func Serve(endpoint string, ss pb.SenderServer) {
	log := Logger.With(log.String("source", "senders/serve.go"))

	lis, err := net.Listen("tcp", endpoint)
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	srv := grpc.NewServer(
		grpc.MaxRecvMsgSize(1024*1024*128 /* 128 MB */),
		grpc.MaxSendMsgSize(1024*1024*128 /* 128 MB */),
		grpc.MaxConcurrentStreams(2000),
		grpc.KeepaliveEnforcementPolicy(keepalive.EnforcementPolicy{
			MinTime:             10 * time.Second,
			PermitWithoutStream: true,
		}),
	)
	log.Infof("Register as gRPC server on: %s", endpoint)
	pb.RegisterSenderServer(srv, ss)
	_ = srv.Serve(lis)
}
