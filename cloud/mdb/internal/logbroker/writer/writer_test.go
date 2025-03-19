package writer_test

import (
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestWriteAndWait(t *testing.T) {
	id100 := int64(100)
	id500 := int64(500)

	t.Run("do nothing when no events", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)

		written, err := writer.WriteAndWait(m, nil)

		require.NoError(t, err)
		require.Empty(t, written)
	})

	t.Run("write events and get response for them", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)
		responses := make(chan writer.WriteResponse, 2)
		responses <- writer.WriteResponse{ID: id100}
		responses <- writer.WriteResponse{ID: id500}
		defer close(responses)

		m.EXPECT().Write(id100, nil, time.Time{}).Return(nil)
		m.EXPECT().Write(id500, nil, time.Time{}).Return(nil)
		m.EXPECT().WriteResponses().Return(responses)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}})

		require.NoError(t, err)
		require.Equal(t, []int64{id100, id500}, written)
	})

	t.Run("first write fails", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)

		retErr := xerrors.Errorf("write fail")
		m.EXPECT().Write(id100, nil, time.Time{}).Return(retErr)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}})

		require.Error(t, err)
		require.True(t, xerrors.Is(err, retErr))
		require.Empty(t, written)
	})

	t.Run("write fails after successful write", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)
		responses := make(chan writer.WriteResponse, 2)
		responses <- writer.WriteResponse{ID: id100}
		defer close(responses)

		m.EXPECT().Write(id100, nil, time.Time{}).Return(nil)
		m.EXPECT().Write(id500, nil, time.Time{}).Return(xerrors.New("write error"))
		m.EXPECT().WriteResponses().Return(responses)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}})

		require.NoError(t, err)
		require.Equal(t, []int64{id100}, written)
	})

	t.Run("part of documents response successfully", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)
		responses := make(chan writer.WriteResponse, 2)
		responses <- writer.WriteResponse{ID: id100, Err: xerrors.New("response err")}
		responses <- writer.WriteResponse{ID: id500}
		defer close(responses)
		m.EXPECT().Write(id100, nil, time.Time{}).Return(nil)
		m.EXPECT().Write(id500, nil, time.Time{}).Return(nil)
		m.EXPECT().WriteResponses().Return(responses)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}})

		require.NoError(t, err)
		require.Equal(t, []int64{id500}, written)
	})

	t.Run("all responses was failed", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)

		failErr := xerrors.New("response err")
		responses := make(chan writer.WriteResponse, 2)
		responses <- writer.WriteResponse{ID: id100, Err: failErr}
		responses <- writer.WriteResponse{ID: id500, Err: failErr}
		defer close(responses)
		m.EXPECT().Write(id100, nil, time.Time{}).Return(nil)
		m.EXPECT().Write(id500, nil, time.Time{}).Return(nil)
		m.EXPECT().WriteResponses().Return(responses)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}})

		require.Error(t, err)
		require.Empty(t, written)
	})
	t.Run("Feedback doesn't returning withing timeout", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		m := mocks.NewMockWriter(ctrl)

		responses := make(chan writer.WriteResponse)
		defer close(responses)
		m.EXPECT().Write(id100, nil, time.Time{}).Return(nil)
		m.EXPECT().Write(id500, nil, time.Time{}).Return(nil)
		m.EXPECT().WriteResponses().Return(responses)

		written, err := writer.WriteAndWait(m, []writer.Doc{{ID: id100}, {ID: id500}}, writer.FeedbackTimeout(time.Millisecond))

		require.Error(t, err)
		require.Empty(t, written)
	})
}
