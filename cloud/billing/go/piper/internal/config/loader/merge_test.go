package loader

import (
	"context"
	"encoding/json"
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestConfigMerge(t *testing.T) {
	value := mConfig{
		DefaultKey: "default",
		MapKey: map[string]*nestedMerge{
			"default":  {Override: "default", Keep: "keep"},
			"override": {Override: "default", Keep: "keep"},
		},
		Nested:    nestedMerge{Override: "default", Keep: "keep"},
		NestedPtr: &nestedMerge{Override: "default", Keep: "keep"},
	}

	backend := &jsonBackend{data: `{
"Key": "value",
"DefaultKey": "overridden key",
"MapKey":{
  "override":{"Override": "overridden"},
  "new":{"Keep": "new"}
},
"Nested":{"Override": "overridden"},
"NestedPtr":{"Override": "overridden"}
}`}

	wanted := mConfig{
		Key:        "value",
		DefaultKey: "overridden key",
		MapKey: map[string]*nestedMerge{
			"default":  {Override: "default", Keep: "keep"},
			"override": {Override: "overridden", Keep: "keep"},
			"new":      {Keep: "new"},
		},
		Nested:    nestedMerge{Override: "overridden", Keep: "keep"},
		NestedPtr: &nestedMerge{Override: "overridden", Keep: "keep"},
	}

	mrg := MergeBackend{backend}
	err := mrg.Unmarshal(context.Background(), &value)
	assert.NoError(t, err)

	assert.EqualValues(t, wanted, value)
}

type nestedMerge struct {
	Override string
	Keep     string
}

type mConfig struct {
	Key        string
	DefaultKey string
	Nested     nestedMerge
	NestedPtr  *nestedMerge
	MapKey     map[string]*nestedMerge
}

type jsonBackend struct {
	data string
}

func (b *jsonBackend) Unmarshal(_ context.Context, to interface{}) error {
	return json.Unmarshal([]byte(b.data), to)
}

func (b *jsonBackend) Get(context.Context, string) ([]byte, error) {
	return nil, errors.New("not implemented")
}

func (b *jsonBackend) Name() string {
	return "yaml"
}
