package tilde_test

import (
	"fmt"
	"os"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/tilde"
)

func TestMain(m *testing.M) {
	// Set fake home dir if it doesn't exist
	_, ok := os.LookupEnv(tilde.LinuxHome)
	if !ok {
		if err := os.Setenv(tilde.LinuxHome, "/home/test"); err != nil {
			panic(fmt.Sprintf("failed to set home env var: %s", err))
		}
	}

	res := m.Run()

	// Remove fake home dir if needed
	if !ok {
		if err := os.Unsetenv(tilde.LinuxHome); err != nil {
			panic(fmt.Sprintf("failed to unset home env var: %s", err))
		}
	}

	os.Exit(res)
}

func TestHome(t *testing.T) {
	h, err := tilde.Home()
	assert.NoError(t, err)
	require.NotEqual(t, 0, len(h))
	t.Logf("Found user home: %s", h)
}

func TestExpand(t *testing.T) {
	h, err := tilde.Home()
	require.NoError(t, err)
	require.NotEqual(t, 0, len(h))

	p, err := tilde.Expand("~")
	assert.NoError(t, err)
	assert.Equal(t, h, p)

	p, err = tilde.Expand("~/")
	assert.NoError(t, err)
	assert.Equal(t, h, p)

	p, err = tilde.Expand("~/path")
	assert.NoError(t, err)
	assert.Equal(t, filepath.Join(h, "path"), p)
}
