package unknownjson

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/ptr"
)

func TestRoundtrip(t *testing.T) {
	type example struct {
		A string `json:"a"`
		B string `json:",omitempty"`
		C string `json:"-"`

		Unknown map[string]json.RawMessage `json:"-" unknown:",store"`
	}

	type stringFields struct {
		A string `json:",string"`
		B int    `json:",string"`
		C bool   `json:",string"`
	}

	type pointerField struct {
		A *int `json:"a"`
		B *int `json:"b"`
		C *int `json:"c,omitempty"`
	}

	type privateStore struct {
		A string `json:"a"`
		B string `json:",omitempty"`
		C string `json:"-"`

		Unknown Store `json:"-" unknown:",store"`
	}

	type primitiveContainers struct {
		A []int
		B map[string]string
	}

	for _, testCase := range []struct {
		name  string
		js    string
		value interface{}
	}{
		{
			name:  "int",
			js:    "1",
			value: new(int),
		},
		{
			name:  "uint",
			js:    "1",
			value: new(uint),
		},
		{
			name:  "string",
			js:    `"abc"`,
			value: new(string),
		},
		{
			name:  "true",
			js:    `true`,
			value: ptr.Bool(true),
		},
		{
			name:  "false",
			js:    `false`,
			value: ptr.Bool(false),
		},
		//{
		//	name:  "interface{}",
		//	js:    `{"a":1}`,
		//	value: new(interface{}),
		//},
		{
			name:  "struct",
			js:    `{"a":"1","c":"2"}`,
			value: new(example),
		},
		{
			name:  "private",
			js:    `{"a":"1","c":"2"}`,
			value: new(privateStore),
		},
		{
			name:  "slice",
			js:    `[{"a":"1","c":"2"},{"a":"11","B":"31","c":"21"}]`,
			value: new([]example),
		},
		{
			name:  "map",
			js:    `{"first":{"a":"1","c":"2"},"second":{"a":"11","B":"31","c":"21"}}`,
			value: new(map[string]example),
		},
		{
			name:  "string fields",
			js:    `{"A": "\"a\"", "B": "1", "C": "true"}`,
			value: new(stringFields),
		},
		{
			name:  "pointer fields",
			js:    `{"a": 1, "b": null}`,
			value: new(pointerField),
		},
		{
			name:  "primitive containers",
			js:    `{"A": [1,2,3], "B": {"foo": "bar", "bar": "foo"}}`,
			value: new(primitiveContainers),
		},
	} {
		t.Run(testCase.name, func(t *testing.T) {
			require.NoError(t, Unmarshal([]byte(testCase.js), testCase.value))

			out, err := Marshal(testCase.value)
			require.NoError(t, err)

			var firstJSON, secondJSON interface{}
			require.NoError(t, json.Unmarshal(out, &firstJSON))
			require.NoError(t, json.Unmarshal([]byte(testCase.js), &secondJSON))

			require.Equal(t, firstJSON, secondJSON)
		})
	}
}

func TestFieldReset(t *testing.T) {
	var fields struct {
		A []int
		B map[string]string
	}

	fields.A = []int{1}
	fields.B = map[string]string{"foo": "bar"}

	require.NoError(t, Unmarshal([]byte(`{"A":[1,2,3],"B":{"zog":"zog","1":"2"}}`), &fields))
	require.Equal(t, 3, len(fields.A))
	require.Equal(t, 2, len(fields.B))
}
