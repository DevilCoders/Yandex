package admin

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

type testObject struct {
	Name  string
	Value int
}

func equalTestObjects(a, b testObject) bool {
	return a.Value == b.Value
}

func (o testObject) DiffKey() string {
	return o.Name
}

func TestDiff(t *testing.T) {
	var objects struct {
		Expected, Actual []testObject
	}

	objects.Expected = []testObject{
		{Name: "a", Value: 0},
		{Name: "b", Value: 1},
		{Name: "c", Value: 2},
		{Name: "d", Value: 3},
	}

	objects.Actual = []testObject{
		{Name: "b", Value: 0},
		{Name: "a", Value: 0},
		{Name: "d", Value: 3},
		{Name: "e", Value: 3},
	}

	add, remove, update := Diff(ReflectDiff(&objects, equalTestObjects))

	assert.Equal(t, []int{2}, add)
	assert.Equal(t, []int{3}, remove)
	assert.Equal(t, []Update{{From: 1, To: 0}}, update)
}
