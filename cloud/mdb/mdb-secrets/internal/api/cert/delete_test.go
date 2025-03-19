package cert

import (
	"context"
	"fmt"
	"net/http"
	"testing"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	authmock "a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/crt/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	secretsmock "a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestService_DeleteCert(t *testing.T) {
	type fields struct {
		setExpectations func(
			auth *authmock.MockAuthenticator,
			sdb *secretsmock.MockService,
			crt *mocks.MockClient)
	}
	type args struct {
		hostname string
	}

	hostname := "test_hostname"

	setAuth := func(auth *authmock.MockAuthenticator) {
		auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
	}

	tests := []struct {
		name       string
		fields     fields
		args       args
		wantType   middleware.Responder
		wantStatus int
	}{
		{
			name: "happy path",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService, crt *mocks.MockClient) {
				setAuth(auth)
				crt.EXPECT().RevokeCertsByHostname(gomock.Any(), hostname).Times(1).Return(1, nil)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, nil)
				sdb.EXPECT().DeleteCert(gomock.Any(), hostname).Times(1).Return(nil)

			}},
			args: args{
				hostname: hostname,
			},
			wantType: certs.NewRevokeCertificateOK(),
		},
		{
			name: "cannot delete in db produces error",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService, crt *mocks.MockClient) {
				setAuth(auth)
				crt.EXPECT().RevokeCertsByHostname(gomock.Any(), hostname).Times(1).Return(1, nil)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, nil)
				sdb.EXPECT().DeleteCert(gomock.Any(), hostname).Times(1).Return(fmt.Errorf("error for tests"))
			}},
			args: args{
				hostname: hostname,
			},
			wantType: certs.NewRevokeCertificateDefault(http.StatusInternalServerError),
		},
		{
			name: "no cert in db produces error",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService, crt *mocks.MockClient) {
				setAuth(auth)
				crt.EXPECT().RevokeCertsByHostname(gomock.Any(), hostname).Times(1).Return(1, nil)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, secretsdb.ErrCertNotFound.Wrap(fmt.Errorf("error for tests")))
			}},
			args: args{
				hostname: hostname,
			},
			wantType: certs.NewRevokeCertificateDefault(http.StatusNotFound),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			auth := authmock.NewMockAuthenticator(ctrl)
			sdb := secretsmock.NewMockService(ctrl)
			crt := mocks.NewMockClient(ctrl)
			if tt.fields.setExpectations != nil {
				tt.fields.setExpectations(auth, sdb, crt)
			}

			s := New(sdb, auth, crt, &nop.Logger{}, time.Now)
			params := certs.NewRevokeCertificateParams()
			params.Hostname = tt.args.hostname
			got := s.DeleteCertificate(context.Background(), params)

			require.IsType(t, tt.wantType, got)
		})
	}
}
