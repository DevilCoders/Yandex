package govendor

import (
	"bytes"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/yatest"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

const testPolicy = `
ALLOW vendor/ -> .*

ALLOW .* -> vendor/github.com/heetch/confita/backend/env
ALLOW .* -> vendor/github.com/heetch/confita/backend/file
ALLOW .* -> vendor/github.com/heetch/confita/backend/flags
DENY .* -> vendor/github.com/heetch/confita/backend/
ALLOW .* -> vendor/github.com/heetch/confita
# disable tests because of unwanted dependency
DENY .* -> vendor/github.com/heetch/confita;test

ALLOW .* -> vendor/github.com/aws/aws-sdk-go/aws
ALLOW .* -> vendor/github.com/aws/aws-sdk-go/service/s3
ALLOW .* -> vendor/github.com/aws/aws-sdk-go/service/sqs

# CONTRIB-1496 RTP/RTCP stack for Go. responsible: rmcf@
ALLOW yabs/telephony/platform/internal/rtp -> vendor/github.com/wernerd/GoRTP

# SUBBOTNIK-90
ALLOW infra/rtc/instance_resolver/pkg/clients/iss3 -> vendor/git.apache.org/thrift.git/lib/go

DENY .* -> vendor/
`

func TestPolicyRoots(t *testing.T) {
	directives, err := yamake.ParsePolicy(bytes.NewBufferString(testPolicy))
	require.NoError(t, err)

	p, err := preparePolicy(directives)
	require.NoError(t, err)

	for _, allowed := range []string{
		"git.apache.org/thrift.git/lib/go",
		"github.com/heetch/confita/backend/env",
		"github.com/heetch/confita/backend",
		"github.com/heetch/confita",
		"github.com/aws/aws-sdk-go/aws",
		"github.com/aws/aws-sdk-go/aws/credentials",
		"github.com/aws/aws-sdk-go/aws/service/s3",
		"github.com/aws/aws-sdk-go/aws/service/sqs",
	} {
		require.Truef(t, p.allows(allowed), "package %s must be allowed", allowed)
	}

	for _, denied := range []string{
		"github.com/sirupsen/logrus",
		"github.com/heetch/confita/backend/etcd",
		"github.com/aws/aws-sdk-go/service/route53",
	} {
		require.Falsef(t, p.allows(denied), "package %s must be denied", denied)
	}

	require.False(t, p.allowsTests("github.com/heetch/confita/backend/env"))
	require.False(t, p.allowsTests("github.com/heetch/confita"))
}

func TestRealPolicy(t *testing.T) {
	path := yatest.SourcePath("build/rules/go/vendor.policy")

	directives, err := yamake.ReadPolicy(path)
	require.NoError(t, err)

	_, err = preparePolicy(directives)
	require.NoError(t, err)
}

func BenchmarkPolicy(b *testing.B) {
	path := yatest.SourcePath("build/rules/go/vendor.policy")

	directives, err := yamake.ReadPolicy(path)
	require.NoError(b, err)

	p, err := preparePolicy(directives)
	require.NoError(b, err)

	for i := 0; i < b.N; i++ {
		require.True(b, p.allows("github.com/aws/aws-sdk-go/aws/service/s3"))
	}
}
