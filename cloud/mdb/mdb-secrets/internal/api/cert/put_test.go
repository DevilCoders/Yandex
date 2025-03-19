package cert

import (
	"context"
	"reflect"
	"testing"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	authmock "a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/internal/crt/fixtures"
	"a.yandex-team.ru/cloud/mdb/internal/crt/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/crt/yacrt"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	secretsmock "a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

const (
	hostname = "test_hostname"
	caName   = "InternalCA"
)

var altNames = []string{
	"alt_test_hostname",
}

func TestService_PutCert(t *testing.T) {
	type fields struct {
		setExpectations func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService)
		nowF            func() time.Time
	}
	type args struct {
		ctx    context.Context
		params certs.PutParams
	}

	auth := "test_auth"
	force := true

	certExpiration := fixtures.CertEndDate
	dbCrt := &secretsdb.Cert{
		Key: secretsdb.EncryptedData{
			Version: 1,
			Data:    "base64 data",
		},
		Crt:        fixtures.FullCert,
		Host:       hostname,
		Expiration: certExpiration,
		AltNames:   altNames,
	}

	tests := []struct {
		name        string
		fields      fields
		args        args
		wantType    middleware.Responder
		wantPayload *secretsdb.Cert
	}{
		{
			name: "has_cert_in_db",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(dbCrt, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(-(expiringPeriod * 2))
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      altNames,
				},
			},
			wantType:    certs.NewPutOK(),
			wantPayload: dbCrt,
		},
		{
			name: "has_cert_in_db_with_force_flag",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(dbCrt, nil)
					sdb.EXPECT().UpdateCert(gomock.Any(), hostname, caName, gomock.Any(), gomock.Any()).Return(&secretsdb.Cert{
						Key: secretsdb.EncryptedData{
							Version: 1,
							Data:    "new_cert_data",
						},
						Crt:        "new_cert",
						Expiration: certExpiration.Add(time.Hour * 6666),
					}, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(-(expiringPeriod * 2))
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      altNames,
					Force:         &force,
				},
			},
			wantType: certs.NewPutOK(),
			wantPayload: &secretsdb.Cert{
				Key: secretsdb.EncryptedData{
					Version: 1,
					Data:    "new_cert_data",
				},
				Crt: "new_cert",
			},
		},
		{
			name: "no_cert_in_db",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(nil, secretsdb.ErrCertNotFound)
					sdb.EXPECT().InsertDummyCert(gomock.Any(), hostname).Return(nil)
					sdb.EXPECT().UpdateCert(gomock.Any(), hostname, caName, gomock.Any(), gomock.Any()).Return(dbCrt, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(-(expiringPeriod * 2))
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      altNames,
				},
			},
			wantType:    certs.NewPutOK(),
			wantPayload: dbCrt,
		},
		{
			name: "expired_cert",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(&secretsdb.Cert{
						Key: secretsdb.EncryptedData{
							Version: 1,
							Data:    "old_cert_data",
						},
						Crt:        "old_cert",
						Host:       hostname,
						Expiration: certExpiration,
					}, nil)
					sdb.EXPECT().UpdateCert(gomock.Any(), hostname, caName, gomock.Any(), gomock.Any()).Return(&secretsdb.Cert{
						Key: secretsdb.EncryptedData{
							Version: 1,
							Data:    "new_cert_data",
						},
						Crt:        "new_cert",
						Expiration: certExpiration.Add(time.Hour * 6666),
					}, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(expiringPeriod)
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      altNames,
				},
			},
			wantType: certs.NewPutOK(),
			wantPayload: &secretsdb.Cert{
				Key: secretsdb.EncryptedData{
					Version: 1,
					Data:    "new_cert_data",
				},
				Crt: "new_cert",
			},
		},
		{
			name: "different_alt_names",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(&secretsdb.Cert{
						Crt:        "old_cert",
						Expiration: certExpiration,
						AltNames:   []string{"alt_name_1"},
					}, nil)
					sdb.EXPECT().UpdateCert(gomock.Any(), hostname, caName, gomock.Any(), gomock.Any()).Return(&secretsdb.Cert{
						Crt:        "new_cert",
						Expiration: certExpiration,
						AltNames:   []string{"alt_name_1", "alt_name_2"},
					}, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(-(expiringPeriod * 2))
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      []string{"alt_name_1", "alt_name_2"},
				},
			},
			wantType: certs.NewPutOK(),
			wantPayload: &secretsdb.Cert{
				Crt:      "new_cert",
				AltNames: []string{"alt_name_1", "alt_name_2"},
			},
		},
		{
			name: "no_cert_in_db_with_force_flag",
			fields: fields{
				setExpectations: func(auth *authmock.MockAuthenticator, sdb *secretsmock.MockService) {
					auth.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
					sdb.EXPECT().GetCert(gomock.Any(), hostname).Return(nil, secretsdb.ErrCertNotFound)
					sdb.EXPECT().InsertDummyCert(gomock.Any(), hostname).Return(nil)
					sdb.EXPECT().UpdateCert(gomock.Any(), hostname, caName, gomock.Any(), gomock.Any()).Return(&secretsdb.Cert{
						Key: secretsdb.EncryptedData{
							Version: 1,
							Data:    "new_cert_data",
						},
						Crt:        "new_cert",
						Expiration: certExpiration.Add(time.Hour * 6666),
					}, nil)
				},
				nowF: func() time.Time {
					return fixtures.CertEndDate.Add(-(expiringPeriod * 2))
				},
			},
			args: args{
				ctx: context.Background(),
				params: certs.PutParams{
					Authorization: &auth,
					Hostname:      hostname,
					Ca:            caName,
					AltNames:      altNames,
					Force:         &force,
				},
			},
			wantType: certs.NewPutOK(),
			wantPayload: &secretsdb.Cert{
				Key: secretsdb.EncryptedData{
					Version: 1,
					Data:    "new_cert_data",
				},
				Crt: "new_cert",
			},
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

			s := New(sdb, auth, mocks.NewMockClient(ctrl), &nop.Logger{}, tt.fields.nowF)
			got := s.PutCertHandler(tt.args.ctx, tt.args.params)

			require.IsType(t, tt.wantType, got)

			if putok, ok := got.(*certs.PutOK); ok {
				require.Equal(t, tt.wantPayload.Crt, putok.Payload.Cert)
				require.Equal(t, tt.wantPayload.Key.Version, putok.Payload.Key.Version)
				require.Equal(t, tt.wantPayload.Key.Data, putok.Payload.Key.Data)
			}
		})
	}
}

