package rule

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestIsRFC3339(t *testing.T) {
	testCases := []struct {
		val         string
		expectedErr error
	}{
		{"2006-01-02T13:37:42Z", nil},
		{"2006-01-02T13:37:42,326Z", nil},
		{"2006-01-02T13:37:42.0Z", nil},
		{"2006-01-02T13:37:42.0000Z", nil},
		{"2006-01-02T13:37:42,326876Z", nil},
		{"2006-01-02T13:37:42,326876123Z", nil},
		{"2006-01-02T13:37:42.326876123Z", nil},
		{"2006-01-02T13:37:42,326+08:00", nil},
		{"2006-01-02T13:37:42.326-08:00", nil},
		{"2006-01-02T13:37:42.326-08:21", nil},
		{"2006-01-02T13:37:42.326+08:21", nil},
		{"2021-09-30T08:28:33.137578Z", nil},

		{"2006", ErrStringTooShort},
		{"2006-01-02T13:37:42", ErrStringTooShort},
		{"2006-01-02T13:37:42,326", ErrInvalidCharsSequence},
		{"2006:01-02T13:37:42Z", ErrInvalidCharsSequence},
		{"2006-01:02T13:37:42Z", ErrInvalidCharsSequence},
		{"2006-01-02 13:37:42Z", ErrInvalidCharsSequence},
		{"2006-01-02T13-37:42Z", ErrInvalidCharsSequence},
		{"2006-01-02T13:37-42Z", ErrInvalidCharsSequence},
		{"200a-01-02T13:37:42Z", ErrInvalidCharsSequence},
		{"2006-0b-02T13:37:42Z", ErrInvalidCharsSequence},
		{"2006-01-0cT13:37:42Z", ErrInvalidCharsSequence},
		{"2006-01-02T1d:37:42Z", ErrInvalidCharsSequence},
		{"2006-01-02T13:3e:42Z", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:4fZ", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42.727", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42x08:00", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42+08x00", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42+0a:00", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42+08:0a", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42+08:0", ErrInvalidCharsSequence},
		{"2006-01-02T13:37:42+08:00hello", ErrInvalidCharsSequence},
		{"2006-01-02§13:37:42+08:00", ErrInvalidCharsSequence},
	}

	for _, tc := range testCases {
		t.Run(tc.val, func(t *testing.T) {
			v := inspection.Inspect(tc.val)
			err := IsRFC3339(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func TestIsRFC3339_Time(t *testing.T) {
	testCases := []struct {
		val         interface{}
		expectedErr error
	}{
		{time.Time{}, nil},
		{&time.Time{}, nil},
	}

	for _, tc := range testCases {
		v := inspection.Inspect(tc.val)
		err := IsRFC3339(v)
		assert.Equal(t, tc.expectedErr, err)
	}
}

func BenchmarkIsRFC3339(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect(""),
		inspection.Inspect("2006"),
		inspection.Inspect("2006-01-02T13:37:42"),
		inspection.Inspect("2006-01-02T13:37:42,326"),
		inspection.Inspect("2006:01-02T13:37:42Z"),
		inspection.Inspect("2006-01:02T13:37:42Z"),
		inspection.Inspect("2006-01-02 13:37:42Z"),
		inspection.Inspect("2006-01-02T13-37:42Z"),
		inspection.Inspect("2006-01-02T13:37-42Z"),
		inspection.Inspect("200a-01-02T13:37:42Z"),
		inspection.Inspect("2006-0b-02T13:37:42Z"),
		inspection.Inspect("2006-01-0cT13:37:42Z"),
		inspection.Inspect("2006-01-02T1d:37:42Z"),
		inspection.Inspect("2006-01-02T13:3e:42Z"),
		inspection.Inspect("2006-01-02T13:37:4fZ"),
		inspection.Inspect("2006-01-02T13:37:42.727"),
		inspection.Inspect("2006-01-02T13:37:42x08:00"),
		inspection.Inspect("2006-01-02T13:37:42+08x00"),
		inspection.Inspect("2006-01-02T13:37:42+0a:00"),
		inspection.Inspect("2006-01-02T13:37:42+08:0a"),
		inspection.Inspect("2006-01-02T13:37:42+08:0"),
		inspection.Inspect("2006-01-02T13:37:42+08:00hello"),
		inspection.Inspect("2006-01-02§13:37:42+08:00"),
		inspection.Inspect("2006-01-02T13:37:42Z"),
		inspection.Inspect("2006-01-02T13:37:42,326Z"),
		inspection.Inspect("2006-01-02T13:37:42.0Z"),
		inspection.Inspect("2006-01-02T13:37:42.0000Z"),
		inspection.Inspect("2006-01-02T13:37:42,326876Z"),
		inspection.Inspect("2006-01-02T13:37:42,326876123Z"),
		inspection.Inspect("2006-01-02T13:37:42.326876123Z"),
		inspection.Inspect("2006-01-02T13:37:42,326+08:00"),
		inspection.Inspect("2006-01-02T13:37:42.326-08:00"),
		inspection.Inspect("2006-01-02T13:37:42.326-08:21"),
		inspection.Inspect("2006-01-02T13:37:42.326+08:21"),
		inspection.Inspect("2021-09-30T08:28:33.137578Z"),
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = IsRFC3339(testCases[i%len(testCases)])
	}
}
