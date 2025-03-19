package move

import (
	"context"
	"testing"

	"go.uber.org/zap"

	"github.com/stretchr/testify/require"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

func TestChangedBlocksMaskAdd(t *testing.T) {
	ctx := log.WithLogger(context.Background(), zap.NewNop())

	cbm := newChangedBlocksMask(1, 16)
	cbm.AddMask(ctx, 0, 1, []byte{0xA6})
	cbm.AddMask(ctx, 8, 1, []byte{0x12})
	require.EqualValues(t, []byte{0xA6, 0x12}, cbm.mask)

	cbm = newChangedBlocksMask(4, 32)
	cbm.AddMask(ctx, 0, 2, []byte{0xA6, 0x12})
	require.EqualValues(t, []byte{0x5F}, cbm.mask)

	cbm = newChangedBlocksMask(8, 64)
	for i := byte(0); i < 8; i++ {
		cbm.AddMask(ctx, uint64(i*8), 1, []byte{i})
	}
	require.EqualValues(t, []byte{0xFE}, cbm.mask)
}

func TestChangeBlocksMaskGet(t *testing.T) {
	ctx := log.WithLogger(context.Background(), zap.NewNop())

	cbm := &changedBlocksMask{[]byte{0xA6}, 1}
	result := []bool{false, true, true, false, false, true, false, true}
	for i := 0; i < 8; i++ {
		require.EqualValues(t, result[i], cbm.GetBit(ctx, uint64(i), 1))
	}

	cbm = &changedBlocksMask{[]byte{0x26}, 1}
	result = []bool{true, true, true, false}
	for i := 0; i < 4; i++ {
		require.EqualValues(t, result[i], cbm.GetBit(ctx, uint64(i), 2))
	}

	cbm = &changedBlocksMask{[]byte{0x26}, 1}
	result = []bool{true, true}
	for i := 0; i < 2; i++ {
		require.EqualValues(t, result[i], cbm.GetBit(ctx, uint64(i), 4))
	}

	cbm = &changedBlocksMask{[]byte{0xA6}, 4}
	result = []bool{false, false, true}
	for i := 0; i < 3; i++ {
		require.EqualValues(t, result[i], cbm.GetBit(ctx, uint64(i), 2))
	}

	cbm = &changedBlocksMask{[]byte{0xA6}, 4}
	result = []bool{false, false, true, true, true, true, false, false, false, false, true, true, false, false, true, true}
	for i := 0; i < 16; i++ {
		require.EqualValues(t, result[i], cbm.GetBit(ctx, uint64(i), 2))
	}
}
