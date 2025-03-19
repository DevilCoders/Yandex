package userauth

import (
	"bytes"
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/base64"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/cookiejar"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/golang/mock/gomock"
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
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/userauth/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	asmocks "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const logInTests = false

// Whole authentication flow is described within AuthMiddleware() method.
// Here we test this flow end-to-end.
func TestWholeAuthenticationFlow(t *testing.T) {
	var logger log.Logger = &nop.Logger{}
	if logInTests {
		zapConfig := zap.KVConfig(log.DebugLevel)
		zapConfig.OutputPaths = []string{"stdout"}
		var err error
		logger, err = zap.New(zapConfig)
		require.NoError(t, err)
	}

	oauthDomain := "console-preprod.cloud.yandex.ru"
	uiProxyDomain := "dataproc-ui.cloud-preprod.yandex.net"
	uiProxyURL := ""

	// Step 1. Run oauth server stub
	oauthHandler := func(w http.ResponseWriter, r *http.Request) {
		err := r.ParseForm()
		require.NoError(t, err)

		switch r.URL.Path {
		case "/oauth/authorize":
			state := r.Form.Get("state")
			callbackURL := fmt.Sprintf("%s/oauth2?state=%s&code=%s", uiProxyURL, state, testAuthCode)
			logger.Debugf("redirecting back to %s", callbackURL)
			http.Redirect(w, r, callbackURL, http.StatusFound)
		case "/oauth/token":
			code := r.Form.Get("code")
			require.Equal(t, testAuthCode, code)

			w.Header().Set("Content-Type", "application/json")
			_, err = io.WriteString(w, fmt.Sprintf(`{"access_token": "%s"}`, testAccessToken))
			require.NoError(t, err)
		default:
			t.Fatalf("unexpected path: %s", r.URL.Path)
		}
	}
	oauthServer := httptest.NewServer(http.HandlerFunc(oauthHandler))
	defer oauthServer.Close()
	parts := strings.Split(oauthServer.URL, ":")
	port := parts[len(parts)-1]
	oauthURL := "http://" + oauthDomain + ":" + port

	// Step 2. Create UserAuth object
	ctrl := gomock.NewController(t)
	sessionService := mocks.NewMockSessionServiceClient(ctrl)
	intapi := intapimocks.NewMockInternalAPI(ctrl)
	accessService := asmocks.NewMockAccessService(ctrl)
	config := Config{
		CallbackPath: "/oauth2",
		Oauth: OauthConfig{
			Endpoint: oauth2.Endpoint{
				TokenURL: oauthServer.URL + "/oauth/token",
			},
		},
		SessionCookieAuthKey: base64.URLEncoding.EncodeToString([]byte(testSessionKey)),
	}
	agent, err := New(config, sessionService, intapi, accessService, testErrorRenderer, logger)
	require.NoError(t, err)

	// Step 3. Run stub ui proxy server with authentication middleware and simple handler
	srv := http.ServeMux{}
	agent.Install(&srv) // setup handler for /oauth2 callback
	var handler http.Handler
	handler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// this will do actual work after request is authenticated
		_, err = w.Write([]byte("hello world"))
		require.NoError(t, err)
	})
	handler = agent.AuthMiddleware(handler)
	srv.Handle("/", handler)
	uiProxyHTTPServer := httptest.NewUnstartedServer(&srv)
	uiProxyTLSCert, uiProxyTLSConfig, err := common.GenerateSelfSignedCert([]string{uiProxyDomain, "*." + uiProxyDomain})
	require.NoError(t, err)
	uiProxyHTTPServer.TLS = uiProxyTLSConfig
	uiProxyHTTPServer.StartTLS()
	defer uiProxyHTTPServer.Close()
	parts = strings.Split(uiProxyHTTPServer.URL, ":")
	port = parts[len(parts)-1]
	uiProxyURL = "https://" + uiProxyDomain + ":" + port
	uiProxyClusterURL := fmt.Sprintf("https://cluster-%s.%s:%s", testClusterID, uiProxyDomain, port)

	// Step 4. Mock response to the first check-session request to the session service
	// (request will be without cookie)
	st := status.New(codes.InvalidArgument, "the provided cookies are invalid or may have expired")
	authorizationRequired := iamoauthv1.AuthorizationRequired{
		AuthorizeUrl: oauthURL + "/oauth/authorize",
	}
	st, err = st.WithDetails(&authorizationRequired)
	require.NoError(t, err)
	sessionService.EXPECT().Check(gomock.Any(), gomock.Any()).
		Return(nil, st.Err()).
		Times(2)

	// Step 5. Mock response to create-session request
	createSessionReq := &iamoauthv1.CreateSessionRequest{
		AccessToken: testAccessToken,
	}
	createSessionResp := &iamoauthv1.CreateSessionResponse{
		SetCookieHeader: []string{testYCSession},
	}
	sessionService.EXPECT().Create(gomock.Any(), createSessionReq).
		Return(createSessionResp, nil).
		Times(1)

	// Step 6. Mock response to the second check-session request to the session service.
	// This time user is authenticated and cookie is present
	checkSessionRequest := &iamoauthv1.CheckSessionRequest{
		CookieHeader: testYCSession,
	}
	checkSessionResponse := iamoauthv1.CheckSessionResponse{
		SubjectClaims: &iamoauth.SubjectClaims{
			Sub:               testUserID,
			PreferredUsername: testPreferredUsername,
		},
		IamToken: &token.IamToken{
			IamToken: testIAMToken,
		},
	}
	sessionService.EXPECT().Check(gomock.Any(), checkSessionRequest).
		Return(&checkSessionResponse, nil).
		Times(1)

	// Step. Mock intapi and AccessService (used for authorization)
	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID, UIProxy: true}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	subject := &cloudauth.UserAccount{ID: testUserID}
	accessService.EXPECT().
		Authorize(gomock.Any(), subject, "dataproc.clusters.accessUi", folder).
		Return(nil).
		Times(1)

	// Step 7. Create http client with cookie Jar and dispatch request. Then check response.
	// Step 7.1. Customize http transport: all domain names are "resolved" to 127.0.0.1
	defaultTransport := http.DefaultTransport.(*http.Transport)
	myTransport := defaultTransport.Clone()
	myTransport.DialContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
		parts := strings.Split(addr, ":")
		parts[0] = "127.0.0.1"
		addr = strings.Join(parts, ":")
		return defaultTransport.DialContext(ctx, network, addr)
	}
	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(uiProxyTLSCert)
	myTransport.TLSClientConfig = &tls.Config{
		RootCAs: caCertPool,
	}

	cookieJar, err := cookiejar.New(nil)
	require.NoError(t, err)
	client := &http.Client{
		Transport: myTransport,
		Jar:       cookieJar,
	}
	req, err := http.NewRequest(http.MethodGet, uiProxyClusterURL+"/some/path", nil)
	require.NoError(t, err)
	resp, err := client.Do(req)
	require.NoError(t, err)
	require.Equal(t, http.StatusOK, resp.StatusCode)
	buffer := bytes.Buffer{}
	_, err = buffer.ReadFrom(resp.Body)
	require.NoError(t, err)
	require.Equal(t, "hello world", buffer.String())
}
