package agentauth

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	intapimocks "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	asmocks "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	testClusterID = "test-cluster-id"
	testIAMToken  = "test-iam-token"
	testFolderID  = "test-folder-id"
)

func setupTest(t *testing.T) (*intapimocks.MockInternalAPI, *asmocks.MockAccessService, *http.Request, http.HandlerFunc) {
	ctrl := gomock.NewController(t)
	intapi := intapimocks.NewMockInternalAPI(ctrl)
	accessService := asmocks.NewMockAccessService(ctrl)

	next := http.HandlerFunc(func(writer http.ResponseWriter, request *http.Request) {
		_, err := writer.Write([]byte("ok"))
		require.NoError(t, err)
	})

	req, err := http.NewRequest(http.MethodGet, "/some/path", nil)
	require.NoError(t, err)
	req.Header.Add(common.HeaderAgentID, testClusterID)
	req.Header.Add("Authorization", "Bearer "+testIAMToken)

	return intapi, accessService, req, next
}

func TestSuccessfulAuth(t *testing.T) {
	intapi, accessService, req, next := setupTest(t)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	accessService.EXPECT().
		Auth(gomock.Any(), testIAMToken, "dataproc.agent.proxyUi", folder).
		Return(accessservice.Subject{}, nil).
		Times(1)

	agentAuth := New(intapi, accessService, nil, &nop.Logger{})
	rr := httptest.NewRecorder()
	agentAuth.Middleware(next).ServeHTTP(rr, req)
	require.Equal(t, "ok", rr.Body.String())
}

func TestIntapiRequestFails(t *testing.T) {
	intapi, accessService, req, next := setupTest(t)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{}, xerrors.New("some error")).
		Times(1)

	agentAuth := New(intapi, accessService, nil, &nop.Logger{})
	rr := httptest.NewRecorder()
	agentAuth.Middleware(next).ServeHTTP(rr, req)
	require.Equal(t, 500, rr.Code)
	require.Equal(t, "Internal Server Error\n", rr.Body.String())
}

func TestNotAuthenticated(t *testing.T) {
	intapi, accessService, req, next := setupTest(t)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	accessService.EXPECT().
		Auth(gomock.Any(), testIAMToken, "dataproc.agent.proxyUi", folder).
		Return(accessservice.Subject{}, semerr.Authentication("iam token invalid")).
		Times(1)

	agentAuth := New(intapi, accessService, nil, &nop.Logger{})
	rr := httptest.NewRecorder()
	agentAuth.Middleware(next).ServeHTTP(rr, req)
	require.Equal(t, 401, rr.Code)
	require.Equal(t, "iam token invalid\n", rr.Body.String())
}

func TestNotAuthorized(t *testing.T) {
	intapi, accessService, req, next := setupTest(t)

	intapi.EXPECT().
		GetClusterTopology(gomock.Any(), testClusterID).
		Return(models.ClusterTopology{FolderID: testFolderID}, nil).
		Times(1)

	folder := accessservice.ResourceFolder(testFolderID)
	accessService.EXPECT().
		Auth(gomock.Any(), testIAMToken, "dataproc.agent.proxyUi", folder).
		Return(accessservice.Subject{}, semerr.Authorization("permission denied")).
		Times(1)

	agentAuth := New(intapi, accessService, nil, &nop.Logger{})
	rr := httptest.NewRecorder()
	agentAuth.Middleware(next).ServeHTTP(rr, req)
	require.Equal(t, 403, rr.Code)
	require.Equal(t, "permission denied\n", rr.Body.String())
}
