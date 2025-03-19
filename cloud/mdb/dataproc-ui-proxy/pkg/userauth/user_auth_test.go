package userauth

import (
	"encoding/base64"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"net/url"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/gorilla/sessions"
	"github.com/stretchr/testify/require"
	"golang.org/x/oauth2"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	token "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/token"
	iamoauth "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/oauth"
	iamoauthv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/oauth/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	intapimocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/userauth/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	asmocks "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	testMyHost            = "dataproc-ugc.yandexcloud.net"
	testIAMToken          = "test-token"
	testUserID            = "test-user-id"
	testPreferredUsername = "test-nickname"
	testAppResponse       = "my app test output"
	testClientID          = "test-client-id"
	testRedirectURI       = "/callback-path"
	testOauthDomain       = "console.cloud.yandex.ru"
	testAuthorizeURL      = "/oauth/authorize"
	testSessionKey        = "test-session-key"
	testState             = "test-state"
	testAuthCode          = "test-auth-code"
	testAccessToken       = "test-access-token"
	testYCSession         = "yc_session=xxx"
	testYCSessionSecure   = "yc_session=xxx; HttpOnly; Secure; SameSite=Lax"
	testFolderID          = "test-folder-id"
	testClusterID         = "testClusterID"
)

var testScopes = []string{"openid"}
var testErrorRenderer = util.NewErrorRenderer(false, &nop.Logger{})

func createUserAuth(t *testing.T, config Config) (*UserAuth, *mocks.MockSessionServiceClient,
	*intapimocks.MockInternalAPI, *asmocks.MockAccessService) {

	ctrl := gomock.NewController(t)
	sessionService := mocks.NewMockSessionServiceClient(ctrl)
	intapi := intapimocks.NewMockInternalAPI(ctrl)
	accessService := asmocks.NewMockAccessService(ctrl)
	agent, err := New(config, sessionService, intapi, accessService, testErrorRenderer, &nop.Logger{})
	require.NoError(t, err)
	return agent, sessionService, intapi, accessService
}

func TestSuccessfulAuthenticationViaCookie(t *testing.T) {
	config := Config{Host: testMyHost}
	agent, sessionService, intapi, accessService := createUserAuth(t, config)

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)

	req.Header.Add("Cookie", testYCSession)
	req.Header.Add("X-Forwarded-Host", fmt.Sprintf("cluster-%s.dataproc-ui", testClusterID))

	checkSessionReq := iamoauthv1.CheckSessionRequest{
		CookieHeader: testYCSession,
		Host:         testMyHost,
	}
	resp := iamoauthv1.CheckSessionResponse{
		SubjectClaims: &iamoauth.SubjectClaims{
			Sub:               testUserID,
			PreferredUsername: testPreferredUsername,
		},
		IamToken: &token.IamToken{
			IamToken: testIAMToken,
		},
	}
	sessionService.EXPECT().Check(gomock.Any(), &checkSessionReq).Return(&resp, nil).Times(1)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID, UIProxy: true}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	subject := &cloudauth.UserAccount{ID: testUserID}
	accessService.EXPECT().Authorize(gomock.Any(), subject, "dataproc.clusters.accessUi", folder).Return(nil).Times(1)

	myApp := func(w http.ResponseWriter, r *http.Request) {
		user := UserFromContext(r.Context())
		require.Equal(t, testUserID, user.Sub)
		require.Equal(t, testPreferredUsername, user.PreferredUsername)
		require.Equal(t, testIAMToken, user.IamToken)

		cookie, err := r.Cookie("yc_session")
		require.Nil(t, cookie)
		require.Equal(t, err, http.ErrNoCookie)

		_, _ = io.WriteString(w, testAppResponse)
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)
	require.Equal(t, testAppResponse, rr.Body.String())
}

