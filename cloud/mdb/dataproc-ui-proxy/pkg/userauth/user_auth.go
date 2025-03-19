package userauth

import (
	"bufio"
	"context"
	"crypto/rand"
	"encoding/base64"
	"fmt"
	"io"
	"net/http"
	urlpkg "net/url"
	"strings"

	"github.com/gorilla/sessions"
	"golang.org/x/oauth2"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	iamoauth "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/oauth/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	intapi "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh SessionServiceClient

type OauthConfig = oauth2.Config
type SessionServiceClient = iamoauth.SessionServiceClient

type Config struct {
	Host                 string      `json:"host" yaml:"host"`
	CallbackPath         string      `json:"callback_path" yaml:"callback_path"`
	Oauth                OauthConfig `json:"oauth" yaml:"oauth"`
	SessionCookieAuthKey string      `json:"session_cookie_auth_key" yaml:"session_cookie_auth_key"`
	CAFile               string      `yaml:"ca_file"`
}

type User struct {
	IamToken string

	// Following is subset of fields of
	// a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/oauth/v1.SubjectClaims
	// It may be extended if necessary

	// Subject - Identifier for the End-User at the Issuer.
	Sub string
	// Shorthand name by which the End-User wishes to be referred to at the RP, such as janedoe or j.doe.
	// This value MAY be any valid JSON string including special characters such as @, /, or whitespace. The RP MUST NOT rely upon this value being unique, as discussed in Section 5.7.
	PreferredUsername string
}

type ctxKey string

const (
	requestedURLSessionKey = "requested-url"
	ycSessionCookieName    = "yc_session"
	sessionCookieName      = "auth_agent_session"
	stateLength            = 16
	stateSessionKey        = "state"
	userContextKey         = ctxKey("user")
	permission             = "dataproc.clusters.accessUi"
)

type UserAuth struct {
	config         Config
	sessionService SessionServiceClient
	internalAPI    intapi.InternalAPI
	accessService  accessservice.AccessService
	logger         log.Logger
	cookieStore    *sessions.CookieStore
	renderError    util.RenderError
	httpClient     *http.Client
}

func New(
	config Config,
	sessionService SessionServiceClient,
	internalAPI intapi.InternalAPI,
	accessService accessservice.AccessService,
	renderError util.RenderError,
	logger log.Logger) (*UserAuth, error) {

	client := UserAuth{
		config:         config,
		sessionService: sessionService,
		internalAPI:    internalAPI,
		accessService:  accessService,
		logger:         logger,
		renderError:    renderError,
	}
	client.config.Oauth.RedirectURL = client.config.Host + client.config.CallbackPath

	sessionKey, err := base64.URLEncoding.DecodeString(config.SessionCookieAuthKey)
	if err != nil {
		return nil, xerrors.Errorf("failed to decode session cookie auth key: %w", err)
	}
	client.cookieStore = sessions.NewCookieStore(sessionKey)

	// We're going to use custom http client for requests to oauth server because it may have certificate
	// signed by internal CA. See https://st.yandex-team.ru/MDB-17171.
	// Even if config.CAFile is not defined (empty), httputil.NewTransport will create transport that
	// uses YandexInternalRootCA.
	cfg := httputil.TransportConfig{
		TLS: httputil.TLSConfig{
			CAFile: config.CAFile,
		},
	}
	rt, err := httputil.NewTransport(cfg, &nop.Logger{})
	if err != nil {
		return nil, err
	}
	client.httpClient = &http.Client{Transport: rt}

	return &client, nil
}

func (ua *UserAuth) AuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Whole authentication flow is:
		// * user sends request to requestedURL=https://cluster-XXX.dataproc-ui.cloud-preprod.yandex.net/some/path
		// * send Cookie header content to SessionService.Check()
		// * it responds with iamoauth.AuthorizationRequired
		// * redirect user to https://dataproc-ui.cloud-preprod.yandex.net/auth?redirect_to=${requestedURL}
		// * once again: request to SessionService.Check() and iamoauth.AuthorizationRequired response
		// * prepare for oauth flow (generate random state, save it to cookie)
		// * redirect user to oauth server at https://console-preprod.cloud.yandex.ru/oauth/authorize
		// * oauth server authenticates user
		// * redirect user to https://dataproc-ui.cloud-preprod.yandex.net/oauth2
		// * validate request params (eg. state)
		// * exchange auth code (taken from /oauth2 params) for token via oauth server at
		//   https://console-preprod.cloud.yandex.ru/oauth/token
		// * exchange auth token for session cookie via SessionService.Create()
		// * set session cookie for *.dataproc-ui.cloud-preprod.yandex.net
		// * redirect user to ${requestedURL}
		// * user agent sends request to ${requestedURL} along with session cookie, which is successfully
		//   checked via SessionService.Check().
		cookie := r.Header.Get("Cookie")
		checkSessionResponse, authorizeURL, err := ua.authenticate(r.Context(), cookie)
		if err != nil {
			ua.renderError(w, xerrors.Errorf("failed to check cookie: %s", err))
			return
		}
		if authorizeURL != "" {
			baseDomainURL := ua.baseDomainURL(r)
			if baseDomainURL != "" {
				http.Redirect(w, r, baseDomainURL, http.StatusFound)
				return
			}

			session, _ := ua.cookieStore.Get(r, sessionCookieName)
			session.Options.Secure = true
			state, err := generateRandomState()
			if err != nil {
				ua.renderError(w, xerrors.Errorf("failed to generate random state: %s", err))
				return
			}
			session.Values[stateSessionKey] = state
			err = r.ParseForm()
			if err != nil {
				ua.renderError(w, xerrors.Errorf("failed to parse request: %w", err))
				return
			}
			requestedURL := r.Form.Get("redirect_to")
			if requestedURL == "" {
				requestedURL = r.URL.String()
			}
			session.Values[requestedURLSessionKey] = requestedURL
			err = session.Save(r, w)
			if err != nil {
				ua.renderError(w, xerrors.Errorf("failed to save session to cookie: %s", err))
				return
			}

			oauthConfig := ua.config.Oauth
			oauthConfig.Endpoint.AuthURL = authorizeURL
			fullAuthorizeURL := oauthConfig.AuthCodeURL(state)
			ua.logger.Infof("Redirecting to oauth server at %s", authorizeURL)
			http.Redirect(w, r, fullAuthorizeURL, http.StatusFound)
			return
		}

		user := buildUser(checkSessionResponse)
		ua.logger.Debugf("User %s authenticated by cookie", user.Sub)

		clusterID, err := ua.authorize(r, user)
		if err != nil {
			ua.renderError(w, xerrors.Errorf("failed to authorize user: %w", err))
			return
		}

		ctx := context.WithValue(r.Context(), userContextKey, user)
		ctx = util.WithClusterID(ctx, clusterID)
		r = r.WithContext(ctx)

		var cookies []string
		for _, cookie := range r.Cookies() {
			if cookie.Name != ycSessionCookieName {
				cookies = append(cookies, cookie.String())
			}
		}
		r.Header.Set("Cookie", strings.Join(cookies, "; "))

		next.ServeHTTP(w, r)
	})
}

