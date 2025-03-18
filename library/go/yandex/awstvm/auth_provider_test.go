package awstvm_test

import (
	"testing"

	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/awstvm"
)

func TestAuthProvider(t *testing.T) {
	cases := []struct {
		name  string
		opts  []awstvm.Option
		creds credentials.Value
		err   bool
	}{
		{
			name: "default",
			opts: []awstvm.Option{
				awstvm.WithTvmClientID(1000),
			},
			creds: credentials.Value{
				AccessKeyID:     "",
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "ticket_for:1000;counter:1",
			},
		},
		{
			name: "override_defaults",
			opts: []awstvm.Option{
				awstvm.WithTvmClientID(2000),
				awstvm.WithAccessKeyID("TestNewAuthProvider_Key"),
				awstvm.WithProviderName("TestNewAuthProvider"),
				awstvm.WithSecretAccessKey("TestNewAuthProvider_SecretAccessKey"),
			},
			creds: credentials.Value{
				AccessKeyID:     "TestNewAuthProvider_Key",
				ProviderName:    "TestNewAuthProvider",
				SecretAccessKey: "TestNewAuthProvider_SecretAccessKey",
				SessionToken:    "ticket_for:2000;counter:1",
			},
		},
		{
			name: "no_tvm",
			opts: []awstvm.Option{},
			err:  true,
		},
	}

	tvmClient := newMockTvmClient()
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			provider, err := awstvm.NewAuthProvider(tvmClient, tc.opts...)
			if tc.err {
				require.Error(t, err)
				return
			}

			require.NoError(t, err)
			require.True(t, provider.IsExpired())

			value, err := provider.Retrieve()
			require.NoError(t, err)

			require.EqualValues(t, tc.creds, value)
			require.True(t, provider.IsExpired())
		})
	}
}
