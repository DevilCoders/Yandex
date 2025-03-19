package functest

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/pkg/internalapi"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/pkg/internalapi/grpc"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type GRPCContext struct {
	Client    internalapi.Client
	RawClient *grpc.ClientConn

	LastResponse proto.Message
	LastError    error

	AuthToken     string
	IdempotenceID string
}

// Implements gRPC credentials
var _ credentials.PerRPCCredentials = &GRPCContext{}

func NewgRPCContext(port int, l log.Logger) (*GRPCContext, error) {
	grpcCtx := &GRPCContext{}
	ctx := context.Background()
	c, err := grpcapi.New(
		ctx,
		fmt.Sprintf(":%d", port),
		"func_test",
		grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{Insecure: true}},
		grpcCtx,
		l,
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to create internal api client: %w", err)
	}

	ctx, cancel := context.WithTimeout(ctx, time.Second*5)
	defer cancel()
	if err = ready.Wait(ctx, c, &ready.DefaultErrorTester{}, time.Second); err != nil {
		return nil, xerrors.Errorf("failed to wait for internal api client to become ready: %w", err)
	}

	grpcCtx.Client = c
	grpcCtx.RawClient = c.Conn()
	return grpcCtx, nil
}

func (gc *GRPCContext) Reset() {
	gc.LastResponse = nil
	gc.LastError = nil
}

func (gc *GRPCContext) GetRequestMetadata(_ context.Context, _ ...string) (map[string]string, error) {
	if gc.AuthToken != "" {
		return api.NewAuthHeader(gc.AuthToken), nil
	}

	return nil, nil
}

func (gc *GRPCContext) RequireTransportSecurity() bool {
	return false
}
