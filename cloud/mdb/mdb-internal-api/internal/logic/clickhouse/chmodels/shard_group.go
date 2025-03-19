package chmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
)

const shardGroupNamePattern = "^[a-zA-Z0-9_-]+$"

type ShardGroup struct {
	Name        string
	ClusterID   string
	Description string
	ShardNames  []string
}

type UpdateShardGroupArgs struct {
	Name        string
	ClusterID   string
	Description optional.String
	ShardNames  optional.Strings
}

// NewShardGroupNameValidator constructs validator for shard group names
func NewShardGroupNameValidator(pattern string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "shard group name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "shard group name %q is too short",
			TooLongMsg:  "shard group name %q is too long",
		},
	)
}

// MustShardGroupNameValidator constructs validator for shard group names. Panics on error.
func MustShardGroupNameValidator(pattern string) *valid.StringComposedValidator {
	v, err := NewShardGroupNameValidator(pattern)
	if err != nil {
		panic(err)
	}

	return v
}

var shardGroupNameValidator = MustShardGroupNameValidator(shardGroupNamePattern)

func ValidateShardGroupName(name string, cid string) error {
	if name == cid {
		return semerr.InvalidInput("shard group name must not be euqal to cluster ID")
	}

	return shardGroupNameValidator.ValidateString(name)
}
