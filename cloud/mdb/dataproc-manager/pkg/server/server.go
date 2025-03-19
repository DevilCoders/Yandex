package server

import (
	"context"
	"net"
	"strings"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/reflection"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	cc "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/compute-service"
	iam "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/iam-token-service"
	ig "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/cloud/instance-group-service"
	datastoreBackend "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore"
	internalAPI "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	as_grpc "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type contextKey int

const (
	agentReportPermission            = "mdb.dataproc.agentReport"
	folderIDKey           contextKey = iota
)

// CertConfig describes configuration for server TLS certificate
type CertConfig struct {
	CertFile string `json:"cert_file" yaml:"cert_file"`
	KeyFile  string `json:"key_file" yaml:"key_file"`
}

// Config describes configuration for dataproc-manager server
type Config struct {
	Listen string     `json:"listen" yaml:"listen"`
	CAPath string     `json:"capath" yaml:"capath"`
	IAMURL string     `json:"iamurl" yaml:"iamurl"`
	NoAuth bool       `json:"noauth" yaml:"noauth"`
	Cert   CertConfig `json:"cert" yaml:"cert"`
}

// DefaultConfig returns default config for dataproc-manager server
func DefaultConfig() Config {
	return Config{
		Listen: "0.0.0.0:50051",
	}
}

// Server implements grpc dataproc-manager server
type Server struct {
	logger                     log.Logger
	cfg                        Config
	accessService              as.AccessService
	datastore                  datastoreBackend.Backend
	intAPI                     internalAPI.InternalAPI
	computeServiceClient       cc.ComputeServiceClient
	instanceGroupServiceClient ig.InstanceGroupServiceClient
	iamTokenServiceClient      iam.IamTokenServiceClient
	reportCount                int64
}

// NewServer constructs new dataproc-manager server
func NewServer(cfg Config, datastore datastoreBackend.Backend, intAPI internalAPI.InternalAPI, logger log.Logger,
	computeServiceClient cc.ComputeServiceClient,
	instanceGroupServiceClient ig.InstanceGroupServiceClient) (*Server, error) {
	grpcConfig := grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{TLS: grpcutil.TLSConfig{CAFile: cfg.CAPath}}}
	accessService, err := as_grpc.NewClient(context.Background(), cfg.IAMURL, "DataProc Manager", grpcConfig, logger)
	if err != nil {
		return nil, xerrors.Errorf("failed to init Access Service client: %w", err)
	}
	return &Server{
		logger:                     logger,
		cfg:                        cfg,
		accessService:              accessService,
		datastore:                  datastore,
		intAPI:                     intAPI,
		computeServiceClient:       computeServiceClient,
		instanceGroupServiceClient: instanceGroupServiceClient,
	}, nil
}

// Run server
func (s *Server) Run() {
	var serverGrpc *grpc.Server
	lis, err := net.Listen("tcp", s.cfg.Listen)
	if err != nil {
		s.logger.Errorf("Failed to listen: %s", err)
	}

	options := append(
		[]grpc.ServerOption{
			grpc.UnaryInterceptor(
				interceptors.ChainUnaryServerInterceptors(
					nil,
					false,
					nil,
					interceptors.DefaultLoggingConfig(),
					s.logger,
				),
			),
		},
		grpcutil.DefaultKeepaliveServerOptions()...,
	)

	defaultCert := CertConfig{}
	if s.cfg.Cert != defaultCert {
		creds, err := credentials.NewServerTLSFromFile(s.cfg.Cert.CertFile, s.cfg.Cert.KeyFile)
		if err != nil {
			s.logger.Errorf("Failed to read certificate: %s", err)
		} else {
			options = append(options, grpc.Creds(creds))
		}
	}

	serverGrpc = grpc.NewServer(options...)
	pb.RegisterDataprocManagerServiceServer(serverGrpc, s)
	pb.RegisterJobServiceServer(serverGrpc, s)
	grpchealth.RegisterHealthServer(serverGrpc, s)
	reflection.Register(serverGrpc)
	if err := serverGrpc.Serve(lis); err != nil {
		s.logger.Errorf("Failed to serve: %v", err)
	}
}

func (s *Server) validateContext(ctx context.Context) (folderID string, err error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", xerrors.New("can't get metadata from context")
	}
	auth := md.Get("authorization")
	var token string
	// We got array of two elements
	if len(auth) == 2 {
		token = auth[0]
		folderID = auth[1]
	} else if len(auth) == 1 {
		// Or one string comma separated
		splitted := strings.Split(auth[0], ",")
		if len(splitted) != 2 {
			return "", semerr.Authentication("bad auth metadata")
		}
		token = splitted[0]
		folderID = splitted[1]
	} else {
		return "", semerr.Authentication("bad auth metadata")
	}

	_, err = s.accessService.Auth(ctx, token, agentReportPermission, as.ResourceFolder(folderID))
	if err != nil {
		s.logger.Warnf("Auth error: %v", err)
		err = semerr.WhitelistErrors(err, semerr.SemanticAuthentication, semerr.SemanticAuthorization)
		return "", err
	}

	return folderID, nil
}

func (s *Server) validateClusterWithinFolder(ctx context.Context, clusterID string, folderID string) error {
	topology, err := s.getClusterTopology(ctx, clusterID, -1)
	if err != nil {
		return xerrors.Errorf("failed to get cluster topology: %w", err)
	}

	if folderID != topology.FolderID {
		s.logger.Warnf("folderId from request: %s, folderId from topology: %s", folderID, topology.FolderID)
		return semerr.Authentication("bad auth metadata")
	}

	return nil
}

func (s *Server) authorizeClusterAccess(ctx context.Context, clusterID string) error {
	if s.cfg.NoAuth {
		return nil
	}

	folderID, err := s.validateContext(ctx)
	if err != nil {
		return err
	}

	err = s.validateClusterWithinFolder(ctx, clusterID, folderID)
	return err
}
