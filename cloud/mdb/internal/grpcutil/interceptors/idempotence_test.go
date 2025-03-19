package interceptors

import (
	"encoding/hex"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1"
)

func TestHashRequest(t *testing.T) {
	inputs := []struct {
		Name     string
		Req      interface{}
		Expected string
	}{
		{
			Name: "nil",
			Req:  nil,
		},
		{
			Name: "string",
			Req:  "foo",
		},
		{
			Name: "int",
			Req:  42,
		},
		{
			Name: "struct",
			Req:  struct{ Foo string }{Foo: "foo"},
		},
		{
			Name: "struct ptr",
			Req:  &struct{ Foo string }{Foo: "foo"},
		},
		{
			Name: "proto.Message value",
			Req:  mdb.GetOperationRequest{OperationId: "foo"},
		},
		{
			Name:     "proto.Message",
			Req:      &mdb.GetOperationRequest{OperationId: "foo"},
			Expected: "3f0fabeaf1fdbc1eb80048779d018081a281f088815effa55582439668c96187",
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			hash, err := hashRequest(input.Req)
			if input.Expected == "" {
				assert.Error(t, err)
				assert.Empty(t, hash)
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, input.Expected, hex.EncodeToString(hash))
		})
	}
}
