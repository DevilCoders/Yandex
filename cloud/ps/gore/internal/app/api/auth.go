package api

import (
	"context"
	"errors"
	"fmt"
	"net"
	"net/http"
	"regexp"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

type val int
type key int

// OAuth response values
const (
	CtxAuthSuccess val = iota
	CtxAuthFailed
	CtxAuthNoHeader
	CtxAuthHeaderIncorrect
)

// OAuth - context keys
const (
	CtxStatus key = iota
	CtxUID
	CtxLogin
	CtxScopes
	CtxToken
)

var (
	ErrAuthRetardAlert = &APIErr{
		StatusCode: http.StatusInternalServerError,
		Err:        errors.New("this should never happen, but here we are"),
	}
	ErrAuthFailed = &APIErr{
		StatusCode: http.StatusForbidden,
		Err:        errors.New("authorization failed"),
	}
	ErrAuthHeaderIncorrect = &APIErr{
		StatusCode: http.StatusBadRequest,
		Err:        errors.New("authorization data corrupted"),
	}
	ErrAuthNoHeader = &APIErr{
		StatusCode: http.StatusForbidden,
		Err:        errors.New("authorization required for this method"),
	}
	ErrAuthForbidden = &APIErr{
		StatusCode: http.StatusForbidden,
		Err:        errors.New("User not authorized for this action"),
	}
)

// OAuth - ...
func (srv *Server) mwOAuth(bb blackbox.Client) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			zlog := srv.BaseLogger.With(
				log.String("module", "http-auth"),
			)
			a := r.Header["Authorization"]
			if len(a) < 1 {
				r = r.WithContext(context.WithValue(r.Context(), CtxStatus, CtxAuthNoHeader))
				next.ServeHTTP(w, r)
				return
			}

			if len(a[0]) <= 6 || a[0][:6] != "OAuth " {
				r = r.WithContext(context.WithValue(r.Context(), CtxStatus, CtxAuthHeaderIncorrect))
				next.ServeHTTP(w, r)
				return
			}

			ip, _, err := net.SplitHostPort(r.RemoteAddr)
			if err != nil {
				ip = r.RemoteAddr
			}

			bbResponse, err := bb.OAuth(r.Context(), blackbox.OAuthRequest{
				OAuthToken: a[0][6:],
				UserIP:     ip,
			})
			if err != nil {
				zlog.Errorf("Cannot create BlackBox client: %v", err)
				r = r.WithContext(context.WithValue(r.Context(), CtxStatus, CtxAuthFailed))
				next.ServeHTTP(w, r)
				return
			}

			ctx := r.Context()
			ctx = context.WithValue(ctx, CtxStatus, CtxAuthSuccess)
			ctx = context.WithValue(ctx, CtxUID, bbResponse.User.ID)
			ctx = context.WithValue(ctx, CtxLogin, bbResponse.User.Login)
			ctx = context.WithValue(ctx, CtxScopes, bbResponse.Scopes)
			ctx = context.WithValue(ctx, CtxToken, a[0][6:])
			next.ServeHTTP(w, r.WithContext(ctx))
		}
		return http.HandlerFunc(fn)
	}
}

func inOwners(to map[string]string, u string) bool {
	for _, l := range to {
		if l == u {
			return true
		}
	}

	return false
}

func inScopes(scs []string, perm string) bool {
	for _, p := range scs {
		if p == perm {
			return true
		}
	}

	return false
}

func checkPermissions(s *service.Service, u string, scs []string, perm string) error {
	if !inOwners(s.TeamOwners, u) {
		return fmt.Errorf("user %s is not one of team owners", u)
	}

	if !inScopes(scs, perm) {
		return fmt.Errorf("no active permission %s for token", perm)
	}

	return nil
}

func checkAuthContext(ctx context.Context) error {
	if st, ok := ctx.Value(CtxStatus).(val); ok {
		switch st {
		default:
			return ErrAuthRetardAlert
		case CtxAuthFailed:
			return ErrAuthFailed
		case CtxAuthHeaderIncorrect:
			return ErrAuthHeaderIncorrect
		case CtxAuthNoHeader:
			return ErrAuthNoHeader
		case CtxAuthSuccess:
			return nil
		}
	} else {
		return ErrAuthRetardAlert
	}
}

func (srv *Server) CheckAuthForService(r *http.Request) (s *service.Service, err error) {
	ctx := r.Context()
	if err = checkAuthContext(ctx); err != nil {
		return nil, err
	}

	sn := chi.URLParam(r, "serviceSlug")
	if m, _ := regexp.MatchString(objectIDRE, sn); m {
		// Probably ID
		s, err = srv.Config.Mongo.GetServiceByID(sn, nil)
	}

	if s == nil || err != nil {
		// Found nothing, trying slug
		s, err = srv.Config.Mongo.GetServiceByName(sn, nil)
	}

	if err != nil {
		return nil, ErrServiceNotFound.SetMsg(sn)
	}

	if err := checkPermissions(s, ctx.Value(CtxLogin).(string), ctx.Value(CtxScopes).([]string), appScopeWrite); err != nil {
		return nil, ErrAuthForbidden.SetMsg(err.Error())
	}

	return
}
