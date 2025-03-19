package publish

import (
	"context"
	"io"
	"io/ioutil"
	"os"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/s3/http"
	s3mocks "a.yandex-team.ru/cloud/mdb/internal/s3/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/test/yatest"
)

func Test_getRevFromImageKey(t *testing.T) {
	require.Equal(t, "100500", getRevFromImageKey("image-112233-r100500.xz"))
}

func Test_genImageKey(t *testing.T) {
	require.Regexp(t, `image-\d+-r100500.xz`, genImageKey("image.tar.xz", "100500"))
}

func Test_getRevFromImageKeyFrom_genImageKey(t *testing.T) {
	require.Equal(
		t,
		"100500",
		getRevFromImageKey(
			genImageKey("image.tar.xz", "100500"),
		),
	)
}

func TestConfig_Valid(t *testing.T) {
	t.Run("empty config is not valid", func(t *testing.T) {
		require.Error(t, Config{}.Valid())
	})
	t.Run("config with duplicate envs is invalid", func(t *testing.T) {
		require.Error(t, Config{
			KeepImages: 42,
			Envs: []EnvConfig{
				{
					S3: http.Config{
						Host: "s3.host",
					},
					Bucket: "images",
				},
				{
					S3: http.Config{
						Host: "s3.host",
					},
					Bucket: "images",
				},
			},
		}.Valid())
	})
	t.Run("invalid keep images", func(t *testing.T) {
		require.Error(t, Config{
			KeepImages: -1,
			Envs: []EnvConfig{
				{
					S3: http.Config{
						Host: "s3.host",
					},
					Bucket: "images",
				},
			},
		}.Valid())
	})
	t.Run("valid config", func(t *testing.T) {
		require.NoError(t, Config{
			KeepImages: 42,
			Envs: []EnvConfig{
				{
					S3: http.Config{
						Host: "s3.host",
					},
					Bucket: "images",
				},
			},
		}.Valid())
	})
}

func TestPublisher_ListImages(t *testing.T) {
	now := time.Now()
	yesterday := now.Add(-24 * time.Hour)

	for _, testCase := range []struct {
		name      string
		s1objects []s3.Object
		s2objects []s3.Object
		expected  []Image
	}{
		{
			"images preset in all envs",
			[]s3.Object{
				{Key: "image-100-r500", LastModified: now},
			},
			[]s3.Object{
				{Key: "image-100-r500", LastModified: yesterday},
			},
			[]Image{{"500", yesterday}},
		},
		{
			"images not present in some envs should be ignored",
			[]s3.Object{
				{Key: "image-100-r500", LastModified: yesterday},
				{Key: "image-200-r600", LastModified: now},
			},
			[]s3.Object{
				{Key: "image-100-r500", LastModified: now},
			},
			[]Image{{"500", yesterday}},
		},
		{
			"images with bad keys should be ignored",
			[]s3.Object{
				{Key: "image-100-r500", LastModified: yesterday},
				{Key: "image-200-rSTRANGE-REV", LastModified: now},
			},
			[]s3.Object{
				{Key: "image-200-r", LastModified: now},
				{Key: "image-100-r500", LastModified: now},
			},
			[]Image{{"500", yesterday}},
		},
	} {
		t.Run(testCase.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			s1 := s3mocks.NewMockClient(ctrl)
			s2 := s3mocks.NewMockClient(ctrl)
			s1.EXPECT().ListObjects(gomock.Any(), "s1", gomock.Any()).Return(testCase.s1objects, []s3.Prefix{}, nil)
			s2.EXPECT().ListObjects(gomock.Any(), "s2", gomock.Any()).Return(testCase.s2objects, []s3.Prefix{}, nil)
			p := New(
				DefaultConfig(),
				map[string]Env{
					"s1": NewEnv("s1", s1, false),
					"s2": NewEnv("s2", s2, false),
				},
				&nop.Logger{},
			)

			ret, err := p.ListImages(context.Background())
			require.NoError(t, err)
			require.Equal(t, testCase.expected, ret)
		})
	}

	t.Run("possibly unavailable s3 is unavailable", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		s1 := s3mocks.NewMockClient(ctrl)
		s2 := s3mocks.NewMockClient(ctrl)
		s1.EXPECT().ListObjects(gomock.Any(), "s1", gomock.Any()).Return([]s3.Object{{Key: "image-100-r500", LastModified: yesterday}}, []s3.Prefix{}, nil)
		s2.EXPECT().ListObjects(gomock.Any(), "s2", gomock.Any()).Return(nil, nil, io.EOF)
		p := New(
			DefaultConfig(),
			map[string]Env{
				"s1": NewEnv("s1", s1, false),
				"s2": NewEnv("s2", s2, true),
			},
			&nop.Logger{},
		)

		ret, err := p.ListImages(context.Background())
		require.NoError(t, err)
		require.Equal(t, []Image{{"500", yesterday}}, ret)
	})
}

