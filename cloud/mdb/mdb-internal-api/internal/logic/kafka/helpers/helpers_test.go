package helpers

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestHelpers_PointerLiterals(t *testing.T) {
	t.Run("When define int64 pointer then correct", func(t *testing.T) {
		pointer := Pointer[int64](int64(3917))
		require.Equal(t, "*int64 3917", fmt.Sprintf("%T %v", pointer, *pointer))
	})

	t.Run("When define bool pointer then correct", func(t *testing.T) {
		pointer := Pointer[bool](true)
		require.Equal(t, "*bool true", fmt.Sprintf("%T %v", pointer, *pointer))
	})

	t.Run("When define string pointer then correct", func(t *testing.T) {
		pointer := Pointer[string]("blank")
		require.Equal(t, "*string blank", fmt.Sprintf("%T %v", pointer, *pointer))
	})

	t.Run("When define pointer of type _any_ then correct", func(t *testing.T) {
		pointer := Pointer[any](nil)
		require.Equal(t, "*interface {} <nil>", fmt.Sprintf("%T %v", pointer, *pointer))
	})
}
