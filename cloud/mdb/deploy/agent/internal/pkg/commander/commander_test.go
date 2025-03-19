package commander

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestVersionFromFromName(t *testing.T) {
	tests := []struct {
		caseName string
		name     string
		want     string
	}{
		{
			"File with an ext (typically S3 key)",
			"image-1644272755-r9117567.txz",
			"1644272755-r9117567",
		},
		{
			"File without an ext (typically /srv symlink)",
			"image-1644272755-r9117567",
			"1644272755-r9117567",
		},
	}
	for _, tt := range tests {
		t.Run(tt.caseName, func(t *testing.T) {
			require.Equal(t, tt.want, VersionFromFromName(tt.name))
		})
	}
}
