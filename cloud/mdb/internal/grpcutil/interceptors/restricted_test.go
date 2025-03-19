package interceptors

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func TestParseGRPCFullMethod(t *testing.T) {
	fullMethod := "/yandex.cloud.priv.mdb.greenplum.v1.BackupService/Get"
	pkg, svc, method := parseGRPCFullMethod(fullMethod)
	require.Equal(t, "yandex.cloud.priv.mdb.greenplum.v1", pkg)
	require.Equal(t, "BackupService", svc)
	require.Equal(t, "Get", method)
}

func TestCheckMethodRestricted(t *testing.T) {
	cfg := RestrictedCheckerConfig{}
	meth := "/some.pkg.Service/Do"
	now := time.Now()
	require.Falsef(t, checkMethodRestricted(meth, now, cfg), "empty config don't restrict")
	cfg = RestrictedCheckerConfig{
		Rules: []Rule{{
			Package: "some.pkg",
			Service: "Service",
			Method:  "Do",
			Since:   now.Add(-time.Hour),
			Until:   now.Add(time.Hour),
		}},
	}
	require.Falsef(t, checkMethodRestricted(meth, now, cfg), "exact match with correct time don't not restrict")
	cfg.Rules[0].Since = now.Add(time.Minute)
	require.Truef(t, checkMethodRestricted(meth, now, cfg), "exact match with now < since restricts")

	cfg.Rules[0].Since = now.Add(-time.Hour)
	cfg.Rules[0].Until = now.Add(-time.Minute)
	require.Truef(t, checkMethodRestricted(meth, now, cfg), "exact match with now > after restricts")
	cfg.Rules[0].Method = ""
	require.Truef(t, checkMethodRestricted(meth, now, cfg), "no method match with now > after restricts")
	cfg.Rules[0].Service = ""
	require.Truef(t, checkMethodRestricted(meth, now, cfg), "no method,service match with now > after restricts")
	cfg.Rules[0].Package = ""
	require.Truef(t, checkMethodRestricted(meth, now, cfg), "no package,method,service match with now > after restricts")
}
