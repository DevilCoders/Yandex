package dnsapi_test

import (
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
)

func generate(count int) *dnsapi.RequestUpdate {
	req := &dnsapi.RequestUpdate{Records: make(dnsapi.Records, count)}
	for i := 0; i < count; i++ {
		cid := uuid.Must(uuid.NewV4()).String()
		cur := uuid.Must(uuid.NewV4()).String()
		prim := uuid.Must(uuid.NewV4()).String()
		req.Records[cid] = dnsapi.Update{CNAMEOld: cur, CNAMENew: prim}
	}
	return req
}

func TestSplitOnPartsCorner(t *testing.T) {
	rup := dnsapi.SplitOnParts(generate(0))
	require.Equal(t, 0, len(rup))
	rup = dnsapi.SplitOnParts(generate(1))
	require.Equal(t, 0, len(rup))
}

func TestSplitOnParts2(t *testing.T) {
	size := 2
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 2, len(rup))
	require.Equal(t, 1, len(rup[0].Records))
	require.Equal(t, 1, len(rup[1].Records))
}

func TestSplitOnParts3(t *testing.T) {
	size := 3
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 3, len(rup))
	require.Equal(t, 1, len(rup[0].Records))
	require.Equal(t, 1, len(rup[1].Records))
	require.Equal(t, 1, len(rup[2].Records))
}

func TestSplitOnParts5(t *testing.T) {
	size := 5
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 5, len(rup))
	require.Equal(t, 1, len(rup[0].Records))
	require.Equal(t, 1, len(rup[1].Records))
	require.Equal(t, 1, len(rup[2].Records))
	require.Equal(t, 1, len(rup[3].Records))
	require.Equal(t, 1, len(rup[4].Records))
}

func TestSplitOnParts6(t *testing.T) {
	size := 6
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 5, len(rup))
	require.Equal(t, 2, len(rup[0].Records))
	require.Equal(t, 1, len(rup[1].Records))
	require.Equal(t, 1, len(rup[2].Records))
	require.Equal(t, 1, len(rup[3].Records))
	require.Equal(t, 1, len(rup[4].Records))
}

func TestSplitOnParts17(t *testing.T) {
	size := 17
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 5, len(rup))
	require.Equal(t, 4, len(rup[0].Records))
	require.Equal(t, 4, len(rup[1].Records))
	require.Equal(t, 3, len(rup[2].Records))
	require.Equal(t, 3, len(rup[3].Records))
	require.Equal(t, 3, len(rup[4].Records))
}

func TestSplitOnParts50(t *testing.T) {
	size := 50
	ru := generate(size)
	require.Equal(t, size, len(ru.Records))

	rup := dnsapi.SplitOnParts(ru)
	require.Equal(t, 5, len(rup))
	require.Equal(t, 10, len(rup[0].Records))
	require.Equal(t, 10, len(rup[1].Records))
	require.Equal(t, 10, len(rup[2].Records))
	require.Equal(t, 10, len(rup[3].Records))
	require.Equal(t, 10, len(rup[4].Records))
}