func TestRedirectToOauthServer(t *testing.T) {
	config := Config{
		CallbackPath: testRedirectURI,
		Oauth: OauthConfig{
			ClientID: testClientID,
			Scopes:   testScopes,
		},
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, sessionService, _, _ := createUserAuth(t, config)

	const requestedPath = "/some/path"
	req, err := http.NewRequest("GET", requestedPath, nil)
	require.NoError(t, err)

	st := status.New(codes.InvalidArgument, "the provided cookies are invalid or may have expired")
	authorizationRequired := iamoauthv1.AuthorizationRequired{
		AuthorizeUrl: fmt.Sprintf("http://%s%s", testOauthDomain, testAuthorizeURL),
	}

	st, err = st.WithDetails(&authorizationRequired)
	require.NoError(t, err)

	sessionService.EXPECT().Check(gomock.Any(), gomock.Any()).
		Return(nil, st.Err()).
		Times(1)
	myApp := func(w http.ResponseWriter, r *http.Request) {
		t.Fatal("should not be called")
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)

	require.Equal(t, http.StatusFound, rr.Code)
	redirectURL := rr.Header().Get("Location")
	uri, err := url.Parse(redirectURL)
	require.NoError(t, err)
	require.Equal(t, testOauthDomain, uri.Host)
	require.Equal(t, testAuthorizeURL, uri.Path)
	require.Equal(t, testScopes, uri.Query()["scope"])
	require.Equal(t, []string{testClientID}, uri.Query()["client_id"])
	require.Equal(t, []string{testRedirectURI}, uri.Query()["redirect_uri"])
	require.Equal(t, []string{"code"}, uri.Query()["response_type"])

	cookie := rr.Header().Get("Set-Cookie")
	cookieStore := sessions.NewCookieStore([]byte(testSessionKey))
	header := http.Header{}
	header.Add("Cookie", cookie)
	request := &http.Request{Header: header}
	session, err := cookieStore.Get(request, sessionCookieName)
	require.NoError(t, err)

	require.Equal(t, uri.Query()["state"][0], session.Values[stateSessionKey])
	require.Equal(t, requestedPath, session.Values[requestedURLSessionKey])
}

func TestSessionServiceCheckReturnsError(t *testing.T) {
	agent, sessionService, _, _ := createUserAuth(t, Config{})

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)

	err = status.Error(codes.Unknown, "some error")
	sessionService.EXPECT().Check(gomock.Any(), gomock.Any()).Return(nil, err).Times(1)
	myApp := func(w http.ResponseWriter, r *http.Request) {
		t.Fatal("should not be called")
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)

	require.Equal(t, http.StatusInternalServerError, rr.Code)
}

