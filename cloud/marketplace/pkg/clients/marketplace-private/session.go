package marketplace

import (
	"context"
	"fmt"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/core/log"
)

type SessionManager interface {
	SessionWithYCSubjectToken(ctx context.Context, iamToken string) APISession
	SessionWithAuthToken(ctx context.Context, token string) APISession
}

type Session struct {
	ctx    context.Context
	client *resty.Client

	logger log.Logger

	authInfo   authInfo
	opInterval time.Duration
	opTimeout  time.Duration
}

type authInfo struct {
	header string
	token  string
}

func (a *authInfo) kv() (string, string) {
	return a.header, a.token
}

func (c *Client) SessionWithYCSubjectToken(ctx context.Context, iamToken string) APISession {
	return c.sessionWithAuthToken(ctx, authInfo{
		header: "X-YaCloud-SubjectToken",
		token:  iamToken,
	})
}

func (c *Client) SessionWithAuthToken(ctx context.Context, token string) APISession {
	return c.sessionWithAuthToken(ctx, authInfo{
		header: "Authorization",
		token:  fmt.Sprintf("Bearer %s", token), // TODO: check header value format.
	})
}

func (c *Client) sessionWithAuthToken(ctx context.Context, authInfo authInfo) *Session {
	return &Session{
		ctx: ctx,

		client: c.client,
		logger: c.logger,

		authInfo:   authInfo,
		opInterval: c.OpPollInterval,
		opTimeout:  c.OpTimeout,
	}
}

func (s *Session) ctxAuthRequest() *resty.Request {
	return s.client.R().
		SetContext(s.ctx).
		SetHeader(s.authInfo.kv()) // Set auth header.
}
