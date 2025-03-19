package grpc

import (
	"context"

	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/datatransfer"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	dt_api "a.yandex-team.ru/transfer_manager/go/proto/api"
)

type Client struct {
	conn *grpc.ClientConn
}

var _ datatransfer.DataTransferService = &Client{}

func (d Client) IsReady(ctx context.Context) error {
	health := grpchealth.NewHealthClient(d.conn)
	resp, err := health.Check(ctx, &grpchealth.HealthCheckRequest{})
	if err != nil {
		return err
	}

	switch resp.Status {
	case grpchealth.HealthCheckResponse_SERVING:
		return nil
	case grpchealth.HealthCheckResponse_NOT_SERVING:
		return xerrors.New("service is not serving")
	case grpchealth.HealthCheckResponse_SERVICE_UNKNOWN:
		return xerrors.New("requested health info for unknown service")
	}

	return xerrors.Errorf("unknown response for health request: %d", resp.Status)
}

func (d Client) ResolveFolderID(ctx context.Context, transfers []string) ([]string, error) {
	client := dt_api.NewTransferServiceClient(d.conn)
	var res []string
	for _, transferID := range transfers {
		transfer, err := client.Get(ctx, &dt_api.GetTransferRequest{TransferId: transferID})
		if err != nil {
			return nil, semerr.Authorizationf("get transfer: %s:%v", transferID, err)
		}
		res = append(res, transfer.FolderId)
	}
	return res, nil
}

func New(shutdownCtx context.Context, addr string, clientCfg grpcutil.ClientConfig, userAgent string, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(
		shutdownCtx,
		addr,
		userAgent,
		clientCfg,
		l,
		grpcutil.WithClientCredentials(grpcauth.NewContextCredentials(iamauth.NewIAMAuthTokenModel())),
	)
	if err != nil {
		return nil, err
	}

	return &Client{conn: conn}, nil
}