func TestSuccessfulEndOfOauthFlow(t *testing.T) {
	handler := func(w http.ResponseWriter, r *http.Request) {
		err := r.ParseForm()
		require.NoError(t, err)

		code := r.Form.Get("code")
		require.Equal(t, testAuthCode, code)

		w.Header().Set("Content-Type", "application/json")
		_, err = io.WriteString(w, fmt.Sprintf(`{"access_token": "%s"}`, testAccessToken))
		require.NoError(t, err)
	}
	ts := httptest.NewServer(http.HandlerFunc(handler))
	defer ts.Close()

	config := Config{
		Oauth: OauthConfig{
			Endpoint: oauth2.Endpoint{
				TokenURL: ts.URL,
			},
		},
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, sessionService, _, _ := createUserAuth(t, config)

	req, err := http.NewRequest("GET",
		fmt.Sprintf("/oauth2?state=%s&code=%s", testState, testAuthCode), nil)
	require.NoError(t, err)

	const requestedPath = "/some/path"
	cookie := buildSessionCookie(t, testState, requestedPath)
	req.Header.Add("Cookie", cookie)

	createSessionReq := &iamoauthv1.CreateSessionRequest{
		AccessToken: testAccessToken,
	}
	createSessionResp := &iamoauthv1.CreateSessionResponse{
		SetCookieHeader: []string{testYCSession},
	}
	sessionService.EXPECT().Create(gomock.Any(), createSessionReq).
		Return(createSessionResp, nil).
		Times(1)

	rr := httptest.NewRecorder()
	agent.oauthCallback(rr, req)

	require.Equal(t, http.StatusFound, rr.Code)
	redirectURL := rr.Header().Get("Location")
	require.Equal(t, requestedPath, redirectURL)
	require.Equal(t, testYCSessionSecure, rr.Header().Get("Set-Cookie"))
}

func TestInvalidState(t *testing.T) {
	config := Config{
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, sessionService, _, _ := createUserAuth(t, config)
	sessionService.EXPECT().Create(gomock.Any(), gomock.Any()).Times(0)

	req, err := http.NewRequest("GET",
		fmt.Sprintf("/oauth2?state=%s&code=%s", "invalid-state", testAuthCode), nil)
	require.NoError(t, err)

	cookie := buildSessionCookie(t, testState, "/some/path")
	req.Header.Add("Cookie", cookie)

	rr := httptest.NewRecorder()
	agent.oauthCallback(rr, req)

	require.Equal(t, http.StatusBadRequest, rr.Code)
	require.Equal(t, "State invalid\n", rr.Body.String())
}

func TestInvalidSessionCookie(t *testing.T) {
	config := Config{
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, sessionService, _, _ := createUserAuth(t, config)
	sessionService.EXPECT().Create(gomock.Any(), gomock.Any()).Times(0)

	req, err := http.NewRequest("GET",
		fmt.Sprintf("/oauth2?state=%s&code=%s", testState, testAuthCode), nil)
	require.NoError(t, err)

	req.AddCookie(&http.Cookie{Name: sessionCookieName, Value: "some-invalid-value"})

	rr := httptest.NewRecorder()
	agent.oauthCallback(rr, req)

	require.Equal(t, http.StatusBadRequest, rr.Code)
	require.Equal(t, "State invalid\n", rr.Body.String())
}

func TestErrorResponseFromTokenEndpoint(t *testing.T) {
	testCases := []struct {
		name              string
		tokenResponseCode int
		code              int
		body              string
	}{
		{
			name:              "When Token endpoint returns 401",
			tokenResponseCode: http.StatusUnauthorized,
			code:              http.StatusUnauthorized,
			body:              "Unauthorized\n",
		},
		{
			name:              "When Token endpoint returns some unexpected error",
			tokenResponseCode: http.StatusBadRequest,
			code:              http.StatusInternalServerError,
			body:              "Internal Server Error\n",
		},
	}
	for _, testCase := range testCases {
		t.Run(testCase.name, func(t *testing.T) {
			handler := func(w http.ResponseWriter, r *http.Request) {
				w.WriteHeader(testCase.tokenResponseCode)
			}
			ts := httptest.NewServer(http.HandlerFunc(handler))

			config := Config{
				Oauth: OauthConfig{
					Endpoint: oauth2.Endpoint{
						TokenURL: ts.URL,
					},
				},
				SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
			}
			agent, sessionService, _, _ := createUserAuth(t, config)
			sessionService.EXPECT().Create(gomock.Any(), gomock.Any()).Times(0)

			req, err := http.NewRequest("GET",
				fmt.Sprintf("/oauth2?state=%s&code=%s", testState, "invalid-code"), nil)
			require.NoError(t, err)

			cookie := buildSessionCookie(t, testState, "/some/path")
			req.Header.Add("Cookie", cookie)

			rr := httptest.NewRecorder()
			agent.oauthCallback(rr, req)

			require.Equal(t, testCase.code, rr.Code)
			require.Equal(t, testCase.body, rr.Body.String())
			ts.Close()
		})
	}
}

func TestSessionServiceCreateReturnsError(t *testing.T) {
	handler := func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		_, err := io.WriteString(w, fmt.Sprintf(`{"access_token": "%s"}`, testAccessToken))
		require.NoError(t, err)
	}
	ts := httptest.NewServer(http.HandlerFunc(handler))
	defer ts.Close()

	config := Config{
		Oauth: OauthConfig{
			Endpoint: oauth2.Endpoint{
				TokenURL: ts.URL,
			},
		},
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, sessionService, _, _ := createUserAuth(t, config)

	req, err := http.NewRequest("GET",
		fmt.Sprintf("/oauth2?state=%s&code=%s", testState, testAuthCode), nil)
	require.NoError(t, err)

	cookie := buildSessionCookie(t, testState, "/some/path")
	req.Header.Add("Cookie", cookie)

	sessionService.EXPECT().Create(gomock.Any(), gomock.Any()).
		Return(nil, xerrors.New("some error")).
		Times(1)

	rr := httptest.NewRecorder()
	agent.oauthCallback(rr, req)

	require.Equal(t, http.StatusInternalServerError, rr.Code)
	require.Equal(t, "Internal Server Error\n", rr.Body.String())
}

func TestUnknownClusterID(t *testing.T) {
	config := Config{Host: testMyHost}
	agent, sessionService, intapi, _ := createUserAuth(t, config)

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)

	req.Header.Add("Cookie", testYCSession)
	req.Header.Add("X-Forwarded-Host", fmt.Sprintf("cluster-%s.dataproc-ui", testClusterID))

	checkSessionReq := iamoauthv1.CheckSessionRequest{
		CookieHeader: testYCSession,
		Host:         testMyHost,
	}
	resp := iamoauthv1.CheckSessionResponse{
		SubjectClaims: &iamoauth.SubjectClaims{
			Sub:               testUserID,
			PreferredUsername: testPreferredUsername,
		},
		IamToken: &token.IamToken{
			IamToken: testIAMToken,
		},
	}
	sessionService.EXPECT().Check(gomock.Any(), &checkSessionReq).Return(&resp, nil).Times(1)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{}, semerr.NotFoundf("cluster with id %s not found", testClusterID)).
		Times(1)

	myApp := func(w http.ResponseWriter, r *http.Request) {
		t.Fatal("should not be called")
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)
	require.Equal(t, http.StatusNotFound, rr.Code)
}

func TestUnauthorized(t *testing.T) {
	config := Config{Host: testMyHost}
	agent, sessionService, intapi, accessService := createUserAuth(t, config)

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)

	req.Header.Add("Cookie", testYCSession)
	req.Header.Add("X-Forwarded-Host", fmt.Sprintf("cluster-%s.dataproc-ui", testClusterID))

	checkSessionReq := iamoauthv1.CheckSessionRequest{
		CookieHeader: testYCSession,
		Host:         testMyHost,
	}
	resp := iamoauthv1.CheckSessionResponse{
		SubjectClaims: &iamoauth.SubjectClaims{
			Sub:               testUserID,
			PreferredUsername: testPreferredUsername,
		},
		IamToken: &token.IamToken{
			IamToken: testIAMToken,
		},
	}
	sessionService.EXPECT().Check(gomock.Any(), &checkSessionReq).Return(&resp, nil).Times(1)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	subject := &cloudauth.UserAccount{ID: testUserID}
	accessService.EXPECT().
		Authorize(gomock.Any(), subject, "dataproc.clusters.accessUi", folder).
		Return(semerr.Authorizationf("authorization failed")).
		Times(1)

	myApp := func(w http.ResponseWriter, r *http.Request) {
		t.Fatal("should not be called")
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)
	require.Equal(t, http.StatusForbidden, rr.Code)
}

