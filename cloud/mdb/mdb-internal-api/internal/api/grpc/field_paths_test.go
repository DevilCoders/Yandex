package grpc

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestFieldPaths(t *testing.T) {
	fieldPaths := NewFieldPaths([]string{
		"field1",
		"field2",
		"parent1.field3",
		"parent1.field4",
		"parent1.parent2.field5",
		"parent1.parent2.field6",
		"parent3.field7",
	})

	require.False(t, fieldPaths.Remove("field0"))
	require.True(t, fieldPaths.Remove("field1"))

	parent1 := fieldPaths.Subtree("parent1.")
	require.False(t, parent1.Empty())
	require.False(t, parent1.Remove("field0"))
	require.True(t, parent1.Remove("field3"))

	parent2 := parent1.Subtree("parent2.")
	require.False(t, parent2.Empty())
	require.False(t, parent2.Remove("field0"))
	require.True(t, parent2.Remove("field6"))

	parent3 := fieldPaths.Subtree("parent3.")
	require.False(t, parent2.Empty())
	parent5 := parent3.Subtree("parent5.")
	require.True(t, parent5.Empty())
	require.True(t, parent3.Remove("field7"))

	parent4 := fieldPaths.Subtree("parent4.")
	require.True(t, parent4.Empty())

	err := fieldPaths.MustBeEmpty()
	require.Error(t, err)
	require.Equal(t, "unknown field paths: field2, parent1.field4, parent1.parent2.field5", err.Error())
}

func TestSubPaths(t *testing.T) {
	fieldPaths := NewFieldPaths([]string{
		"connector.properties.key1",
		"connector.properties.key2",
		"connector.properties.key3",
	})

	connectorSubtree := fieldPaths.Subtree("connector.")
	propertiesSubtree := connectorSubtree.Subtree("properties.")
	pathContains := map[string]bool{}
	for _, path := range propertiesSubtree.SubPaths() {
		pathContains[path] = true
	}

	_, ok := pathContains["key1"]
	require.True(t, ok)
	_, ok = pathContains["key2"]
	require.True(t, ok)
	_, ok = pathContains["key3"]
	require.True(t, ok)
	_, ok = pathContains["key4"]
	require.False(t, ok)
}
