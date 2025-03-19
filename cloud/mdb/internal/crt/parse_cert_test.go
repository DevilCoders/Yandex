package crt

import (
	"crypto/x509"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crt/fixtures"
)

func Test_calcExpiration(t *testing.T) {
	var earliest, latest time.Time

	earliest, _ = time.Parse("2006-01-02", "2006-01-02")
	latest, _ = time.Parse("2006-01-02", "2010-01-02")

	type args struct {
		certs []*x509.Certificate
	}
	tests := []struct {
		name string
		args args
		want time.Time
	}{
		{
			name: "single cert",
			args: args{
				certs: []*x509.Certificate{{
					NotAfter: earliest,
				}},
			},
			want: earliest,
		},
		{
			name: "first latest",
			args: args{
				certs: []*x509.Certificate{
					{
						NotAfter: latest,
					},
					{
						NotAfter: earliest,
					},
				},
			},
			want: earliest,
		},
		{
			name: "first earliest",
			args: args{
				certs: []*x509.Certificate{
					{
						NotAfter: earliest,
					},
					{
						NotAfter: latest,
					},
				},
			},
			want: earliest,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := calcExpiration(tt.args.certs); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("calcExpiration() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestParseCert(t *testing.T) {
	initialPem := fixtures.FullPem

	parsedCert, err := ParseCert([]byte(initialPem))

	require.NoError(t, err)
	require.True(t, strings.Contains(string(parsedCert.KeyPem), fixtures.PrivateKey))
	require.True(t, strings.Contains(string(parsedCert.CertPem), fixtures.FirstCert))
	require.True(t, strings.Contains(string(parsedCert.CertPem), fixtures.SecondCert))
	require.True(t, strings.Contains(string(parsedCert.CertPem), fixtures.ThirdCert))
	require.Equal(t, fixtures.CertEndDate.UTC(), parsedCert.Expiration.UTC())
}
