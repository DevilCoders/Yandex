package get

import (
	"bytes"
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"k8s.io/cli-runtime/pkg/genericclioptions"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/info/testdata"
	options "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options/fake"
)

func assertStdout(t *testing.T, expectedOut string, o genericclioptions.IOStreams) {
	t.Helper()
	stderr := o.ErrOut.(*bytes.Buffer).String()
	assert.Empty(t, stderr)

	expectedOut = stripTrailingWhitespace(expectedOut)
	stdout := stripTrailingWhitespace(o.Out.(*bytes.Buffer).String())
	if !assert.Equal(t, expectedOut, stdout) {
		fmt.Println("\n" + stdout)
	}
}

// stripTrailingWhitespace is a helper to strip trailing spaces from every line of the output
func stripTrailingWhitespace(s string) string {
	var newLines []string
	for _, line := range strings.Split(s, "\n") {
		newLines = append(newLines, strings.TrimRight(line, " "))
	}
	return strings.Join(newLines, "\n")
}

func TestGetUsage(t *testing.T) {
	o := options.NewFakeSaltFormulaOptions()
	cmd := NewCmdGet(o)
	cmd.PersistentPreRunE = o.PersistentPreRunE
	cmd.SetArgs([]string{})
	err := cmd.Execute()
	assert.Error(t, err)
	stderr := o.ErrOut.(*bytes.Buffer).String()
	assert.Contains(t, stderr, "Usage:")
	assert.Contains(t, stderr, "get SALT_FORMULA_NAME")
}

func TestSaltFormulaNotFound(t *testing.T) {
	o := options.NewFakeSaltFormulaOptions()
	cmd := NewCmdGet(o)
	cmd.PersistentPreRunE = o.PersistentPreRunE
	cmd.SetArgs([]string{"does-not-exist"})
	err := cmd.Execute()
	assert.Error(t, err)
	stdout := o.Out.(*bytes.Buffer).String()
	stderr := o.ErrOut.(*bytes.Buffer).String()
	assert.Empty(t, stdout)
	assert.Equal(t, "Error: saltformulas.bootstrap.cloud.yandex.net \"does-not-exist\" not found\n", stderr)
}

func TestSaltFormulaGet(t *testing.T) {
	sfObjs := testdata.NewSimpleSaltFormula()

	o := options.NewFakeSaltFormulaOptions(sfObjs.AllObjects()...)
	o.UserNamespace = sfObjs.SaltFormulas[0].Namespace
	cmd := NewCmdGet(o)
	cmd.PersistentPreRunE = o.PersistentPreRunE
	cmd.SetArgs([]string{sfObjs.SaltFormulas[0].Name, "--no-color"})
	err := cmd.Execute()
	assert.NoError(t, err)
	expectedOut := strings.TrimPrefix(`
Name:            bootstrap
Namespace:       bootstrap
BaseRole:        bootstrap
Role:            bootstrap
Desired:         2
Current:         2
Ready:           1
Epoch:           22.07.14.0011

  NAME                                                     KIND         STATUS       AGE  INFO
  ƒ bootstrap                                              SaltFormula               7d
  ├──⊞ bootstrap-myt1-bootstrap-0.1-10766.220713           Job          ✖ Failed     7d
  │  ├──□ bootstrap-myt1-bootstrap-0.1-10766.220713-dsktz  Pod          ⚠ Error      7d
  │  └──□ bootstrap-myt1-bootstrap-0.1-10766.220713-r89kc  Pod          ⚠ Error      7d
  └──⊞ bootstrap-myt2-bootstrap-0.1-10766.220713           Job          ✔ Complete   7d   Duration: 61s
     └──□ bootstrap-myt2-bootstrap-0.1-10766.220713-rv8c5  Pod          ✔ Completed  7d
`, "\n")
	assertStdout(t, expectedOut, o.IOStreams)
}
