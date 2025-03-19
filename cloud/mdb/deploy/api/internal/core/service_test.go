package core_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/core"
)

func TestJobResultBlacklist(t *testing.T) {
	bl := core.JobResultBlacklist{
		Blacklist: []core.BlacklistedJobResult{
			{Func: "foo", Args: []string{"one"}},
			{Func: "bar"},
			{Args: []string{"one", "two"}},
		},
	}
	var inputs = []struct {
		Func string
		Args []string
		Res  bool
	}{
		{
			Func: "foo",
			Args: []string{"one"},
			Res:  true,
		},
		{
			Args: []string{"one"},
			Res:  false,
		},
		{
			Func: "foo",
			Res:  false,
		},
		{
			Func: "bar",
			Args: []string{"two"},
			Res:  true,
		},
		{
			Func: "bar",
			Args: []string{"one"},
			Res:  true,
		},
		{
			Func: "foo",
			Args: []string{"one", "two"},
			Res:  true,
		},
		{
			Func: "foo",
			Args: []string{"two"},
			Res:  false,
		},
		{
			Func: "bar",
			Res:  true,
		},
		{
			Args: []string{"one", "two"},
			Res:  true,
		},
		{
			Args: []string{"one"},
			Res:  false,
		},
		{
			Args: []string{"two"},
			Res:  false,
		},
		{
			Func: "foo",
			Args: []string{"one", "three"},
			Res:  true,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("Func %s/Args %s", input.Func, input.Args), func(t *testing.T) {
			assert.Equal(t, input.Res, bl.IsBlacklisted(input.Func, input.Args))
		})
	}
}

func TestEscapeUnicodeNULL(t *testing.T) {
	for _, goodJS := range [][]byte{
		[]byte(`{"one": "two"}`),
		[]byte(`{"air plane char": "is \u2708"}`),
	} {
		t.Run("For good string change nothing: "+string(goodJS), func(t *testing.T) {
			assert.Equal(t, goodJS, core.EscapeUnicodeNULL(goodJS))
		})
	}
	t.Run("Escape NULL", func(t *testing.T) {
		badJs := []byte(`{
  			 "jid": "20190822065749952609",
			"return": [
				"Rendering SLS failed: ... - test: mysql-initialized\n\n\n\n\u0000\n---"
			],
		}
		`)
		fixedJs := []byte(`{
  			 "jid": "20190822065749952609",
			"return": [
				"Rendering SLS failed: ... - test: mysql-initialized\n\n\n\n\u2400\n---"
			],
		}
		`)
		assert.Equal(t, fixedJs, core.EscapeUnicodeNULL(badJs))
	})
}
