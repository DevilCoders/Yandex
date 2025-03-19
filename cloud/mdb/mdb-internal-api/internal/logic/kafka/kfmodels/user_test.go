package kfmodels

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestUserSpec_Validate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		us := UserSpec{
			Name: "test",
		}
		err := us.Validate()
		require.NoError(t, err)
	})
	t.Run("user with mdb_ prefix fails", func(t *testing.T) {
		us := UserSpec{
			Name: "mdb_test",
		}
		err := us.Validate()
		require.EqualError(t, err, "invalid user name \"mdb_test\"")
	})
	t.Run("user with name default fails", func(t *testing.T) {
		us := UserSpec{
			Name: "default",
		}
		err := us.Validate()
		require.EqualError(t, err, "invalid user name \"default\"")
	})
	t.Run("user with too long name fails", func(t *testing.T) {
		us := UserSpec{
			Name: "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee",
		}
		err := us.Validate()
		require.EqualError(t, err, "user name \"aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee\" is too long")
	})
	t.Run("user with empty name fails", func(t *testing.T) {
		us := UserSpec{}
		err := us.Validate()
		require.EqualError(t, err, "user name \"\" has invalid symbols")
	})
	t.Run("user with empty permissions fails", func(t *testing.T) {
		us := UserSpec{
			Name:        "test",
			Permissions: []Permission{{}},
		}
		err := us.Validate()
		require.EqualError(t, err, "permission must be non empty")
	})
}

func TestPermission_Validate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		p := Permission{
			TopicName:  "test",
			AccessRole: AccessRoleProducer,
		}
		err := p.Validate()
		require.NoError(t, err)
	})

	t.Run("empty permission fails", func(t *testing.T) {
		p := Permission{}
		err := p.Validate()
		require.EqualError(t, err, "permission must be non empty")
	})
	t.Run("unspecified role fails", func(t *testing.T) {
		p := Permission{
			AccessRole: AccessRoleUnspecified,
		}
		err := p.Validate()
		require.EqualError(t, err, "permission must be non empty")
	})
	t.Run("empty topic name fails", func(t *testing.T) {
		p := Permission{
			AccessRole: AccessRoleProducer,
		}
		err := p.Validate()
		require.EqualError(t, err, "topic name \"\" is too short")
	})
	t.Run("topic name with invalid symbols fails", func(t *testing.T) {
		p := Permission{
			TopicName:  "**invalid##",
			AccessRole: AccessRoleProducer,
		}
		err := p.Validate()
		require.EqualError(t, err, "topic name \"**invalid##\" has invalid symbols")
	})
	t.Run("too long topic namefails", func(t *testing.T) {
		p := Permission{
			TopicName:  "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee",
			AccessRole: AccessRoleProducer,
		}
		err := p.Validate()
		require.EqualError(t, err, "topic name \"aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee\" is too long")
	})
}
