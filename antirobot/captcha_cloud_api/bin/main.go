package main

import (
	"context"
	"log"
	"net"
	"os"
	"syscall"

	"golang.org/x/sys/unix"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/sugar"

	smartcaptcha_pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"

	corezap "a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/logbroker/unified_agent/client/go/ua"

	corelog "a.yandex-team.ru/library/go/core/log"
)

type Server struct {
	Args                  *Args
	YdbConnection         *ydb.Connection
	Authority1            *Authority
	ResourceManagerClient FolderServiceClient
	AccessLogger          *Logger
	EventLogger           *Logger
	SearchTopicLogger     *Logger
	AuditLogger           *Logger
}

func (server *Server) Authenticate(ctx context.Context) (*Subject, error) {
	return server.Authority1.Authenticate(ctx)
}

func (server *Server) Authorize(ctx context.Context, resource cloudauth.Resource, permission string) (*Subject, error) {
	return server.Authority1.AuthorizeByIamToken(ctx, resource, permission)
}

func (server *Server) AuthorizeAnon(ctx context.Context, resource cloudauth.Resource, permission string) (*Subject, error) {
	return server.Authority1.AuthorizeBySubject(ctx, cloudauth.AnonymousAccount{}, resource, permission)
}

type OperationType int

const (
	UnknownOperation OperationType = iota
	CreateOperation
	DeleteOperation
	UpdateOperation
)

type QuotaMetricName string

const (
	QuotaMetricCaptchasCount QuotaMetricName = "smart-captcha.captchas.count"
)

func main() {
	args := ParseArgs(os.Args[1:])
	if args.Debug {
		log.Println(args)
	}

	ctx := context.Background()
	db, err := ydb.Open(
		ctx,
		sugar.DSN(args.YdbEndpoint, args.YdbDatabase, false),
		ydb.WithAccessTokenCredentials(args.YdbToken),
	)
	if err != nil {
		log.Fatalf("Failed to connect YDB server: %v", err)
	}
	defer func() {
		_ = db.Close(ctx)
	}()

	lCfg := &net.ListenConfig{}
	lCfg.Control = func(network, address string, c syscall.RawConn) error {
		var fn = func(s uintptr) {
			if args.Debug {
				setErr := syscall.SetsockoptInt(int(s), unix.SOL_SOCKET, unix.SO_REUSEADDR, 1)
				if setErr != nil {
					log.Fatal(setErr)
				}
				setErr = syscall.SetsockoptInt(int(s), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
				if setErr != nil {
					log.Fatal(setErr)
				}
			}
		}
		if err := c.Control(fn); err != nil {
			return err
		}
		return nil
	}

	lis, err := lCfg.Listen(context.Background(), "tcp", args.Addr)

	if err != nil {
		log.Fatalf("Failed to listen: %v", err)
	}

	s := grpc.NewServer()
	authClient, errAccess := NewAccessServiceClient(ctx, &args)
	if errAccess != nil {
		log.Fatalf("Failed to connect access server: %v", errAccess)
	}

	rmClient, errRmClient := NewResourceManagerClient(ctx, &args)
	if errRmClient != nil {
		log.Fatalf("Failed to connect resource manager server: %v", errRmClient)
	}

	var uaAccessLogClient *ua.UAClient = nil
	var uaEventLogClient *ua.UAClient = nil
	var uaSearchTopicLogClient *ua.UAClient = nil
	var uaAuditLogClient *ua.UAClient = nil

	if len(args.UnifiedAgentURI) > 0 {
		innerLogger, err := corezap.NewDeployLogger(corelog.WarnLevel)
		if err != nil {
			log.Fatalln("Failed to initialize logger: ", err)
		}

		createClient := func(logType string) (*ua.UAClient, func()) {
			sessionParamaters := ua.SessionParameters{
				MaxInflightBytes: 0,
				Meta:             map[string]string{"log_type": logType},
			}
			client, err := ua.NewClient(ctx, args.UnifiedAgentURI, ua.WithLogger(innerLogger), ua.WithSession(sessionParamaters))
			if err != nil {
				log.Fatalln("Failed to create UA client: ", err)
			}
			return client, func() {
				log.Println("Close unified agent")
				err := client.Close(ctx)
				if err != nil {
					log.Fatalln("Failed to close unified agent client:", err)
				}
			}
		}
		var closeFunc func()
		uaAccessLogClient, closeFunc = createClient("captcha_cloud_api_access_log")
		defer closeFunc()
		uaEventLogClient, closeFunc = createClient("captcha_cloud_api_event_log")
		defer closeFunc()
		uaSearchTopicLogClient, closeFunc = createClient("captcha_cloud_api_search_topic_log")
		defer closeFunc()
		uaAuditLogClient, closeFunc = createClient("captcha_cloud_api_audit_log")
		defer closeFunc()
	}

	var fileLogger *log.Logger = nil
	if args.Debug {
		fileLogger = log.New(os.Stdout, "", log.Ldate|log.Ltime|log.Lshortfile)
	}

	accessLogger := &Logger{
		FileLogger:         fileLogger,
		UnifiedAgentClient: uaAccessLogClient,
	}
	eventLogger := &Logger{
		FileLogger:         fileLogger,
		UnifiedAgentClient: uaEventLogClient,
	}
	searchTopicLogger := &Logger{
		FileLogger:         fileLogger,
		UnifiedAgentClient: uaSearchTopicLogClient,
	}
	auditLogger := &Logger{
		FileLogger:         fileLogger,
		UnifiedAgentClient: uaAuditLogClient,
	}

	server := Server{
		Args:                  &args,
		YdbConnection:         &db,
		Authority1:            &Authority{Auth: authClient},
		ResourceManagerClient: rmClient,
		AccessLogger:          accessLogger,
		EventLogger:           eventLogger,
		SearchTopicLogger:     searchTopicLogger,
		AuditLogger:           auditLogger,
	}
	smartcaptcha_pb.RegisterCaptchaSettingsServiceServer(s, &CaptchaServer{
		smartcaptcha_pb.UnimplementedCaptchaSettingsServiceServer{},
		server,
	})
	smartcaptcha_pb.RegisterQuotaServiceServer(s, &QuotaServer{
		smartcaptcha_pb.UnimplementedQuotaServiceServer{},
		server,
	})
	smartcaptcha_pb.RegisterStatsServiceServer(s, &StatsServer{
		smartcaptcha_pb.UnimplementedStatsServiceServer{},
		server,
	})
	smartcaptcha_pb.RegisterOperationServiceServer(s, &OperationServer{
		smartcaptcha_pb.UnimplementedOperationServiceServer{},
		server,
	})
	// Register reflection service on gRPC server.
	reflection.Register(s)

	server.EventLogger.Message("Server start")

	log.Printf("Server will start on %v address\n", args.Addr)
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
