package test

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestTest(t *testing.T) {
	require.Equal(t, `"lol", 'kek' | cheburek`, os.Getenv("CUSTOM_ENV"))
}
