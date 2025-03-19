package dns_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/dns"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/dns/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestEnsureDNS(t *testing.T) {
	host := "host"
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	resolver := mocks.NewMockResolver(ctrl)
	checker := dns.NewChecker(host, dns.WithResolver(resolver))

	errResolve := xerrors.Errorf("resolve")
	resolver.EXPECT().LookupHost(host).Return(nil, errResolve).Times(2)
	resolver.EXPECT().LookupHost(host).Return(nil, nil)

	require.NoError(t, checker.EnsureDNS(context.Background(), time.Minute, time.Microsecond))
}
