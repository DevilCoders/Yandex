package functest

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestGetArrayIndex(t *testing.T) {
	index, token, _ := getArrayIndex("array[0]")
	require.Equal(t, 0, index)
	require.Equal(t, "array", token)

	index, token, _ = getArrayIndex("array")
	require.Equal(t, -1, index)
	require.Equal(t, "array", token)

	index, token, _ = getArrayIndex("[0]")
	require.Equal(t, -1, index)
	require.Equal(t, "[0]", token)
}

type FillValueTestData struct {
	rawData  string
	path     string
	value    interface{}
	expected string
}

func TestFillValueAtPath(t *testing.T) {
	testData := []FillValueTestData{
		// Edit existing fields
		{"{ \"a\": 123 }", "$.a", float64(456), "{ \"a\": 456 }"},
		{"{ \"a\": \"old_string\" }", "$.a", "new_string", "{ \"a\": \"new_string\" }"},
		{"{ \"arr\": [\"string1\", \"string2\", \"string3\"] }", "$.arr[1]", "new_string", "{ \"arr\": [\"string1\", \"new_string\", \"string3\"] }"},
		{"{ \"a\": { \"b\": { \"field_1\": \"value_1\" } } }", "$.a.b.field_1", "new_string", "{ \"a\": { \"b\": { \"field_1\": \"new_string\" } } }"},

		// Edit non-existing fields
		{"{ }", "$.a", float64(456), "{ \"a\": 456 }"},
		{"{ }", "$.a", "new_string", "{ \"a\": \"new_string\" }"},
		{"{ }", "$.arr[0]", "new_string", "{ \"arr\": [\"new_string\"] }"},
		{"{ }", "$.a.b.field_1", "new_string", "{ \"a\": { \"b\": { \"field_1\": \"new_string\" } } }"},
	}

	for _, data := range testData {
		initalData := make(map[string]interface{})
		if err := json.Unmarshal([]byte(data.rawData), &initalData); err != nil {
			require.NoError(t, err)
			return
		}

		fillValueAtPath(initalData, data.path, data.value)

		expectedResult := make(map[string]interface{})
		if err := json.Unmarshal([]byte(data.expected), &expectedResult); err != nil {
			require.NoError(t, err)
			return
		}

		require.Equal(t, expectedResult, initalData)
	}
}
