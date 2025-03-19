package secret

import (
	"encoding/json"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/stretchr/testify/require"
)

func TestMaskKeysInObject(t *testing.T) {
	inputs := []struct {
		Name     string
		JSON     string
		Expected string
	}{
		{
			Name:     "EmptyObject",
			JSON:     `{}`,
			Expected: `{}`,
		},
		{
			Name:     "EmptyArray",
			JSON:     `[]`,
			Expected: `[]`,
		},
		{
			Name: "String",
			JSON: `{
	"password": "PWD",
	"string": "str"
}`,
			Expected: `{
	"password": "xxx",
	"string": "str"
}`,
		},
		{
			Name: "Integer",
			JSON: `{
	"integer": 24,
	"password": 42
}`,
			Expected: `{
	"integer": 24,
	"password": 0
}`,
		},
		{
			Name: "SimpleArray",
			JSON: `{
	"password": [
		"PASSWORD"
	]
}`,
			Expected: `{
	"password": [
		"xxx"
	]
}`,
		},
		{
			Name: "NestedObject",
			JSON: `{
	"password": {
		"integer": 42,
		"string": "PASSWORD"
	}
}`,
			Expected: `{
	"password": {
		"integer": 0,
		"string": "xxx"
	}
}`,
		},
		{
			Name: "NestedArray",
			JSON: `{
	"password": [
		{
			"integer": 42,
			"string": "PASSWORD"
		}
	]
}`,
			Expected: `{
	"password": [
		{
			"integer": 0,
			"string": "xxx"
		}
	]
}`,
		},
		{
			Name: "ComplexObject",
			JSON: `{
	"object": {
		"password": {
			"array": [
				{
					"integer": 60,
					"string": "SECRET"
				}
			],
			"bar": "PWD",
			"foo": 51,
			"password": "PASSWORD"
		}
	}
}`,
			Expected: `{
	"object": {
		"password": {
			"array": [
				{
					"integer": 0,
					"string": "xxx"
				}
			],
			"bar": "xxx",
			"foo": 0,
			"password": "xxx"
		}
	}
}`,
		},
		{
			Name: "ComplexArray",
			JSON: `{
	"array": {
		"password": [
			{
				"array": [
					{
						"integer": 60,
						"string": "SECRET"
					}
				],
				"bar": "PWD",
				"foo": 51,
				"password": "PASSWORD"
			}
		]
	}
}`,
			Expected: `{
	"array": {
		"password": [
			{
				"array": [
					{
						"integer": 0,
						"string": "xxx"
					}
				],
				"bar": "xxx",
				"foo": 0,
				"password": "xxx"
			}
		]
	}
}`,
		},
		{
			Name: "ObjectWithNull",
			JSON: `{
	"password": null
}`,
			Expected: `{
	"password": null
}`,
		},
		{
			Name: "ArrayWithNull",
			JSON: `{
	"password": [
		null,
		"PASSWORD"
	]
}`,
			Expected: `{
	"password": [
		null,
		"xxx"
	]
}`,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var v interface{}
			require.NoError(t, json.Unmarshal([]byte(input.JSON), &v))
			maskKeysInObject(v, DefaultSecretKeys)
			res, err := json.MarshalIndent(v, "", "\t")
			require.NoError(t, err)
			diff := cmp.Diff(input.Expected, string(res))
			if len(diff) != 0 {
				t.Errorf("output does not much expectation:\n%s", diff)
			}
		})
	}
}

func TestMaskKeysInJSON(t *testing.T) {
	input := `{
	"password": "PWD",
	"string": "str"
}`
	expected := `{
	"password": "xxx",
	"string": "str"
}`
	res, err := MaskKeysInJSON([]byte(input), DefaultSecretKeys)
	require.NoError(t, err)
	require.Equal(t, expected, string(res))
}
