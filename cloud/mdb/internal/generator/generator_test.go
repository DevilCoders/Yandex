package generator

import (
	"os"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/yatest"
)

func TestGenerateID(t *testing.T) {
	id, err := generateID(defaultCloudIDPrefix)
	require.NoError(t, err)
	require.Len(t, id, cloudIDGenPartLen+len(defaultCloudIDPrefix))
	require.True(t, strings.HasPrefix(id, defaultCloudIDPrefix))
	for _, s := range id[len(defaultCloudIDPrefix):] {
		require.Contains(t, cloudValidIDRunes, string(s))
	}
}

func TestGenerateHostname(t *testing.T) {
	hostname, err := generateHostname(hostnameUbuntuLen)
	require.NoError(t, err)
	require.Len(t, hostname, int(hostnameUbuntuLen))
	for _, s := range hostname {
		require.Contains(t, hostnameValidRunes, string(s))
	}
}

func TestFileSequenceGenerator(t *testing.T) {
	g := fileSequenceGenerator{Path: yatest.OutputPath("cache")}
	prefix := "prefix"

	// Cache should not exist
	_, err := os.Stat(g.Path)
	require.Error(t, err)
	require.True(t, os.IsNotExist(err))

	first := g.Generate(prefix)
	second := g.Generate(prefix)

	// Second id should not be equal the first
	require.NotEqual(t, first, second)

	// Check that cache exists
	_, err = os.Stat(g.Path)
	require.NoError(t, err)

	// Remove the cache
	require.NoError(t, os.RemoveAll(g.Path))

	// Third id must be equal the first one because we removed all the generator's cache
	third := g.Generate(prefix)
	require.Equal(t, first, third)

	// Fourth id must be equal the second
	fourth := g.Generate(prefix)
	require.Equal(t, second, fourth)
}

func TestClusterIDGenerator(t *testing.T) {
	g := &ClusterIDGenerator{}
	id, err := g.Generate()
	require.NoError(t, err)
	require.Len(t, id, cloudIDGenPartLen+len(defaultCloudIDPrefix))
	require.True(t, strings.HasPrefix(id, defaultCloudIDPrefix))
}

func TestTaskIDGenerator(t *testing.T) {
	g := &TaskIDGenerator{}
	id, err := g.Generate()
	require.NoError(t, err)
	require.Len(t, id, cloudIDGenPartLen+len(defaultCloudIDPrefix))
	require.True(t, strings.HasPrefix(id, defaultCloudIDPrefix))
}

func TestRandomHostnameGenerator(t *testing.T) {
	prefix := "prefix"
	suffix := "suffix"
	gens := []*RandomHostnameGenerator{NewRandomHostnameUbuntuGenerator(), NewRandomHostnameWindowsGenerator()}
	for _, gen := range gens {
		t.Run("RandomHostnameUbuntuGenerator", func(t *testing.T) {
			id, err := gen.Generate(prefix, suffix)
			require.NoError(t, err)
			require.GreaterOrEqual(t, len(id), len(prefix)+int(gen.length)+len(suffix))
			require.True(t, strings.HasPrefix(id, prefix))
			require.True(t, strings.HasSuffix(id, suffix))
		})
	}
}

func TestFileSequenceHostnameGenerator(t *testing.T) {
	prefix := "prefix"
	suffix := "suffix"
	path := "path"
	g := NewFileSequenceHostnameGenerator(prefix, path)

	first, err := g.Generate(prefix, suffix)
	require.NoError(t, err)
	require.True(t, strings.HasPrefix(first, prefix))
	require.True(t, strings.HasSuffix(first, suffix))

	second, err := g.Generate(prefix, suffix)
	require.NoError(t, err)
	require.NotEqual(t, first, second)
	require.Len(t, first, len(second))
}

func TestFileSequenceHostnameGeneratorLengthIncreases(t *testing.T) {
	prefix := "prefix"
	suffix := "suffix"
	path := "path"
	g := NewFileSequenceHostnameGenerator(prefix, path)

	first, err := g.Generate(prefix, suffix)
	require.NoError(t, err)

	for i := 0; i < 8; i++ {
		_, err = g.Generate(prefix, suffix)
		require.NoError(t, err)
	}

	tenth, err := g.Generate(prefix, suffix)
	require.NoError(t, err)
	require.Greater(t, len(tenth), len(first))
}

func TestFileSequenceIDGenerator(t *testing.T) {
	prefix := "prefix"
	path := "path"
	g := NewFileSequenceIDGenerator(prefix, path)

	first, err := g.Generate()
	require.NoError(t, err)
	require.True(t, strings.HasPrefix(first, prefix))

	second, err := g.Generate()
	require.NoError(t, err)
	require.NotEqual(t, first, second)
	require.Len(t, first, len(second))
}

func TestFileSequenceIDGeneratorLengthIncreases(t *testing.T) {
	prefix := "prefix"
	path := "path"
	g := NewFileSequenceIDGenerator(prefix, path)

	first, err := g.Generate()
	require.NoError(t, err)

	for i := 0; i < 8; i++ {
		_, err = g.Generate()
		require.NoError(t, err)
	}

	tenth, err := g.Generate()
	require.NoError(t, err)
	require.Greater(t, len(tenth), len(first))
}
