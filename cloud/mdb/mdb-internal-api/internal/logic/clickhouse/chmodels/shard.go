package chmodels

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/valid"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const shardNamePattern = "^[a-zA-Z0-9][a-zA-Z0-9-]*$"

// NewShardNameValidator constructs validator for shard names
func NewShardNameValidator(pattern string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "shard name %q is too short",
			TooLongMsg:  "shard name %q is too long",
		},
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "shard name %q has invalid symbols",
		},
	)
}

// MustShardNameValidator constructs validator for shard names. Panics on error.
func MustShardNameValidator(pattern string) *valid.StringComposedValidator {
	v, err := NewShardNameValidator(pattern)
	if err != nil {
		panic(err)
	}

	return v
}

var shardNameValidator = MustShardNameValidator(shardNamePattern)

func ValidateShardName(name string) error {
	return shardNameValidator.ValidateString(name)
}

func ParseShardName(shardName string) (int, error) {
	var ind int
	_, err := fmt.Sscanf(shardName, ShardNameTemplate, &ind)
	if err != nil {
		return ind, xerrors.Errorf("parsing shard name %q: %w", shardName, err)
	}
	return ind, nil
}

func SortShardsByName(shards []clusters.Shard) {
	sort.Slice(shards, func(i, j int) bool {
		iInd, err := ParseShardName(shards[i].Name)
		if err != nil {
			panic(err)
		}
		jInd, err := ParseShardName(shards[j].Name)
		if err != nil {
			panic(err)
		}

		return iInd < jInd
	})
}

func GenerateShardName(shardIndex int) string {
	return fmt.Sprintf(ShardNameTemplate, shardIndex)
}

type Shard struct {
	clusters.Shard
	ClusterID string
	Config    ShardConfig
}

type ShardConfig struct {
	Weight    int64
	ConfigSet ClickhouseConfigSet
	Resources models.ClusterResources
}
