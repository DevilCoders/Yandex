package pagination

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestPagination_NewPage(t *testing.T) {
	a := []int{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
	total := int64(len(a))

	t.Run("01.Full", func(t *testing.T) {
		expected := a

		page := NewPage(total, total, 0)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("02.ExceededLimit", func(t *testing.T) {
		expected := a

		page := NewPage(total, 100, 0)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("03.Half", func(t *testing.T) {
		expected := []int{1, 2, 3, 4, 5}

		page := NewPage(total, total/2, 0)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("04.OffsetHalf", func(t *testing.T) {
		expected := []int{6, 7, 8, 9, 10}

		page := NewPage(total, total, total/2)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("05.NegativeOffsetFixed", func(t *testing.T) {
		expected := a

		page := NewPage(total, total, -5)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("06.NegativePageSizeSetToDefault", func(t *testing.T) {
		expected := a

		page := NewPage(total, -5, 0)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})

	t.Run("07.ZeroPageSizeSetToDefault", func(t *testing.T) {
		expected := a

		page := NewPage(total, 0, 0)
		actual := a[page.LowerIndex:page.UpperIndex]

		t.Logf("\nExpected:\t%v\nActual:\t\t%v\n", expected, actual)

		require.Equal(t, expected, actual)
	})
}
