package s3_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource/s3"
	s3api "a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/s3/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func objectsFromKeys(s3keys []string) []s3api.Object {
	objects := make([]s3api.Object, len(s3keys))
	for i := 0; i < len(s3keys); i++ {
		objects[i].Key = s3keys[i]
	}
	return objects
}

func TestSource_LatestVersion(t *testing.T) {
	t.Run("Empty bucket", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		s3client := mocks.NewMockClient(mockCtrl)
		s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, nil, nil)

		source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
		v, err := source.LatestVersion(context.Background())

		require.Empty(t, v)
		require.Error(t, err)
	})
	for _, testCase := range []struct {
		name          string
		s3keys        []string
		latestVersion string
	}{
		{
			"one object",
			[]string{"image-10-r10.txz"},
			"10-r10",
		},
		{
			"3 objects",
			[]string{"image-10-r10.txz", "image-20-r20", "image-30-r30"},
			"30-r30",
		},
		{
			"object with same rev but different timestamps",
			[]string{"image-10-r10.txz", "image-20-r10", "image-30-r10"},
			"30-r10",
		},
	} {
		t.Run(testCase.name, func(t *testing.T) {
			mockCtrl := gomock.NewController(t)
			defer mockCtrl.Finish()
			s3client := mocks.NewMockClient(mockCtrl)

			s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(objectsFromKeys(testCase.s3keys), nil, nil)

			source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
			v, err := source.LatestVersion(context.Background())

			require.Equal(t, testCase.latestVersion, v)
			require.NoError(t, err)
		})
	}
}

func TestSource_ResolveVersion(t *testing.T) {
	t.Run("Empty bucket", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		s3client := mocks.NewMockClient(mockCtrl)
		s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, nil, nil)

		source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
		v, err := source.ResolveVersion(context.Background(), "42")

		require.Empty(t, v)
		require.Error(t, err)
	})

	t.Run("one object match suffix", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		s3client := mocks.NewMockClient(mockCtrl)

		s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(objectsFromKeys([]string{
			"image-10-r10.txz", "image-20-r20.txz", "image-30-r30.txz",
		}), nil, nil)

		source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
		v, err := source.ResolveVersion(context.Background(), "r20")

		require.NoError(t, err)
		require.Equal(t, "20-r20", v)
	})

	t.Run("more than one object match suffix", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		s3client := mocks.NewMockClient(mockCtrl)

		s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(objectsFromKeys([]string{
			"image-10-r10.txz", "image-20-r10.txz", "image-30-r30.txz",
		}), nil, nil)

		source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
		v, err := source.ResolveVersion(context.Background(), "r10")

		require.Equal(t, "", v)
		require.Error(t, err)
	})

	t.Run("no objects that match suffix", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		s3client := mocks.NewMockClient(mockCtrl)

		s3client.EXPECT().ListObjects(gomock.Any(), gomock.Any(), gomock.Any()).Return(objectsFromKeys([]string{
			"image-10-r10.txz", "image-20-r10.txz", "image-30-r30.txz",
		}), nil, nil)

		source := s3.New(s3client, s3.DefaultConfig(), &nop.Logger{})
		v, err := source.ResolveVersion(context.Background(), "r40")

		require.Equal(t, "", v)
		require.Error(t, err)
	})
}
