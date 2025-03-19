package cert

import (
	"context"
	"io"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	authmock "a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	secretsmock "a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestService_GetCertHandler(t *testing.T) {
	type fields struct {
		setExpectations func(
			auth *authmock.MockAuthenticator,
			sdb *secretsmock.MockService)
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
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
				setAuth(auth)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(&secretsdb.Cert{}, nil)
			}},
			args: args{
				hostname: hostname,
			},
			wantType:   certs.NewGetCertificateOK(),
			wantStatus: 200,
		},
		{
			name: "cannot get db produces error",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
				setAuth(auth)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, xerrors.New("some error"))
			}},
			args: args{
				hostname: hostname,
			},
			wantType:   certs.NewGetCertificateInternalServerError(),
			wantStatus: 500,
		},
		{
			name: "no cert in db produces error",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
				setAuth(auth)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, secretsdb.ErrCertNotFound)
			}},
			args: args{
				hostname: hostname,
			},
			wantType:   certs.NewGetCertificateNotFound(),
			wantStatus: 404,
		},
		{
			name: "db produces unavailable",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
				setAuth(auth)
				sdb.EXPECT().GetCert(gomock.Any(), hostname).Times(1).Return(nil, secretsdb.ErrNotAvailable)
			}},
			args: args{
				hostname: hostname,
			},
			wantType:   certs.NewGetCertificateServiceUnavailable(),
			wantStatus: 503,
		},
		{
			name: "auth failed",
			fields: fields{setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
				auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Times(1).Return(httpauth.ErrAuthFailure)
			}},
			args: args{
				hostname: hostname,
			},
			wantType:   certs.NewGetCertificateUnauthorized(),
			wantStatus: 401,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			auth := authmock.NewMockAuthenticator(ctrl)
			sdb := secretsmock.NewMockService(ctrl)
			if tt.fields.setExpectations != nil {
				tt.fields.setExpectations(auth, sdb)
			}

			s := New(sdb, auth, nil, &nop.Logger{}, time.Now)
			params := certs.NewGetCertificateParams()
			params.Hostname = tt.args.hostname
			got := s.GetCertHandler(context.Background(), params)

			assert.IsType(t, tt.wantType, got)
			rr := &httptest.ResponseRecorder{}
			got.WriteResponse(rr, &stubProducer{})

			assert.Equal(t, tt.wantStatus, rr.Code, "http status differs")
			ctrl.Finish()
		})
	}
}

type stubProducer struct {
}

func (s *stubProducer) Produce(_ io.Writer, _ interface{}) error {
	return nil
}
