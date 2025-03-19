package juggler_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func TestJugglerReachabilityChecker(t *testing.T) {
	const (
		UNREACHABLE = "UNREACHABLE"
		fqdn1       = "man.db.yandex.net"
		threshold   = time.Hour
	)
	now := time.Date(2020, 12, 10, 10, 45, 0, 0, time.UTC)
	type input struct {
		fqdns     []string
		jEvents   func() ([]juggler.RawEvent, error)
		threshold time.Duration
	}
	type expect struct {
		result   juggler2.FQDNGroupByJugglerCheck
		checkErr func(err error) bool
	}
	type testCase struct {
		name   string
		input  input
		expect expect
	}
	tcs := []testCase{{
		name: "reachable",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{{
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "OK",
					ReceivedTime: now,
				}, {
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "CRIT",
					ReceivedTime: now.Add(-threshold - time.Second),
				}}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				OK: []string{fqdn1},
			},
		},
	}, {
		name: "unreachable",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{{
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "OK",
					ReceivedTime: now,
				}, {
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "CRIT",
					ReceivedTime: now,
				}}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				NotOK: []string{fqdn1},
			},
		},
	}, {
		name: "stale events",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{
					{
						Host:         fqdn1,
						Service:      UNREACHABLE,
						Status:       "OK",
						ReceivedTime: now.Add(-threshold - time.Second),
					},
				}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				NotOK: []string{fqdn1},
			},
		},
	}, {
		name: "no events",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				NotOK: []string{fqdn1},
			},
		},
	}, {
		name: "empty list",
		input: input{
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				NotOK: []string(nil),
				OK:    []string(nil),
			},
		},
	}, {
		name: "two fqnds one reachable one not",
		input: input{
			fqdns: []string{fqdn1, "sas1.db.yandex.net"},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{{
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "OK",
					ReceivedTime: now,
				}, {
					Host:         fqdn1,
					Service:      UNREACHABLE,
					Status:       "CRIT",
					ReceivedTime: now,
				}, {
					Host:         "sas1.db.yandex.net",
					Service:      UNREACHABLE,
					Status:       "OK",
					ReceivedTime: now,
				}}, nil
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{
				NotOK: []string{fqdn1},
				OK:    []string{"sas1.db.yandex.net"},
			},
		},
	}, {
		name: "juggler error",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{}, semerr.Authorization("auth err")
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{},
			checkErr: func(err error) bool {
				return semerr.IsAuthorization(err)
			},
		},
	}, {
		name: "juggler unavailable",
		input: input{
			fqdns: []string{fqdn1},
			jEvents: func() ([]juggler.RawEvent, error) {
				return []juggler.RawEvent{}, semerr.Unavailable("unavailable")
			},
		},
		expect: expect{
			result: juggler2.FQDNGroupByJugglerCheck{},
			checkErr: func(err error) bool {
				return semerr.IsUnavailable(err)
			},
		},
	},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			jglr := jugglermock.NewMockAPI(ctrl)
			jglr.EXPECT().RawEvents(ctx, tc.input.fqdns, []string{UNREACHABLE}).Return(tc.input.jEvents())

			checker := juggler2.NewCustomJugglerChecker(jglr, tc.input.threshold)
			result, err := checker.Check(ctx, tc.input.fqdns, []string{juggler2.Unreachable}, now)

			expected := tc.expect
			if expected.checkErr == nil {
				require.NoError(t, err)
				require.Equal(t, expected.result, result)
			} else {
				require.Error(t, err)
				require.True(t, expected.checkErr(err))
			}
		})
	}
}
