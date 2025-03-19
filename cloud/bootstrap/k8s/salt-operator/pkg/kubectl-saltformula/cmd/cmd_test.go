package cmd

import (
	"bytes"
	"testing"

	"github.com/stretchr/testify/assert"

	options "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options/fake"
)

func TestCmdSaltFormulaCmdUsage(t *testing.T) {
	o := options.NewFakeSaltFormulaOptions()
	cmd := NewCmdSaltFormula(o)
	err := cmd.Execute()
	assert.Error(t, err)
	stdout := o.Out.(*bytes.Buffer).String()
	stderr := o.ErrOut.(*bytes.Buffer).String()
	assert.Empty(t, stdout)
	assert.Contains(t, stderr, "Usage:")
	assert.Contains(t, stderr, "kubectl-sf COMMAND")
}
