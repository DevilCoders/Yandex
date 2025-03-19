package app

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/publish"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/vcs"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func Test_shouldBuildImage(t *testing.T) {
	shouldCases := []struct {
		message         string
		vcsVersion      string
		publishedImages []publish.Image
	}{
		{
			"when no images",
			"10",
			nil,
		},
		{
			"when in vcs version is greater",
			"10",
			[]publish.Image{{Version: "7"}},
		},
		{
			"when version is same but overrideVersionAfter timeout passed",
			"10",
			[]publish.Image{{Version: "10", Uploaded: time.Time{}}},
		},
		{
			"when uploaded version is malformed, it ignored",
			"10",
			[]publish.Image{{Version: "10"}},
		},
	}
	for _, c := range shouldCases {
		t.Run(c.message, func(t *testing.T) {
			ok, err := shouldBuildImage(c.vcsVersion, c.publishedImages, time.Hour, &nop.Logger{})
			require.NoError(t, err)
			require.True(t, ok)
		})
	}

	t.Run("when version is same but overrideVersionAfter timeout not passed", func(t *testing.T) {
		ok, err := shouldBuildImage("10", []publish.Image{{Version: "10", Uploaded: time.Now()}}, time.Hour, &nop.Logger{})
		require.NoError(t, err)
		require.False(t, ok)
	})
	t.Run("when current (vcs) version is malformed", func(t *testing.T) {
		_, err := shouldBuildImage("xx", []publish.Image{{Version: "10"}}, time.Hour, &nop.Logger{})
		require.Error(t, err)
	})
	t.Run("when a lot of same uploaded version exists but with different dates", func(t *testing.T) {
		ok, err := shouldBuildImage("10", []publish.Image{{Version: "10"}, {Version: "10", Uploaded: time.Now()}}, time.Hour, &nop.Logger{})
		require.NoError(t, err)
		require.False(t, ok)
	})
}

func Test_monitorPublishedImages(t *testing.T) {
	tests := []struct {
		name   string
		last   vcs.Head
		images []publish.Image
		want   monrun.ResultCode
	}{
		{
			"No images",
			vcs.Head{Version: "10"},
			nil,
			monrun.CRIT,
		},
		{
			"Malformed VCS version",
			vcs.Head{Version: "not-a-number"},
			nil,
			monrun.CRIT,
		},
		{
			"Old image",
			vcs.Head{Version: "10", Time: time.Now()},
			[]publish.Image{{Version: "7", Uploaded: time.Time{}}},
			monrun.CRIT,
		},
		{
			"Exists newer image, but it upload within WARN limit",
			vcs.Head{Version: "10", Time: time.Now()},
			[]publish.Image{{Version: "7", Uploaded: time.Now().Add(-2 * time.Minute)}},
			monrun.WARN,
		},
		{
			"Exists newer image, but it upload reticently",
			vcs.Head{Version: "10", Time: time.Now()},
			[]publish.Image{{Version: "7", Uploaded: time.Now()}},
			monrun.OK,
		},
		{
			"Uploaded images on latest version",
			vcs.Head{Version: "10"},
			[]publish.Image{{Version: "10"}},
			monrun.OK,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(
				t,
				tt.want,
				monitorPublishedImages(tt.last, tt.images, time.Minute, time.Hour, &nop.Logger{}).Code,
			)
		})
	}
}
