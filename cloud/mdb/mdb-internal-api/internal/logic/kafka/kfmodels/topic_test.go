package kfmodels

import (
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestTopicSpec_Validate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		us := TopicSpec{
			Name: "test",
		}
		require.NoError(t, us.Validate())
	})

	t.Run("When topic name has length 249 should not return error", func(t *testing.T) {
		us := TopicSpec{
			Name: strings.Repeat("a", 249),
		}
		require.NoError(t, us.Validate())
	})

	t.Run("When topic name has length more than 249 characters should return error", func(t *testing.T) {
		us := TopicSpec{
			Name: strings.Repeat("a", 250),
		}
		require.EqualError(t, us.Validate(), "topic name \""+us.Name+"\" is too long")
	})

	t.Run("When topic name enabled preallocate flag return error", func(t *testing.T) {
		preallocate := true
		us := TopicSpec{
			Name: "test",
			Config: TopicConfig{
				Preallocate: &preallocate,
			},
		}
		require.EqualError(t, us.Validate(), "preallocate flag is disabled due the kafka issue KAFKA-13664")
	})
}
