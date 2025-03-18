package yofix

import (
	"bytes"
	"os/exec"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/yatest"
)

func Run(t *testing.T, owner string) {
	require.NoError(t, yatest.PrepareGOPATH())
	require.NoError(t, yatest.PrepareGOCACHE())

	bin, err := yatest.BinaryPath("library/go/yo/cmd/yo/yo")
	require.NoError(t, err)

	goListCmd := exec.Command(bin, "fix", "-dry-run", "-add-owner", owner, ".")

	var out bytes.Buffer
	goListCmd.Stdout = &out
	goListCmd.Stderr = &out

	if err := goListCmd.Run(); err != nil {
		t.Errorf("yo fix exited with non-zero exit code, please run 'ya tool yo fix -add-owner %s <path>':\n%s",
			owner, out.String())
	}
}