// That test may fail if your run them without ya, cause they requite TEST_WORK_PATH
func TestPublisher_PutImage(t *testing.T) {
	makeImageFile := func(t *testing.T) string {
		imageFile := yatest.WorkPath("some-image.txz")
		require.NoError(t, ioutil.WriteFile(imageFile, []byte(`some binary data`), os.ModePerm), "failed to prepare image file")
		return imageFile
	}

	t.Run("Happy path all works like a charm", func(t *testing.T) {
		uploadedImages := []s3.Object{
			{Key: "image-10-r100", LastModified: time.Now().Add(-time.Hour)},
			{Key: "image-50-r300", LastModified: time.Now().Add(-time.Minute)},
			{Key: "image-100-r500", LastModified: time.Now()},
		}
		ctrl := gomock.NewController(t)
		s1 := s3mocks.NewMockClient(ctrl)
		s2 := s3mocks.NewMockClient(ctrl)
		s1.EXPECT().PutObject(gomock.Any(), "s1", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
		s2.EXPECT().PutObject(gomock.Any(), "s2", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
		s1.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(uploadedImages, []s3.Prefix{}, nil)
		s2.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(uploadedImages, []s3.Prefix{}, nil)
		s1.EXPECT().DeleteObject(gomock.Any(), "s1", "image-10-r100").Return(nil)
		s2.EXPECT().DeleteObject(gomock.Any(), "s2", "image-10-r100").Return(nil)

		cfg := DefaultConfig()
		cfg.KeepImages = 2
		p := New(
			cfg,
			map[string]Env{
				"s1": NewEnv("s1", s1, false),
				"s2": NewEnv("s2", s2, false),
			},
			&nop.Logger{},
		)

		require.NoError(t, p.PutImage(context.Background(), makeImageFile(t), "500"))
	})

	t.Run("unavailable s3 fails", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		s1 := s3mocks.NewMockClient(ctrl)
		s2 := s3mocks.NewMockClient(ctrl)
		s1.EXPECT().PutObject(gomock.Any(), "s1", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
		s2.EXPECT().PutObject(gomock.Any(), "s2", gomock.Any(), gomock.Any(), gomock.Any()).Return(io.EOF).Times(1)
		s1.EXPECT().ListObjects(gomock.Any(), "s1", gomock.Any()).Return(nil, nil, nil)

		cfg := DefaultConfig()
		p := New(
			cfg,
			map[string]Env{
				"s1": NewEnv("s1", s1, false),
				"s2": NewEnv("s2", s2, true),
			},
			&nop.Logger{},
		)

		// since that s3 may be unavailable, no error should be returned
		require.NoError(t, p.PutImage(context.Background(), makeImageFile(t), "500"))
	})

	t.Run("after failed List on possibly unavailable s3, no Put to it should be done", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		s1 := s3mocks.NewMockClient(ctrl)
		s2 := s3mocks.NewMockClient(ctrl)
		s1.EXPECT().ListObjects(gomock.Any(), "s1", gomock.Any()).Return(nil, nil, nil).Times(2)
		s2.EXPECT().ListObjects(gomock.Any(), "s2", gomock.Any()).Return(nil, nil, io.EOF).Times(1)
		s1.EXPECT().PutObject(gomock.Any(), "s1", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)

		cfg := DefaultConfig()
		p := New(
			cfg,
			map[string]Env{
				"s1": NewEnv("s1", s1, false),
				"s2": NewEnv("s2", s2, true),
			},
			&nop.Logger{},
		)

		_, err := p.ListImages(context.Background())
		require.NoError(t, err)

		// since that s3 may be unavailable, no error should be returned
		require.NoError(t, p.PutImage(context.Background(), makeImageFile(t), "500"))
	})

	t.Run("fail put to s3 (not possibly unavailable) should be returned ", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		s1 := s3mocks.NewMockClient(ctrl)
		s1.EXPECT().PutObject(gomock.Any(), "s1", gomock.Any(), gomock.Any(), gomock.Any()).Return(io.EOF).Times(1)

		cfg := DefaultConfig()
		p := New(
			cfg,
			map[string]Env{
				"s1": NewEnv("s1", s1, false),
			},
			&nop.Logger{},
		)

		require.Error(t, p.PutImage(context.Background(), makeImageFile(t), "500"))
	})
}
