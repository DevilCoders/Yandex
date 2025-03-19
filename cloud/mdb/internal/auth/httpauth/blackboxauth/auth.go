package blackboxauth

import (
	"context"
	"net"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey string

// Known headers
var (
	HeaderXRealIP    = http.CanonicalHeaderKey("X-Real-Ip")
	CookieSessionID  = "Session_id"
	ctxKeyToken      = ctxKey("token")
	ctxKeySessionID  = ctxKey("session_id")
	ctxKeyRemoteAddr = ctxKey("remote_addr")
)

func (a *BlackBoxAuth) getSessionID(request *http.Request) string {
	sessionID, err := request.Cookie(CookieSessionID)
	if err != nil {
		ctxlog.Debug(request.Context(), a.l, "session_id cookie not found")
		return ""
	}
	return sessionID.Value
}

func (a *BlackBoxAuth) getOauthToken(request *http.Request) string {
	vals := request.Header[tvm.HeaderAuthorization]
	if len(vals) == 0 {
		ctxlog.Warnf(request.Context(), a.l, "header %q not found", tvm.HeaderAuthorization)
		return ""
	}
	if len(vals) > 1 {
		ctxlog.Warnf(request.Context(), a.l, "multiple %q headers found, expected one", tvm.HeaderAuthorization)
		return ""
	}
	ret, err := tvm.ParseOAuthToken(vals[0])
	if err != nil {
		ctxlog.Warnf(request.Context(), a.l, "header %q parse error: %s", tvm.HeaderAuthorization, err)
		return ""
	}
	return ret
}

func (a *BlackBoxAuth) cookieAuth(ctx context.Context, remoteAddr string, sessionID string) (blackbox.UserInfo, error) {
	if a.cache != nil {
		cached, err := a.cache.Get(sessionID)
		if err == nil {
			return cached, nil
		}
	}

	srvToken, err := a.tvmc.GetServiceTicket(ctx, a.alias)
	if err != nil {
		return blackbox.UserInfo{}, xerrors.Errorf("failed to retrieve service ticket from TVM: %w", err)
	}

	uinfo, err := a.bbc.SessionID(ctx, srvToken, sessionID, remoteAddr, a.baseHost, nil)
	if err == nil && a.cache != nil {
		a.cache.Put(sessionID, uinfo)
	}
	return uinfo, err
}

func (a *BlackBoxAuth) tokenAuth(ctx context.Context, remoteAddr string, oauthToken string) (blackbox.UserInfo, error) {
	if a.cache != nil {
		cached, err := a.cache.Get(oauthToken)
		if err == nil {
			return cached, nil
		}
	}

	srvToken, err := a.tvmc.GetServiceTicket(ctx, a.alias)
	if err != nil {
		return blackbox.UserInfo{}, xerrors.Errorf("failed to retrieve service ticket from TVM: %w", err)
	}

	uinfo, err := a.bbc.OAuth(ctx, srvToken, oauthToken, remoteAddr, nil)
	if err != nil {
		return uinfo, err
	}

	if a.cache != nil {
		a.cache.Put(oauthToken, uinfo)
	}

	if a.oauthScopeChecker != nil {
		err = a.oauthScopeChecker.CheckScope(uinfo)
		if err != nil {
			return blackbox.UserInfo{}, err
		}
	}

	return uinfo, err
}

func (a *BlackBoxAuth) getRemoteIP(request *http.Request) string {
	vals := request.Header[HeaderXRealIP]
	if len(vals) > 0 {
		return vals[0]
	}
	remoteAddr := request.RemoteAddr
	userip, _, err := net.SplitHostPort(remoteAddr)
	if err != nil {
		ctxlog.Warnf(request.Context(), a.l, "invalid http client remote address %q: %s", remoteAddr, err)
		userip = remoteAddr
	}
	return userip
}

// Auth checks oauth token or session id
func (a *BlackBoxAuth) AuthUser(ctx context.Context, request *http.Request) (blackbox.UserInfo, error) {
	actx := a.SetContext(ctx, a.getSessionID(request), a.getOauthToken(request), a.getRemoteIP(request))

	return a.CtxAuth(actx)
}

func (a *BlackBoxAuth) Auth(ctx context.Context, request *http.Request) error {
	_, err := a.AuthUser(ctx, request)
	return err
}

// CtxAuth checks oauth token or session id saved in context
func (a *BlackBoxAuth) CtxAuth(ctx context.Context) (blackbox.UserInfo, error) {
	var uinfo blackbox.UserInfo
	var err error

	remoteAddr, _ := ctx.Value(ctxKeyRemoteAddr).(string)

	sessionID, _ := ctx.Value(ctxKeySessionID).(string)
	if sessionID != "" {
		uinfo, err = a.cookieAuth(ctx, remoteAddr, sessionID)
		if err != nil {
			return blackbox.UserInfo{}, err
		}
	} else {
		oauthToken, _ := ctx.Value(ctxKeyToken).(string)
		if oauthToken != "" {
			uinfo, err = a.tokenAuth(ctx, remoteAddr, oauthToken)
			if err != nil {
				return blackbox.UserInfo{}, err
			}
		} else {
			return blackbox.UserInfo{}, httpauth.ErrNoAuthCredentials
		}
	}

	if a.loginListChecker != nil {
		err = a.loginListChecker.Check(uinfo)
		if err != nil {
			return blackbox.UserInfo{}, err
		}
	}

	return uinfo, nil
}

// SetContext saves auth params in context
func (a *BlackBoxAuth) SetContext(ctx context.Context, sessionID string, oauthToken string, remoteAddr string) context.Context {
	sctx := context.WithValue(ctx, ctxKeySessionID, sessionID)
	tctx := context.WithValue(sctx, ctxKeyToken, oauthToken)
	return context.WithValue(tctx, ctxKeyRemoteAddr, remoteAddr)
}
