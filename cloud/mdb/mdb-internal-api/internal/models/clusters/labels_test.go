package clusters

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestLabels_Validate(t *testing.T) {
	const fullValid = "az-_./\\@0123456789az"
	for _, input := range []Labels{
		{fullValid: fullValid},
		{"a": "a"},
		{"z": "z"},
		{"foo": "foo1"},
		{"foo": "1"},
	} {
		t.Run(fmt.Sprintf("Valid_%+v", input), func(t *testing.T) {
			require.NoError(t, input.Validate())
		})
	}

	const longInvalid = "abc0123456789012345678901234567890123456789012345678901234567890123456789"
	for _, input := range []Labels{
		{"1": "foo"},
		{"ю": "foo"},
		{"foo1": "ю"},
		{"a*": "foo"},
		{"foo": "a*"},
		{"foo": ""},
		{"": "foo"},
		{longInvalid: "foo"},
		{"foo": longInvalid},
	} {
		t.Run(fmt.Sprintf("Invalid_%+v", input), func(t *testing.T) {
			require.Error(t, input.Validate())
		})
	}
}
