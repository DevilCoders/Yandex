package test

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

type Msg struct {
	XXX_unrecognized []byte
}

func F() {
	var m0, m1 Msg

	reflect.DeepEqual(m0, m1)                   // want "avoid using reflect.DeepEqual with proto.Message"
	reflect.DeepEqual(&m0, &m1)                 // want "avoid using reflect.DeepEqual with proto.Message"
	reflect.DeepEqual([]*Msg{&m0}, []*Msg{&m1}) // want "avoid using reflect.DeepEqual with proto.Message"
}

func Other() {
	var m0, m1 Msg

	var t *testing.T

	assert.Equal(t, &m0, &m1)  // want "avoid using assert.Equal with proto.Message"
	require.Equal(t, &m0, &m1) // want "avoid using require.Equal with proto.Message"

	assert.New(t).Equal(&m0, &m1)  // want "avoid using assert.Equal with proto.Message"
	require.New(t).Equal(&m0, &m1) // want "avoid using require.Equal with proto.Message"

	assert.Equalf(t, &m0, &m1, "")  // want "avoid using assert.Equalf with proto.Message"
	require.Equalf(t, &m0, &m1, "") // want "avoid using require.Equalf with proto.Message"

	assert.New(t).Equalf(&m0, &m1, "")  // want "avoid using assert.Equalf with proto.Message"
	require.New(t).Equalf(&m0, &m1, "") // want "avoid using require.Equalf with proto.Message"

	assert.Equalf(t, 1, 2, "%v", &m0)
}
