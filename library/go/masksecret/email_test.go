package masksecret

import (
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestEmail(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expected    string
		expectedErr error
	}{
		{
			"non_email",
			"some_string",
			"some_string",
			errors.New("not a valid email"),
		},
		{
			"valid_email_short_name",
			"me@yandex.ru",
			"xx@yandex.ru",
			nil,
		},
		{
			"valid_email_long_name",
			"mymail@yandex.ru",
			"mxxxxl@yandex.ru",
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v, err := Email(tc.input)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}

			assert.Equal(t, tc.expected, v)
		})
	}
}