// Returns dataproc-ui-proxy URL where oauth flow will be started.
// This URL is on the "base" domain (eg. dataproc-ui.cloud-preprod.yandex.net), not subdomain like
// cluster-XXX.dataproc-ui.cloud-preprod.yandex.net.
func (ua *UserAuth) baseDomainURL(r *http.Request) string {
	host := requestedHost(r)
	parts := strings.Split(host, ".")
	if strings.HasPrefix(parts[0], "cluster-") || strings.HasPrefix(parts[0], "ui-") {
		authHost := strings.Join(parts[1:], ".")
		scheme := "http://"
		if r.TLS != nil {
			scheme = "https://"
		}
		q := urlpkg.Values{}
		q.Set("redirect_to", scheme+host+r.RequestURI)
		return scheme + authHost + "/?" + q.Encode()
	}
	return ""
}

func UserFromContext(ctx context.Context) *User {
	user, ok := ctx.Value(userContextKey).(*User)
	if !ok {
		panic("user info not found in context")
	}

	return user
}

// Find out user by her cookie
func (ua *UserAuth) authenticate(ctx context.Context, cookie string) (
	*iamoauth.CheckSessionResponse, string, error) {
	req := &iamoauth.CheckSessionRequest{
		CookieHeader: cookie,
		Host:         ua.config.Host,
	}
	resp, err := ua.sessionService.Check(ctx, req)
	if err != nil {
		st := status.Convert(err)
		switch st.Code() {
		case codes.Unauthenticated:
			// authorization iam_token are invalid or may have expired
			return nil, "", xerrors.Errorf("SessionService.Check responded with codes.Unauthenticated: %w", err)
		case codes.InvalidArgument:
			// the provided cookies are invalid or may have expired
			authorizeURL := ""
			for _, detail := range st.Details() {
				if authorizationRequired, ok := detail.(*iamoauth.AuthorizationRequired); ok {
					authorizeURL = authorizationRequired.AuthorizeUrl
				}
			}
			if authorizeURL == "" {
				return nil, "", xerrors.Errorf("SessionService.Check returned InvalidArgument error, "+
					"but I failed to extract AuthorizationRequired message from it: %w", err)
			}

			return nil, authorizeURL, nil
		default:
			return nil, "", xerrors.Errorf("SessionService.Check returned error with unexpected "+
				"status code %d: %w", st.Code(), err)
		}
	}

	return resp, "", nil
}

