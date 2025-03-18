package rule

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestFieldError_Error(t *testing.T) {
	type Account struct {
		Login      string
		Password   string
		LastSignIn time.Time
	}

	type User struct {
		Name       string
		Surname    string
		Patronymic string
		Account    Account
		Aliases    []string
	}

	iu := inspection.Inspect(User{})
	ia := inspection.Inspect(Account{})

	err := NewFieldError(
		&iu.Fields[3].Field,
		NewFieldError(
			&ia.Fields[1].Field,
			ErrInvalidLength,
		),
	)

	assert.EqualError(t, err, "invalid length")
	assert.Equal(t, "Account.Password: invalid length", fmt.Sprintf("%+v", err))
}
