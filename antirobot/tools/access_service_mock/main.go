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
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/reflection"
	"google.golang.org/grpc/status"

	asrpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

type Server struct {
	Args *Args
}

type AccessService struct {
	asrpc.UnimplementedAccessServiceServer
	Server
}

func (server *AccessService) Authenticate(ctx context.Context, req *asrpc.AuthenticateRequest) (*asrpc.AuthenticateResponse, error) {
	switch x := req.Credentials.(type) {
	case *asrpc.AuthenticateRequest_IamToken:
		if x.IamToken == "my-test-auth-token" {
			return &asrpc.AuthenticateResponse{
				Subject: &asrpc.Subject{
					Type: &asrpc.Subject_UserAccount_{
						UserAccount: &asrpc.Subject_UserAccount{
							Id: "testing_user_account",
						},
					},
				},
			}, nil
		}
		if x.IamToken == "my-test-no-permissions-auth-token" {
			return &asrpc.AuthenticateResponse{
				Subject: &asrpc.Subject{
					Type: &asrpc.Subject_UserAccount_{
						UserAccount: &asrpc.Subject_UserAccount{
							Id: "testing_no_permissions_user_account",
						},
					},
				},
			}, nil
		}

		return nil, status.Errorf(codes.Unauthenticated, "Unauthenticated")
	default:
	}
	return nil, status.Errorf(codes.Unauthenticated, "Unauthenticated")
}

func (server *AccessService) Authorize(ctx context.Context, req *asrpc.AuthorizeRequest) (*asrpc.AuthorizeResponse, error) {
	switch x := req.Identity.(type) {
	case *asrpc.AuthorizeRequest_IamToken:
		if x.IamToken == "my-test-auth-token" {
			return &asrpc.AuthorizeResponse{
				Subject: &asrpc.Subject{
					Type: &asrpc.Subject_UserAccount_{
						UserAccount: &asrpc.Subject_UserAccount{
							Id: "testing_user_account",
						},
					},
				},
				ResourcePath: req.ResourcePath,
			}, nil
		}
		return nil, status.Errorf(codes.PermissionDenied, "Permission denied")

	case *asrpc.AuthorizeRequest_Subject:
		switch x.Subject.Type.(type) {
		case *asrpc.Subject_AnonymousAccount_:
			if req.Permission == "smart-captcha.captchas.show" && req.ResourcePath[0].Type == "resource-manager.cloud" && strings.HasPrefix(req.ResourcePath[0].Id, "active") {
				return &asrpc.AuthorizeResponse{
					Subject:      x.Subject,
					ResourcePath: req.ResourcePath,
				}, nil
			}
		default:
		}
	default:
	}
	return nil, status.Errorf(codes.PermissionDenied, "Permission denied")
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
	asrpc.RegisterAccessServiceServer(s, &AccessService{
		asrpc.UnimplementedAccessServiceServer{},
		server,
	})
	// Register reflection service on gRPC server.
	reflection.Register(s)

	log.Printf("Server will start on %v address\n", args.Addr)
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