func (ua *UserAuth) Install(httpServer *http.ServeMux) {
	httpServer.HandleFunc(ua.config.CallbackPath, ua.oauthCallback)
}

func (ua *UserAuth) oauthCallback(w http.ResponseWriter, r *http.Request) {
	err := r.ParseForm()
	if err != nil {
		ua.renderError(w, xerrors.Errorf("failed to parse oauth callback request: %w", err))
		return
	}

	if r.Form.Get("error") == "server_error" {
		msg := r.Form.Get("error_description")
		ua.renderError(w, semerr.Authentication(msg))
		return
	}

	state := r.Form.Get("state")
	code := r.Form.Get("code")
	session, err := ua.cookieStore.Get(r, sessionCookieName)
	if err != nil {
		ua.renderError(w, semerr.WrapWithInvalidInput(err, "State invalid"))
		return
	}

	expectedState, ok := session.Values[stateSessionKey].(string)
	if !ok || state != expectedState {
		ua.renderError(w, semerr.InvalidInput("State invalid"))
		return
	}

	if code == "" {
		ua.renderError(w, semerr.InvalidInput("Code not found"))
		return
	}

	cookies, err := ua.completeAuth(r.Context(), code)
	if err != nil {
		ua.renderError(w, err)
		return
	}

	ua.logger.Infof("Successfully authenticated user via oauth server")

	var unexpectedCookies []string
	for _, cookieString := range cookies {
		// By default cookie is only valid for domain, on which it is set.
		// We want it to be sent also from subdomains.
		cookie := parseCookie(cookieString)
		if cookie != nil {
			// from mdn: if a domain is specified, then subdomains are always included
			domain := requestedHost(r)
			domain = strings.Split(domain, ":")[0] // remove port if any
			cookie.Domain = domain
			cookie.HttpOnly = true
			cookie.Secure = true
			cookie.SameSite = http.SameSiteLaxMode
			cookieString = cookie.String()
			if cookie.Name != ycSessionCookieName {
				unexpectedCookies = append(unexpectedCookies, cookie.Name)
			}
		}
		if len(unexpectedCookies) > 0 {
			ua.logger.Warnf("Following unexpected cookies are set by Session Service: %v."+
				" These cookies will not be removed from requests sent to the upstream.", unexpectedCookies)
		}
		w.Header().Set("Set-Cookie", cookieString)
	}

	url, ok := session.Values[requestedURLSessionKey].(string)
	if !ok {
		url = "/"
	}
	session.Options.MaxAge = -1 // expire auth session cookie
	err = session.Save(r, w)
	if err != nil {
		ua.logger.Errorf("Failed to save auth session cookie: %s", err)
	}
	ua.logger.Infof("Redirecting to initial url at %s", url)
	http.Redirect(w, r, url, http.StatusFound)
}

