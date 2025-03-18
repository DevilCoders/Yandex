package valid

import (
	"testing"
	"time"

	"github.com/mitchellh/copystructure"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type User struct {
	Name       string
	Surname    string
	Patronymic string
	Account    Account
	Aliases    []string
}

type Account struct {
	Login      string
	Password   string
	LastSignIn time.Time
}

func (a *Account) Validate() error {
	return Struct(a,
		Value(&a.Login, rule.Required, rule.IsAlphanumeric),
		Value(&a.Password, rule.Required, rule.IsAlphanumeric, rule.Len(6, -1)),
		Value(&a.LastSignIn, rule.Required),
	)
}

func TestValidate_Struct(t *testing.T) {
	user := User{
		Name:       "shimba",
		Surname:    "boomba",
		Patronymic: "",
		Account: Account{
			Login:      "looken",
			Password:   "toooooooooooken",
			LastSignIn: time.Now(),
		},
		Aliases: []string{"pete", "peter", "peteman"},
	}

	t.Run("valid", func(t *testing.T) {
		c, err := copystructure.Copy(user)
		require.NoError(t, err)
		u := c.(User)

		err = Struct(&u,
			Value(&u.Name, rule.Required, rule.IsAlpha),
			Value(&u.Surname, rule.Required, rule.IsAlpha),
			Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
			Value(&u.Account),
			Value(&u.Aliases, rule.NotEmpty, rule.Each(rule.NotEmpty)),
		)

		assert.NoError(t, err)
	})

	t.Run("invalid_name", func(t *testing.T) {
		c, err := copystructure.Copy(user)
		require.NoError(t, err)

		u := c.(User)
		u.Name = "sh1mba"

		invalidField := inspection.Inspect(u).Fields[0].Field

		err = Struct(&u,
			Value(&u.Name, rule.Required, rule.IsAlpha),
			Value(&u.Surname, rule.Required, rule.IsAlpha),
			Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
			Value(&u.Account),
			Value(&u.Aliases, rule.NotEmpty, rule.Each(rule.NotEmpty)),
		)

		expected := rule.Errors{
			rule.NewFieldError(&invalidField, rule.ErrInvalidCharacters),
		}

		assert.Equal(t, expected, err)
	})

	t.Run("invalid_optional", func(t *testing.T) {
		c, err := copystructure.Copy(user)
		require.NoError(t, err)

		u := c.(User)
		u.Patronymic = "l00ken"

		invalidField := inspection.Inspect(u).Fields[2].Field

		err = Struct(&u,
			Value(&u.Name, rule.Required, rule.IsAlpha),
			Value(&u.Surname, rule.Required, rule.IsAlpha),
			Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
			Value(&u.Account),
			Value(&u.Aliases, rule.NotEmpty, rule.Each(rule.NotEmpty)),
		)

		expected := rule.Errors{
			rule.NewFieldError(&invalidField, rule.ErrInvalidCharacters),
		}

		assert.Equal(t, expected, err)
	})

	t.Run("invalid_validator", func(t *testing.T) {
		c, err := copystructure.Copy(user)
		require.NoError(t, err)

		u := c.(User)
		u.Account.Password = "123"

		invalidUserField := inspection.Inspect(u).Fields[3].Field
		invalidAccountField := inspection.Inspect(u.Account).Fields[1].Field

		err = Struct(&u,
			Value(&u.Name, rule.Required, rule.IsAlpha),
			Value(&u.Surname, rule.Required, rule.IsAlpha),
			Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
			Value(&u.Account),
			Value(&u.Aliases, rule.NotEmpty, rule.Each(rule.NotEmpty)),
		)

		expected := rule.Errors{
			rule.NewFieldError(
				&invalidUserField,
				rule.NewFieldError(&invalidAccountField, rule.ErrInvalidLength),
			),
		}

		assert.Equal(t, expected, err)
	})

	t.Run("invalid_iter", func(t *testing.T) {
		c, err := copystructure.Copy(user)
		require.NoError(t, err)

		u := c.(User)
		u.Aliases = []string{"pete", "", "peteman"}

		invalidField := inspection.Inspect(u).Fields[4].Field

		err = Struct(&u,
			Value(&u.Name, rule.Required, rule.IsAlpha),
			Value(&u.Surname, rule.Required, rule.IsAlpha),
			Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
			Value(&u.Account),
			Value(&u.Aliases, rule.NotEmpty, rule.Each(rule.NotEmpty)),
		)

		expected := rule.Errors{
			rule.NewFieldError(
				&invalidField,
				rule.ErrEmptyValue,
			),
		}

		assert.Equal(t, expected, err)
	})
}
