package awstvm_test

import (
	"testing"

	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/awstvm"
)

func TestS3Credentials(t *testing.T) {
	tvmClient := newMockTvmClient()
	cases := []struct {
		name  string
		opts  []awstvm.Option
		creds credentials.Value
	}{
		{
			name: "typical",
			creds: credentials.Value{
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "TVM2 ticket_for:2017579;counter:1",
			},
		},
		{
			name: "override_tvm_id",
			opts: []awstvm.Option{
				awstvm.WithTvmClientID(awstvm.S3TestingTvmID),
			},
			creds: credentials.Value{
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "TVM2 ticket_for:2017577;counter:1",
			},
		},
		{
			name: "provide_client_name",
			opts: []awstvm.Option{
				awstvm.WithAccessKeyID("TVM_V2_31337"),
			},
			creds: credentials.Value{
				AccessKeyID:     "TVM_V2_31337",
				ProviderName:    awstvm.DefaultProviderName,
				SecretAccessKey: awstvm.DefaultSecretAccessKey,
				SessionToken:    "TVM2 ticket_for:2017579;counter:2",
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			sqsCreds, err := awstvm.NewS3Credentials(tvmClient, tc.opts...)
			require.NoError(t, err)

			require.True(t, sqsCreds.IsExpired())
			value, err := sqsCreds.Get()
			require.NoError(t, err, "get auth value")

			require.EqualValues(t, tc.creds, value)
		})
	}
}
