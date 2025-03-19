package secrets

import (
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

////////////////////////////////////////////////////////////////////////////////

func TestFilter(t *testing.T) {
	s := &Secrets{
		Z2ApiKey:      "Z2-API-KEY",
		TelegramToken: "TELEGRAM-TOKEN",
		OAuthToken:    "OAUTH-TOKEN",
	}

	var w strings.Builder

	filter := NewFilter(&w, NewReplacer(s, "IAM-TOKEN"))

	_, err := filter.Write([]byte("==IAM-TOKEN=Z2-API-KEY-TELEGRAM-TOKEN-OAUTH-TOKEN-IAM-TOKEN=="))
	require.NoError(t, err)

	assert.Equal(
		t,
		"==IIIIIIIII=ZZZZZZZZZZ-TTTTTTTTTTTTTT-OOOOOOOOOOO-IIIIIIIII==",
		w.String(),
	)
}
