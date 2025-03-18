package awstvm_test

import (
	"testing"

	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/awstvm"
)

func TestSqsCredentials(t *testing.T) {
	tvmClient := newMockTvmClient()
	cases := []struct {
		account string
		opts    []awstvm.Option
		creds   credentials.Value
	}{
		{
			account: "typical",
			creds: credentials.Value{
				AccessKeyID:     "typical",
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "ticket_for:2002456;counter:1",
			},
		},
		{
			account: "override_tvm_id",
			opts: []awstvm.Option{
				awstvm.WithTvmClientID(31337),
			},
			creds: credentials.Value{
				AccessKeyID:     "override_tvm_id",
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "ticket_for:31337;counter:1",
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.account, func(t *testing.T) {
			sqsCreds, err := awstvm.NewSqsCredentials(tvmClient, tc.account, tc.opts...)
			require.NoError(t, err)

			require.True(t, sqsCreds.IsExpired())
			value, err := sqsCreds.Get()
			require.NoError(t, err, "get auth value")

			require.EqualValues(t, tc.creds, value)
		})
	}
}
