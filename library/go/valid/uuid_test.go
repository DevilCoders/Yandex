package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestUUID(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"934859", valid.ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", valid.ErrInvalidStringLength},
		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3xxx", valid.ErrInvalidStringLength},
		{"a987fbc94bed3078cf079141ba07c9f3", valid.ErrInvalidStringLength},
		{"987fbc9-4bed-3078-cf07a-9141ba07c9f3", valid.ErrInvalidCharsSequence},
		{"aaaaaaaa-1111-1111-aaag-111111111111", valid.ErrInvalidCharacters},

		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", nil},
		{"57b73598-8764-4ad0-a76a-679bb6640eb1", nil},
		{"625e63f3-58f5-40b7-83a1-a72ad31acffb", nil},
		{"987fbc97-4bed-5078-af07-9141ba07c9f3", nil},
		{"987fbc97-4bed-5078-9f07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.UUID(tc.param))
		})
	}
}

func TestUUIDv3(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"412452646", valid.ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", valid.ErrInvalidStringLength},
		{"a987fbc9-4bed-4078-8f07-9141ba07c9f3", valid.ErrInvalidCharsSequence},

		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.UUIDv3(tc.param))
		})
	}
}

func TestUUIDv4(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"934859", valid.ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", valid.ErrInvalidStringLength},
		{"a987fbc9-4bed-5078-af07-9141ba07c9f3", valid.ErrInvalidCharsSequence},

		{"57b73598-8764-4ad0-a76a-679bb6640eb1", nil},
		{"625e63f3-58f5-40b7-83a1-a72ad31acffb", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.UUIDv4(tc.param))
		})
	}
}

func TestUUIDv5(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{

		{"", valid.ErrInvalidStringLength},
		{"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3", valid.ErrInvalidStringLength},
		{"9c858901-8a57-4791-81fe-4c455b099bc9", valid.ErrInvalidCharsSequence},
		{"a987fbc9-4bed-3078-cf07-9141ba07c9f3", valid.ErrInvalidCharsSequence},

		{"987fbc97-4bed-5078-af07-9141ba07c9f3", nil},
		{"987fbc97-4bed-5078-9f07-9141ba07c9f3", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.UUIDv5(tc.param))
		})
	}
}

func BenchmarkUUID(b *testing.B) {
	benchCases := []string{
		"xxxa987fbc9-4bed-3078-cf07-9141ba07c9f3",
		"a987fbc9-4bed-3078-cf07-9141ba07c9f3xxx",
		"a987fbc94bed3078cf079141ba07c9f3",
		"934859",
		"987fbc9-4bed-3078-cf07a-9141ba07c9f3",
		"aaaaaaaa-1111-1111-aaag-111111111111",
		"a987fbc9-4bed-3078-cf07-9141ba07c9f3",
		"57b73598-8764-4ad0-a76a-679bb6640eb1",
		"625e63f3-58f5-40b7-83a1-a72ad31acffb",
		"987fbc97-4bed-5078-af07-9141ba07c9f3",
		"987fbc97-4bed-5078-9f07-9141ba07c9f3",
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = valid.UUID(benchCases[i%len(benchCases)])
	}
}
