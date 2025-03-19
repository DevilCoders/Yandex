package vcs

import (
	"fmt"
	"os/exec"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_lastChangedFromSVNInfo(t *testing.T) {
	var inputs = []struct {
		In      string
		Version string
		Fail    bool
	}{
		{
			In:   "",
			Fail: true,
		},
		{
			In:   "<info></info>",
			Fail: true,
		},
		{
			In: `
<?xml version="1.0" encoding="UTF-8"?>
<info>
	<entry
	   kind="dir"
	   path="salt"
	   revision="7050808">
		<url>svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud/mdb/salt</url>
		<relative-url>^/trunk/arcadia/cloud/mdb/salt</relative-url>
		<repository>
			<root>svn+ssh://arcadia.yandex.ru/arc</root>
			<uuid>41d65440-b5be-11dd-afe3-b2e846d9b4f8</uuid>
		</repository>
		<commit
		   revision="7050449">
			<author>sidh</author>
			<date>2020-06-29T11:52:33.581337Z</date>
		</commit>
	</entry>
</info>
`,
			Version: "7050449",
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("In %q", input.In), func(t *testing.T) {
			out, err := lastChangedFromSVNInfo(input.In)

			if input.Fail {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, input.Version, out.Version)
				assert.NotEqual(t, time.Time{}, out.Time)
			}
		})
	}
}

func Test_safeErrorForSVNUpdate(t *testing.T) {
	t.Run("safe error", func(t *testing.T) {
		safeErr := &ExecutionFailed{
			Cmd:        exec.Command("svn"),
			Stderr:     `"svn": No such file or directory (os error 2)`,
			InnerError: exec.ErrNotFound,
		}
		assert.False(t, safeErrorForSVNUpdate(safeErr))
	})
	t.Run("safe error", func(t *testing.T) {
		safeErr := &ExecutionFailed{
			Cmd: exec.Command("svn"),
			Stderr: `{svn: E170013: Unable to connect to a repository at URL 'svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud/mdb/pg'
svn: E210002: To better debug SSH connection problems, remove the -q option from 'ssh' in the [tunnels] section of your Subversion configuration file.
svn: E210002: Network connection closed unexpectedly}`,
			InnerError: exec.ErrNotFound,
		}
		assert.True(t, safeErrorForSVNUpdate(safeErr))
	})
	t.Run("generic errors not safe for retry", func(t *testing.T) {
		assert.False(t, safeErrorForSVNUpdate(xerrors.New("some new error")))
	})
}
