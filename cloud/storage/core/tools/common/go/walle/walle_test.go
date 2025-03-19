package walle

import (
	"context"
	"testing"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/conductor"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestSplit(t *testing.T) {
	log := nbs.NewStderrLog(nbs.LOG_DEBUG)
	c := conductor.NewClient(log)

	hosts, err := c.GetHostsByGroupName(context.TODO(), "cloud_prod_nbs_sas")
	require.NoError(t, err)

	assert.Greater(t, len(hosts), 0)

	w := NewClient(log)

	m, err := w.SplitByLocation(context.TODO(), hosts)
	require.NoError(t, err)
	assert.Greater(t, len(m), 0)
}
