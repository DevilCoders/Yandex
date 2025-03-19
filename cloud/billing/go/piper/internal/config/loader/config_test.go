package loader

import (
	"context"
	"reflect"
	"testing"

	"github.com/heetch/confita/backend"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

type inner struct {
	Inner string `config:"inner"`
}

func TestParse(t *testing.T) {
	var s struct {
		NoTag  string
		Simple string `config:"simple"`
		Skip   string `config:"-"`
		Nested struct {
			NoTag  string
			Simple string `config:"simple"`
			Skip   string `config:"-"`
		} `config:"nested"`
		Map           map[string]*inner
		MapWithTag    map[string]*inner `config:"maptag"`
		MapNotPointer map[string]inner
		NotStringMap  map[int]*inner `config:"intmap"`
		SomeMap       map[int]string `config:"somemap"`
	}

	s.Map = map[string]*inner{"key": {}}
	s.MapWithTag = map[string]*inner{"key": {}}
	s.NotStringMap = map[int]*inner{0: {}}
	s.MapNotPointer = map[string]inner{"key": {}}
	s.SomeMap = map[int]string{0: ""}

	l := New()
	value := reflect.ValueOf(&s).Elem()
	parsed := l.parseStruct(&value, "")

	for _, f := range parsed.Fields {
		t.Logf("%#v", f)
	}

	require.Len(t, parsed.Fields, 6)

	want := []string{
		"simple",
		"nested.simple",
		"key.inner",
		"maptag.key.inner",
		"intmap",
		"somemap",
	}
	for i, key := range want {
		assert.Equal(t, key, parsed.Fields[i].Key)
	}
}

type unmarshaler struct {
	value string
}

func (u *unmarshaler) UnmarshalText(text []byte) error {
	u.value = string(text)
	return nil
}

func TestSetUnmarshaler(t *testing.T) {
	var s struct {
		Value unmarshaler `config:"value"`
	}

	l := New(backend.Func("func", func(ctx context.Context, value string) ([]byte, error) {
		return []byte("value"), nil
	}))

	err := l.Load(context.TODO(), &s)
	require.NoError(t, err)
	assert.Equal(t, "value", s.Value.value)
}
