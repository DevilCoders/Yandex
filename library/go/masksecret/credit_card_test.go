package masksecret

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestCreditCard(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expected    string
		expectedErr error
	}{
		{"empty_string", "", "", valid.ErrEmptyString},
		{"invalid_number", "foo", "", valid.ErrInvalidChecksum},
		{"invalid_checksum", "5398228707871528", "", valid.ErrInvalidChecksum},

		{"valid", "375556917985515", "37xxxxxxxxx5515", nil},
		{"valid", "36050234196908", "36xxxxxxxx6908", nil},
		{"valid", "4716461583322103", "47xxxxxxxxxx2103", nil},
		{"valid", "5398228707871527", "53xxxxxxxxxx1527", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v, err := CreditCard(tc.input)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}

			assert.Equal(t, tc.expected, v)
		})
	}
}
