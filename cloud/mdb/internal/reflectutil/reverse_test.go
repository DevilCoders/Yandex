package reflectutil_test

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
)

func TestReverseMap(t *testing.T) {
	data := []struct {
		forward  interface{}
		backward interface{}
	}{
		{
			forward: map[string]string{
				"k1": "v1",
				"k2": "v2",
				"k3": "v3",
			},
			backward: map[string]string{
				"v1": "k1",
				"v2": "k2",
				"v3": "k3",
			},
		},
		{
			forward: map[int]string{
				1: "v1",
				2: "v2",
				3: "v3",
			},
			backward: map[string]int{
				"v1": 1,
				"v2": 2,
				"v3": 3,
			},
		},
		{
			forward: map[int]int{
				1: 4,
				2: 5,
				3: 6,
			},
			backward: map[int]int{
				4: 1,
				5: 2,
				6: 3,
			},
		},
	}
	for _, d := range data {
		t.Run(reflect.TypeOf(d).String(), func(t *testing.T) {
			assert.Equal(t, d.backward, reflectutil.ReverseMap(d.forward))
			assert.Equal(t, d.forward, reflectutil.ReverseMap(d.backward))
		})
	}
}