func TestService_newCert(t *testing.T) {
	type args struct {
		ctx      context.Context
		hostname string
		caName   string
		certType crt.CertificateType
		force    bool
	}

	hostname := "test_hostname"
	caName := "InternalCA"
	crtCert := &crt.Cert{
		CertPem:    []byte(fixtures.FullCert),
		KeyPem:     []byte(fixtures.PrivateKey),
		Expiration: fixtures.CertEndDate,
	}

	tests := []struct {
		name            string
		setExpectations func(client *mocks.MockClient)
		args            args
		want            *secretsdb.CertUpdate
		wantErr         bool
	}{
		{
			name: "existing",
			setExpectations: func(client *mocks.MockClient) {
				client.EXPECT().ExistingCert(gomock.Any(), hostname, caName, altNames).Return(crtCert, nil)
			},
			args: args{
				hostname: hostname,
				caName:   caName,
				certType: yacrt.CertTypeMDB,
				force:    false,
			},
			want: &secretsdb.CertUpdate{
				Key:        string(crtCert.KeyPem),
				Crt:        string(crtCert.CertPem),
				Expiration: crtCert.Expiration,
				AltNames:   altNames,
			},
			wantErr: false,
		},
		{
			name: "existing_with_force_flag",
			setExpectations: func(client *mocks.MockClient) {
				client.EXPECT().IssueCert(gomock.Any(), hostname, altNames, caName, yacrt.CertTypeMDB).Return(crtCert, nil)
			},
			args: args{
				hostname: hostname,
				caName:   caName,
				certType: yacrt.CertTypeMDB,
				force:    true,
			},
			want: &secretsdb.CertUpdate{
				Key:        string(crtCert.KeyPem),
				Crt:        string(crtCert.CertPem),
				Expiration: crtCert.Expiration,
				AltNames:   altNames,
			},
			wantErr: false,
		},
		{
			name: "new",
			setExpectations: func(client *mocks.MockClient) {
				client.EXPECT().ExistingCert(gomock.Any(), hostname, caName, altNames).Return(nil, crt.ErrNoCerts)
				client.EXPECT().IssueCert(gomock.Any(), hostname, altNames, caName, yacrt.CertTypeMDB).Return(crtCert, nil)
			},
			args: args{
				hostname: hostname,
				caName:   caName,
				certType: yacrt.CertTypeMDB,
				force:    false,
			},
			want: &secretsdb.CertUpdate{
				Key:        string(crtCert.KeyPem),
				Crt:        string(crtCert.CertPem),
				Expiration: crtCert.Expiration,
				AltNames:   altNames,
			},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			crtClient := mocks.NewMockClient(ctrl)

			tt.setExpectations(crtClient)

			s := &Service{crtClient: crtClient}
			got, err := s.newCert(tt.args.ctx, tt.args.hostname, altNames, tt.args.caName, tt.args.certType, tt.args.force)
			if (err != nil) != tt.wantErr {
				t.Errorf("Service.newCert() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("Service.newCert() = %v, want %v", got, tt.want)
			}
		})
	}
}
