package valid

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

func TestValidate_Validator(t *testing.T) {
	allValid := Account{
		Login:      "looken",
		Password:   "toooooooooooken",
		LastSignIn: time.Now(),
	}

	badLastSignin := Account{
		Login:      "looken",
		Password:   "toooooooooooken",
		LastSignIn: time.Time{},
	}
	badField := inspection.Inspect(badLastSignin).Fields[2]

	testCases := []struct {
		name     string
		target   Account
		expected error
	}{
		{
			name:     "no_errors",
			target:   allValid,
			expected: nil,
		},
		{
			name:   "bad_last_signin",
			target: badLastSignin,
			expected: rule.Errors{
				rule.NewFieldError(&badField.Field, rule.ErrRequired),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			// check Validate method called even without rules provided
			err := Value(&tc.target).Validate()
			assert.Equal(t, tc.expected, err)
		})
	}
}
