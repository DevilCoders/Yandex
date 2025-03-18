package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestIsUUID(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", ErrInvalidStringLength},
		{"934859", ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", ErrInvalidStringLength},
		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3xxx", ErrInvalidStringLength},
		{"a987fbc94bed3078cf079141ba07c9f3", ErrInvalidStringLength},
		{"987fbc9-4bed-3078-cf07a-9141ba07c9f3", ErrInvalidCharsSequence},
		{"aaaaaaaa-1111-1111-aaag-111111111111", ErrInvalidCharacters},

		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", nil},
		{"57b73598-8764-4ad0-a76a-679bb6640eb1", nil},
		{"625e63f3-58f5-40b7-83a1-a72ad31acffb", nil},
		{"987fbc97-4bed-5078-af07-9141ba07c9f3", nil},
		{"987fbc97-4bed-5078-9f07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, IsUUID(v))
		})
	}
}

func TestUUIDv3(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", ErrInvalidStringLength},
		{"412452646", ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", ErrInvalidStringLength},
		{"a987fbc9-4bed-4078-8f07-9141ba07c9f3", ErrInvalidCharsSequence},

		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, IsUUIDv3(v))
		})
	}
}

func TestUUIDv4(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", ErrInvalidStringLength},
		{"934859", ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", ErrInvalidStringLength},
		{"a987fbc9-4bed-5078-af07-9141ba07c9f3", ErrInvalidCharsSequence},

		{"57b73598-8764-4ad0-a76a-679bb6640eb1", nil},
		{"625e63f3-58f5-40b7-83a1-a72ad31acffb", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, IsUUIDv4(v))
		})
	}
}

func TestUUIDv5(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{

		{"", ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", ErrInvalidStringLength},
		{"9c858901-8a57-4791-81fe-4c455b099bc9", ErrInvalidCharsSequence},
		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", ErrInvalidCharsSequence},

		{"987fbc97-4bed-5078-af07-9141ba07c9f3", nil},
		{"987fbc97-4bed-5078-9f07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, IsUUIDv5(v))
		})
	}
}

func BenchmarkUUID(b *testing.B) {
	cases := []*inspection.Inspected{
		inspection.Inspect(""),
		inspection.Inspect("934859"),
		inspection.Inspect("xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3"),
		inspection.Inspect("a987fbc9-4bed-3078-cf07-9141ba07c9f3xxx"),
		inspection.Inspect("a987fbc94bed3078cf079141ba07c9f3"),
		inspection.Inspect("987fbc9-4bed-3078-cf07a-9141ba07c9f3"),
		inspection.Inspect("aaaaaaaa-1111-1111-aaag-111111111111"),
		inspection.Inspect("a987fbc9-4bed-3078-cf07-9141ba07c9f3"),
		inspection.Inspect("57b73598-8764-4ad0-a76a-679bb6640eb1"),
		inspection.Inspect("625e63f3-58f5-40b7-83a1-a72ad31acffb"),
		inspection.Inspect("987fbc97-4bed-5078-af07-9141ba07c9f3"),
		inspection.Inspect("987fbc97-4bed-5078-9f07-9141ba07c9f3"),
	}

	ruleFuncs := map[string]Rule{
		"uuid":  IsUUID,
		"uuid3": IsUUIDv3,
		"uuid4": IsUUIDv4,
		"uuid5": IsUUIDv5,
	}

	for name, fn := range ruleFuncs {
		b.Run(name, func(b *testing.B) {
			b.ReportAllocs()
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				_ = fn(cases[i%len(cases)])
			}
		})
	}
}
