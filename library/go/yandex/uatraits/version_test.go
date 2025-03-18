package uatraits

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestParseVersion(t *testing.T) {
	t.Run("ValidValues", func(t *testing.T) {
		validVersions := []struct {
			Input           string
			ExpectedVersion Version
		}{
			{"1", Version{1, 0, 0, 0}},
			{"1.2", Version{1, 2, 0, 0}},
			{"1.20", Version{1, 20, 0, 0}},
			{"1.2.3", Version{1, 2, 3, 0}},
			{"1.2.3.4", Version{1, 2, 3, 4}},
			{"0", Version{0, 0, 0, 0}},
			{"0.99", Version{0, 99, 0, 0}},
			{"0.0.99", Version{0, 0, 99, 0}},
			{"0.0.0.99", Version{0, 0, 0, 99}},
			{"0.1.0.99", Version{0, 1, 0, 99}},
			{"0.0.2.99", Version{0, 0, 2, 99}},
			{"0.1.2.99", Version{0, 1, 2, 99}},
			{" 1.2", Version{1, 2, 0, 0}},
			{"1.2 ", Version{1, 2, 0, 0}},
			{" 1.2 ", Version{1, 2, 0, 0}},
		}
		for _, item := range validVersions {
			parsedVersion, err := parseVersion(item.Input)
			if assert.NoError(t, err) {
				assert.Equal(t, item.ExpectedVersion, parsedVersion, item.Input)
			}
		}
	})
	t.Run("IncorrectValues", func(t *testing.T) {
		incorrectValues := []string{
			"-1",
			"1.-2",
			"1..2",
			"1.2.3.4.5",
			".1.2",
		}
		for _, item := range incorrectValues {
			_, err := parseVersion(item)
			assert.Error(t, err, item)
		}
	})
}

func TestCompareVersions(t *testing.T) {
	t.Run("Equal", func(t *testing.T) {
		v1 := Version{2, 12, 85, 0}
		v2 := Version{2, 12, 85, 0}

		assert.Equal(t, 0, v1.CompareTo(v2))
		assert.Equal(t, 0, v2.CompareTo(v1))
	})
	t.Run("LessOrGreater", func(t *testing.T) {
		v0 := Version{1, 2, 3, 4}

		v1 := Version{1, 2, 3, 5}
		v2 := Version{1, 2, 4, 0}
		v3 := Version{1, 3, 0, 0}
		v4 := Version{2, 0, 0, 0}

		assert.Equal(t, -1, v0.CompareTo(v1))
		assert.Equal(t, 1, v1.CompareTo(v0))

		assert.Equal(t, -1, v0.CompareTo(v2))
		assert.Equal(t, 1, v2.CompareTo(v0))

		assert.Equal(t, -1, v0.CompareTo(v3))
		assert.Equal(t, 1, v3.CompareTo(v0))

		assert.Equal(t, -1, v0.CompareTo(v4))
		assert.Equal(t, 1, v4.CompareTo(v0))
	})
}
