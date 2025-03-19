package grpc

import (
	"context"
	"time"

	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client implements API to Access Service
type Client struct {
	client *cloudauth.AccessServiceClient
}

var _ as.AccessService = &Client{}

// NewClient constructs new thread safe client
func NewClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, l log.Logger) (as.AccessService, error) {
	conn, err := grpcutil.NewConn(
		ctx,
		target,
		userAgent,
		cfg,
		l,
		grpcutil.WithClientKeepalive(
			// https://st.yandex-team.ru/CLOUDINC-1322#5fddb68ef5bd1f2229a57d5c
			keepalive.ClientParameters{
				Time:                11 * time.Second,
				Timeout:             time.Second,
				PermitWithoutStream: true,
			},
		),
	)
	if err != nil {
		return nil, err
	}

	return &Client{client: cloudauth.NewAccessServiceClient(conn)}, nil
}

func (c *Client) authenticate(ctx context.Context, token string) (cloudauth.Subject, error) {
	iamToken := cloudauth.NewIAMToken(token)
	subject, err := c.client.Authenticate(ctx, iamToken)
	if err != nil {
		return nil, xerrors.Errorf("authentication failed: %w", grpcerr.SemanticErrorFromGRPC(err))
	}

	return subject, nil
}

func (c *Client) Authorize(ctx context.Context, subject cloudauth.Subject, permission string, resourcePath ...cloudauth.Resource) error {
	_, err := c.client.Authorize(
		ctx,
		subject.(cloudauth.Authorizable),
		permission,
		resourcePath...,
	)
	if err != nil {
		return xerrors.Errorf("authorization failed: %w", grpcerr.SemanticErrorFromGRPC(err))
	}

	return nil
}

func (c *Client) Auth(ctx context.Context, token, permission string, resources ...as.Resource) (as.Subject, error) {
	subject, err := c.authenticate(ctx, token)
	if err != nil {
		return as.Subject{}, err
	}

	var sub as.Subject
	switch s := subject.(type) {
	case cloudauth.AnonymousAccount:
		sub = as.Subject{}
	case cloudauth.UserAccount:
		sub = as.Subject{User: &as.UserAccount{ID: s.ID}}
	case cloudauth.ServiceAccount:
		sub = as.Subject{Service: &as.ServiceAccount{ID: s.ID, FolderID: s.FolderID}}
	default:
		return as.Subject{}, xerrors.Errorf("unknown subject type in response")
	}

	if err = c.Authorize(ctx, subject, permission, resources...); err != nil {
		return sub, err
	}
	return sub, nil
}
