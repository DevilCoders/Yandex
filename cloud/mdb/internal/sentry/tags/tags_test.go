package tags

import (
	"context"
	"encoding/hex"
	"testing"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
)

func TestIdempotenceTags(t *testing.T) {
	hash := uuid.Must(uuid.NewV4()).Bytes()
	idemp := idempotence.Incoming{ID: idempotence.Must(), Hash: hash}
	ctx := idempotence.WithIncoming(context.Background(), idemp)
	tags := WellKnownTags(ctx)

	require.Equal(t, idemp.ID, tags["ctx.idempotence.incoming.id"])
	hashFromTags, err := hex.DecodeString(tags["ctx.idempotence.incoming.hash"])
	require.NoError(t, err)
	require.Equal(t, hash, hashFromTags)
}
