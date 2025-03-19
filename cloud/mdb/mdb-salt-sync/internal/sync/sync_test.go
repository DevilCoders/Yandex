package sync

import (
	"fmt"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/vcs"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestRelativePath(t *testing.T) {
	var inputs = []struct {
		Root string
		In   string
		Out  string
		Fail bool
	}{
		{
			Root: "",
			In:   "",
			Out:  ".",
		},
		{
			Root: "components/",
			In:   "components/",
			Out:  ".",
		},
		{
			Root: "components",
			In:   "components/",
			Out:  ".",
		},
		{
			Root: "components",
			In:   "components",
			Out:  ".",
		},
		{
			Root: "/components",
			In:   "/components",
			Out:  ".",
		},
		{
			Root: "/components",
			In:   "/components",
			Out:  ".",
		},
		{
			Root: "components",
			In:   "components/pg-code/",
			Out:  "pg-code",
		},
		{
			Root: "components/pg-code",
			In:   "components/pg-code/metadb/",
			Out:  "metadb",
		},
		{
			Root: "components/pg-code/metadb/",
			In:   "components/pg-code/metadb/s3/stuff/",
			Out:  "s3/stuff",
		},
		{
			Root: "teste",
			In:   "test",
			Fail: true,
		},
		{
			Root: "a",
			In:   "b",
			Fail: true,
		},
		{
			Root: "a/b/c",
			In:   "b/c",
			Fail: true,
		},
		{
			Root: "b/c",
			In:   "b/c/d",
			Out:  "d",
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("Root %q, In %q", input.Root, input.In), func(t *testing.T) {
			p, err := relativePath(input.Root, input.In)

			if input.Fail {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, input.Out, p)
			}
		})
	}
}

func TestVCSForPath(t *testing.T) {
	const (
		mpSalt   = "/srv/salt"
		mpPGCode = "/srv/salt/qa-1/components/pg-code"
		mpMetaDB = "/srv/salt/qa-1/components/pg-code/metadb"
	)
	frvcs := &vcs.SVNProvider{MountPath: "/srv"}
	vcses := map[string]vcs.Provider{
		mpSalt:   &vcs.SVNProvider{MountPath: mpSalt},
		mpPGCode: &vcs.SVNProvider{MountPath: mpPGCode},
		mpMetaDB: &vcs.SVNProvider{MountPath: mpMetaDB},
	}

	var inputs = []struct {
		Target   string
		Expected vcs.Provider
	}{
		{
			Target:   "/srv",
			Expected: frvcs,
		},
		{
			Target:   "/srv/",
			Expected: frvcs,
		},
		{
			Target:   "/srv/salt/qa-1/components",
			Expected: vcses[mpSalt],
		},
		{
			Target:   "/srv/salt/qa-1/components/",
			Expected: vcses[mpSalt],
		},
		{
			Target:   "/srv/salt/qa-1/_returners",
			Expected: vcses[mpSalt],
		},
		{
			Target:   "/srv/salt/qa-1/components/deploy",
			Expected: vcses[mpSalt],
		},
		{
			Target:   "/srv/salt/qa-1/components/pg-code",
			Expected: vcses[mpPGCode],
		},
		{
			Target:   "/srv/salt/qa-1/components/pg-code/metadb",
			Expected: vcses[mpMetaDB],
		},
	}

	vcsMounts := make([]string, 0, len(vcses))
	for mount := range vcses {
		vcsMounts = append(vcsMounts, mount)
	}

	sort.Strings(vcsMounts)
	for _, input := range inputs {
		t.Run(fmt.Sprintf("Target %q", input.Target), func(t *testing.T) {
			assert.Equal(t, input.Expected, vcsForPath(frvcs, vcsMounts, vcses, input.Target, &nop.Logger{}))
		})
	}
}

func Test_excludeForTarget(t *testing.T) {
	t.Run("root", func(t *testing.T) {
		ret := excludeForTarget(
			"",
			map[string]string{
				"components":       "trunk",
				"components/zk":    "1010",
				"components/pg":    "2020",
				"components/ch/zk": "3030",
			})
		require.Equal(t, []string{"components"}, ret)
	})
	t.Run("target with children", func(t *testing.T) {
		ret := excludeForTarget(
			"components",
			map[string]string{
				"components":       "trunk",
				"components/zk":    "1010",
				"components/pg":    "trunk",
				"components/ch":    "3030",
				"components/ch/zk": "3010",
			})
		require.ElementsMatch(t, ret, []string{"zk", "pg", "ch"})
	})
	t.Run("leaf target", func(t *testing.T) {
		ret := excludeForTarget(
			"components/zk",
			map[string]string{
				"components":    "trunk",
				"components/zk": "1010",
				"components/pg": "2020",
			})
		require.Empty(t, ret)
	})
	t.Run("not existed target", func(t *testing.T) {
		ret := excludeForTarget(
			"components/oracle",
			map[string]string{
				"components":    "trunk",
				"components/zk": "1010",
				"components/pg": "2020",
			})
		require.Empty(t, ret)
	})
}