func TestUIProxyIsDisabled(t *testing.T) {
	config := Config{Host: testMyHost}
	agent, sessionService, intapi, accessService := createUserAuth(t, config)

	req, err := http.NewRequest("GET", "/some/path", nil)
	require.NoError(t, err)

	req.Header.Add("Cookie", testYCSession)
	req.Header.Add("X-Forwarded-Host", fmt.Sprintf("cluster-%s.dataproc-ui", testClusterID))

	checkSessionReq := iamoauthv1.CheckSessionRequest{
		CookieHeader: testYCSession,
		Host:         testMyHost,
	}
	resp := iamoauthv1.CheckSessionResponse{
		SubjectClaims: &iamoauth.SubjectClaims{
			Sub:               testUserID,
			PreferredUsername: testPreferredUsername,
		},
		IamToken: &token.IamToken{
			IamToken: testIAMToken,
		},
	}
	sessionService.EXPECT().Check(gomock.Any(), &checkSessionReq).Return(&resp, nil).Times(1)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID, UIProxy: false}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	subject := &cloudauth.UserAccount{ID: testUserID}
	accessService.EXPECT().
		Authorize(gomock.Any(), subject, "dataproc.clusters.accessUi", folder).
		Return(nil).
		Times(1)

	myApp := func(w http.ResponseWriter, r *http.Request) {
		t.Fatal("should not be called")
	}
	rr := httptest.NewRecorder()
	agent.AuthMiddleware(http.HandlerFunc(myApp)).ServeHTTP(rr, req)
	require.Equal(t, http.StatusForbidden, rr.Code)
}

func buildSessionCookie(t *testing.T, state, requestedPath string) string {
	req, err := http.NewRequest("GET", "/", nil)
	require.NoError(t, err)

	cookieStore := sessions.NewCookieStore([]byte(testSessionKey))
	session, _ := cookieStore.Get(req, sessionCookieName)
	session.Values[stateSessionKey] = state
	session.Values[requestedURLSessionKey] = requestedPath
	rr := httptest.NewRecorder()
	err = session.Save(req, rr)
	require.NoError(t, err)

	return rr.Header().Get("Set-Cookie")
}
