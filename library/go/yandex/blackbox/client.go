package blackbox

import (
	"context"
)

type Client interface {
	SessionID(ctx context.Context, req SessionIDRequest) (*SessionIDResponse, error)

	MultiSessionID(ctx context.Context, req MultiSessionIDRequest) (*MultiSessionIDResponse, error)

	OAuth(ctx context.Context, req OAuthRequest) (*OAuthResponse, error)

	UserInfo(ctx context.Context, req UserInfoRequest) (*UserInfoResponse, error)

	UserTicket(ctx context.Context, req UserTicketRequest) (*UserTicketResponse, error)

	CheckIP(ctx context.Context, req CheckIPRequest) (*CheckIPResponse, error)
}
