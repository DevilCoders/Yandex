package yacrt

import (
	"context"
	"io"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_crtClient_latestCert(t *testing.T) {
	type args struct {
		ctx      context.Context
		target   string
		altNames []string
		caName   string
	}
	tests := []struct {
		name             string
		args             args
		body             string
		wantDownloadLink string
		wantErr          error
	}{
		{
			name: "no_alt_names",
			body: `{
								    "count": 2,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-31T16:30:30.646742+03:00",
								            "end_date": "2021-05-30T16:20:30+03:00",
								            "issued": "2019-05-31T16:30:30.903801+03:00",
								            "updated": "2019-05-31T16:35:02.611244+03:00",
								            "revoked": null,
								            "serial_number": "4B799D35000200075A56",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://first-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com"
								            ]
								        },
								        {
								            "common_name": "test.com",
								            "added": "2019-05-29T10:38:54.565081+03:00",
								            "end_date": "2021-05-28T10:28:54+03:00",
								            "issued": "2019-05-29T10:38:54.837755+03:00",
								            "updated": "2019-05-29T10:40:12.817277+03:00",
								            "revoked": null,
								            "serial_number": "3FEAF008000200075098",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://second-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com"
								            ]
								        }
								    ]
								}`,
			args: args{
				ctx:      context.Background(),
				target:   "test.com",
				caName:   "InternalCA",
				altNames: nil,
			},
			wantDownloadLink: "https://first-cert.com",
			wantErr:          nil,
		},
		{
			name: "alt_names",
			body: `{
								    "count": 2,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-31T16:30:30.646742+03:00",
								            "end_date": "2021-05-30T16:20:30+03:00",
								            "issued": "2019-05-31T16:30:30.903801+03:00",
								            "updated": "2019-05-31T16:35:02.611244+03:00",
								            "revoked": null,
								            "serial_number": "4B799D35000200075A56",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://first-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com"
								            ]
								        },
								        {
								            "common_name": "test.com",
								            "added": "2019-05-29T10:38:54.565081+03:00",
								            "end_date": "2021-05-28T10:28:54+03:00",
								            "issued": "2019-05-29T10:38:54.837755+03:00",
								            "updated": "2019-05-29T10:40:12.817277+03:00",
								            "revoked": null,
								            "serial_number": "3FEAF008000200075098",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://second-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com",
												"test1.com",
												"test2.com"
								            ]
								        }
								    ]
								}`,

			args: args{
				ctx:      context.Background(),
				target:   "test.com",
				caName:   "InternalCA",
				altNames: []string{"test1.com", "test2.com"},
			},
			wantDownloadLink: "https://second-cert.com",
			wantErr:          nil,
		},
		{
			name: "no_compatible_cert",
			body: `{
								    "count": 2,
								    "page": 1,
								    "next": null,
								    "previous": null,
								    "results": [
								        {
								            "common_name": "test.com",
								            "added": "2019-05-31T16:30:30.646742+03:00",
								            "end_date": "2021-05-30T16:20:30+03:00",
								            "issued": "2019-05-31T16:30:30.903801+03:00",
								            "updated": "2019-05-31T16:35:02.611244+03:00",
								            "revoked": null,
								            "serial_number": "4B799D35000200075A56",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://first-cert.com",
								            "ca_name": "InternalCA",
								            "hosts": [
								                "test.com"
								            ]
								        },
								        {
								            "common_name": "test.com",
								            "added": "2019-05-29T10:38:54.565081+03:00",
								            "end_date": "2021-05-28T10:28:54+03:00",
								            "issued": "2019-05-29T10:38:54.837755+03:00",
								            "updated": "2019-05-29T10:40:12.817277+03:00",
								            "revoked": null,
								            "serial_number": "3FEAF008000200075098",
								            "priv_key_deleted_at": null,
								            "status": "issued",
								            "download2": "https://second-cert.com",
								            "ca_name": "WrongCA",
								            "hosts": [
								                "test.com",
												"test1.com",
												"test2.com"
								            ]
								        }
								    ]
								}`,

			args: args{
				ctx:      context.Background(),
				target:   "test.com",
				caName:   "InternalCA",
				altNames: []string{"test1.com", "test2.com"},
			},
			wantDownloadLink: "",
			wantErr:          crt.ErrNoCerts,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			listCertCalled := false
			listCertServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
				listCertCalled = true
				assert.Equal(t, "OAuth "+oauth.Unmask(), req.Header.Get("Authorization"), "invalid authorization header")
				w.WriteHeader(http.StatusOK)
				_, err := io.WriteString(w, tt.body)
				require.NoError(t, err)
			}))
			defer listCertServer.Close()

			c, err := New(&nop.Logger{}, Config{
				ClientConfig: configNoRetry,
				URL:          listCertServer.URL,
				OAuth:        oauth,
			})
			require.NoError(t, err)

			got, err := c.latestCert(tt.args.ctx, tt.args.target, tt.args.altNames, tt.args.caName)
			assert.True(t, listCertCalled, "list certificates not called")

			if tt.wantErr != nil && !xerrors.Is(err, tt.wantErr) {
				t.Errorf("expected %v but got %v", tt.wantErr, err)
				return
			}
			if tt.wantDownloadLink != "" {
				require.NotNil(t, got, "result is nil")
				assert.Equal(t, tt.wantDownloadLink, got.Download2)
			}
		})
	}
}
