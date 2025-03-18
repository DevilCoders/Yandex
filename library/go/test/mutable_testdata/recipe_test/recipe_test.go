package testdata_test

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestWriteDir(t *testing.T) {
	_, err := os.Stat("data/a.txt")
	require.NoError(t, err)

	_, err = os.Create("data/b.txt")
	require.NoError(t, err)
}

func TestWriteFile(t *testing.T) {
	_, err := os.OpenFile("ya.make", os.O_RDWR, 0)
	require.NoError(t, err)
}
