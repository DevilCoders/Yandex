package stringsutil_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/stringsutil"
)

func TestQuotedJoin(t *testing.T) {
	inputs := []struct {
		in    []string
		sep   string
		quote string
		outs  []string
	}{
		{
			outs: []string{""},
		},
		{
			sep:   ",",
			quote: "'",
			outs:  []string{""},
		},
		{
			in:   []string{"foo"},
			outs: []string{"foo"},
		},
		{
			in:   []string{"foo"},
			sep:  ",",
			outs: []string{"foo"},
		},
		{
			in:    []string{"foo"},
			quote: "'",
			outs:  []string{"'foo'"},
		},
		{
			in:    []string{"foo"},
			sep:   ",",
			quote: "'",
			outs:  []string{"'foo'"},
		},
		{
			in:   []string{"foo", "bar"},
			outs: []string{"foobar", "barfoo"},
		},
		{
			in:   []string{"foo", "bar"},
			sep:  ",",
			outs: []string{"foo,bar", "bar,foo"},
		},
		{
			in:    []string{"foo", "bar"},
			quote: "'",
			outs:  []string{"'foo''bar'", "'bar''foo'"},
		},
		{
			in:   []string{"foo", "bar"},
			sep:  ", ",
			outs: []string{"foo, bar", "bar, foo"},
		},
		{
			in:    []string{"foo", "bar"},
			sep:   ", ",
			quote: "'",
			outs:  []string{"'foo', 'bar'", "'bar', 'foo'"},
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%s", input.in, input.sep), func(t *testing.T) {
			res := stringsutil.QuotedJoin(input.in, input.sep, input.quote)

			for _, out := range input.outs {
				if res == out {
					return
				}
			}

			assert.Fail(t, fmt.Sprintf("result %q does not match any of the expected ones: %q", res, input.outs))
		})
	}
}

func TestMapJoinStr(t *testing.T) {
	inputs := []struct {
		in   map[string]string
		sep  string
		outs []string
	}{
		{
			outs: []string{""},
		},
		{
			sep:  ",",
			outs: []string{""},
		},
		{
			in:   map[string]string{"foo": ""},
			outs: []string{"foo"},
		},
		{
			in:   map[string]string{"foo": ""},
			sep:  ",",
			outs: []string{"foo"},
		},
		{
			in:   map[string]string{"foo": "", "bar": ""},
			outs: []string{"foobar", "barfoo"},
		},
		{
			in:   map[string]string{"foo": "", "bar": ""},
			sep:  ",",
			outs: []string{"foo,bar", "bar,foo"},
		},
		{
			in:   map[string]string{"foo": "", "bar": ""},
			sep:  ", ",
			outs: []string{"foo, bar", "bar, foo"},
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%s", input.in, input.sep), func(t *testing.T) {
			res := stringsutil.MapJoinStr(input.in, input.sep)

			for _, out := range input.outs {
				if res == out {
					return
				}
			}

			assert.Fail(t, fmt.Sprintf("result %q does not match any of the expected ones: %q", res, input.outs))
		})
	}
}

func TestMapJoin(t *testing.T) {
	inputs := []struct {
		in   map[string]struct{}
		sep  string
		outs []string
	}{
		{
			outs: []string{""},
		},
		{
			sep:  ",",
			outs: []string{""},
		},
		{
			in:   map[string]struct{}{"foo": {}},
			outs: []string{"foo"},
		},
		{
			in:   map[string]struct{}{"foo": {}},
			sep:  ",",
			outs: []string{"foo"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			outs: []string{"foobar", "barfoo"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			sep:  ",",
			outs: []string{"foo,bar", "bar,foo"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			sep:  ", ",
			outs: []string{"foo, bar", "bar, foo"},
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%s", input.in, input.sep), func(t *testing.T) {
			res := stringsutil.MapJoin(input.in, input.sep)

			for _, out := range input.outs {
				if res == out {
					return
				}
			}

			assert.Fail(t, fmt.Sprintf("result %q does not match any of the expected ones: %q", res, input.outs))
		})
	}
}

func TestMapQuotedJoin(t *testing.T) {
	inputs := []struct {
		in    map[string]struct{}
		sep   string
		quote string
		outs  []string
	}{
		{
			outs: []string{""},
		},
		{
			sep:   ",",
			quote: "'",
			outs:  []string{""},
		},
		{
			in:   map[string]struct{}{"foo": {}},
			outs: []string{"foo"},
		},
		{
			in:   map[string]struct{}{"foo": {}},
			sep:  ",",
			outs: []string{"foo"},
		},
		{
			in:    map[string]struct{}{"foo": {}},
			quote: "'",
			outs:  []string{"'foo'"},
		},
		{
			in:    map[string]struct{}{"foo": {}},
			sep:   ",",
			quote: "'",
			outs:  []string{"'foo'"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			outs: []string{"foobar", "barfoo"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			sep:  ",",
			outs: []string{"foo,bar", "bar,foo"},
		},
		{
			in:    map[string]struct{}{"foo": {}, "bar": {}},
			quote: "'",
			outs:  []string{"'foo''bar'", "'bar''foo'"},
		},
		{
			in:   map[string]struct{}{"foo": {}, "bar": {}},
			sep:  ", ",
			outs: []string{"foo, bar", "bar, foo"},
		},
		{
			in:    map[string]struct{}{"foo": {}, "bar": {}},
			sep:   ", ",
			quote: "'",
			outs:  []string{"'foo', 'bar'", "'bar', 'foo'"},
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%s", input.in, input.sep), func(t *testing.T) {
			res := stringsutil.MapQuotedJoin(input.in, input.sep, input.quote)

			for _, out := range input.outs {
				if res == out {
					return
				}
			}

			assert.Fail(t, fmt.Sprintf("result %q does not match any of the expected ones: %q", res, input.outs))
		})
	}
}
