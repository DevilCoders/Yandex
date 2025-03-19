package agent

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

const adminStatusOozieNormal = `{
	"systemMode":"NORMAL"
  }`

const adminStatusOozieSafemode = `{
	"systemMode":"SAFEMODE"
  }`

func Test_parseOozieNormalResponse(t *testing.T) {
	want := models.OozieInfo{
		Available: true,
	}
	got, err := parseOozieResponse([]byte(adminStatusOozieNormal))
	require.NoError(t, err)
	require.Equal(t, want.Available, got.Available)
}

func Test_parseOozieUnusualResponse(t *testing.T) {
	want := models.OozieInfo{
		Available: false,
	}
	got, err := parseOozieResponse([]byte(adminStatusOozieSafemode))
	require.Error(t, err)
	require.Equal(t, want.Available, got.Available)
}
