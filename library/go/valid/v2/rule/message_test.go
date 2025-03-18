package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestMessage(t *testing.T) {
	t.Run("with_error", func(t *testing.T) {
		v := inspection.Inspect("")
		err := Message("поле не должно быть пустым", NotEmpty)(v)
		expected := &MessageErr{
			Msg: "поле не должно быть пустым",
			Err: ErrEmptyValue,
		}
		assert.Equal(t, expected, err)
	})

	t.Run("without_error", func(t *testing.T) {
		v := inspection.Inspect("ololo")
		err := Message("поле не должно быть пустым", NotEmpty)(v)
		assert.NoError(t, err)
	})
}