func (ua *UserAuth) completeAuth(ctx context.Context, code string) ([]string, error) {
	ctx = context.WithValue(ctx, oauth2.HTTPClient, ua.httpClient)
	token, err := ua.config.Oauth.Exchange(ctx, code)
	if err != nil {
		retrieveError, ok := err.(*oauth2.RetrieveError)
		if ok && retrieveError.Response.StatusCode == http.StatusUnauthorized {
			return nil, semerr.WrapWithAuthentication(retrieveError, "Unauthorized")
		}
		return nil, xerrors.Errorf("failed to exchange auth code for auth token: %w", err)
	}

	req := &iamoauth.CreateSessionRequest{
		AccessToken: token.AccessToken,
	}

	resp, err := ua.sessionService.Create(ctx, req)
	if err != nil {
		return nil, xerrors.Errorf("failed to create session: %w", err)
	}

	return resp.SetCookieHeader, nil
}

func (ua *UserAuth) authorize(r *http.Request, user *User) (string, error) {
	host := requestedHost(r)
	subdomain := strings.Split(host, ".")[0]
	parts := strings.Split(subdomain, "-")
	if len(parts) < 2 {
		return "", semerr.NotFoundf("page not found. Hint: invalid subdomain %q", subdomain)
	}
	clusterID := parts[1]

	topology, err := ua.internalAPI.GetClusterTopology(r.Context(), clusterID)
	if err != nil {
		return clusterID, xerrors.Errorf("failed to fetch cluster topology from intapi: %w", err)
	}
	resourceFolder := accessservice.ResourceFolder(topology.FolderID)
	subject := &cloudauth.UserAccount{ID: user.Sub}
	err = ua.accessService.Authorize(r.Context(), subject, permission, resourceFolder)
	if err != nil {
		e := semerr.AsSemanticError(err)
		if e != nil && e.Semantic == semerr.SemanticAuthorization {
			return clusterID, semerr.Authorizationf("%s (hint: make sure that you have role dataproc.user "+
				"within folder %s)", e.Message, topology.FolderID)
		}
		return clusterID, err
	}

	if !topology.UIProxy {
		return clusterID, semerr.Authorizationf("UI Proxy feature is turned off for the cluster %s", clusterID)
	}
	return clusterID, nil
}

func generateRandomState() (string, error) {
	// taken from here: https://a.yandex-team.ru/arc/trunk/arcadia/vendor/github.com/gorilla/sessions/store.go#L224
	// and here: https://a.yandex-team.ru/arc/trunk/arcadia/vendor/github.com/gorilla/securecookie/securecookie.go#L511
	key := make([]byte, stateLength)
	if _, err := io.ReadFull(rand.Reader, key); err != nil {
		return "", err
	}

	return base64.RawURLEncoding.EncodeToString(key), nil
}

func buildUser(checkSessionResponse *iamoauth.CheckSessionResponse) *User {
	return &User{
		IamToken:          checkSessionResponse.IamToken.IamToken,
		Sub:               checkSessionResponse.SubjectClaims.Sub,
		PreferredUsername: checkSessionResponse.SubjectClaims.PreferredUsername,
	}
}

func parseCookie(cookieString string) *http.Cookie {
	rawRequest := fmt.Sprintf("GET / HTTP/1.0\r\nCookie: %s\r\n\r\n", cookieString)
	req, err := http.ReadRequest(bufio.NewReader(strings.NewReader(rawRequest)))

	if err == nil {
		cookies := req.Cookies()
		if len(cookies) > 0 {
			return cookies[0]
		}
	}
	return nil
}

func requestedHost(r *http.Request) string {
	host := r.Header.Get("X-Forwarded-Host")
	if host == "" {
		host = r.Host
	}
	return host
}
