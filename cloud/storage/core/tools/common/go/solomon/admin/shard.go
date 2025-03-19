package admin

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"reflect"
	"sort"

	"gopkg.in/yaml.v2"

	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type ShardConfig struct {
	ID       string   `yaml:"id"`
	Clusters []string `yaml:"clusters"`
	Version  uint     `yaml:"version"`

	ClusterID       string `yaml:"cluster_id"`
	ClusterName     string `yaml:"cluster_name"`
	DecimPolicy     string `yaml:"decim_policy"`
	SensorNameLabel string `yaml:"sensor_name_label"`
	SensorsTTLDays  int    `yaml:"sensors_ttl_days"`
	ServiceID       string `yaml:"service_id"`
	ServiceName     string `yaml:"service_name"`
	State           string `yaml:"state"`
}

type ShardsConfigs struct {
	Shards []ShardConfig `yaml:"shards"`
}

////////////////////////////////////////////////////////////////////////////////

func loadShardsConfigsFromFile(filePath string) ([]ShardConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	shardsConfigs := &ShardsConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), shardsConfigs); err != nil {
		return nil, err
	}

	return shardsConfigs.Shards, nil
}

func loadShardsConfigsFromPath(path string) ([]ShardConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []ShardConfig

	for _, file := range files {
		shardConfigs, err := loadShardsConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, shardConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonShard(
	shardConfig ShardConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Shard, error) {

	var err error

	var shard solomon.Shard
	shard.ProjectID = solomonProjectID

	shard.ID, err = readTmplString(shardConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.ClusterID, err = readTmplString(shardConfig.ClusterID, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.ClusterName, err = readTmplString(shardConfig.ClusterName, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.DecimPolicy, err = readTmplString(shardConfig.DecimPolicy, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.SensorNameLabel, err = readTmplString(shardConfig.SensorNameLabel, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.ServiceID, err = readTmplString(shardConfig.ServiceID, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.ServiceName, err = readTmplString(shardConfig.ServiceName, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.State, err = readTmplString(shardConfig.State, clusterConfig)
	if err != nil {
		return solomon.Shard{}, err
	}

	shard.SensorsTTLDays = shardConfig.SensorsTTLDays

	return shard, nil
}

func createSolomonShards(
	shardConfigs []ShardConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Shard, error) {

	projectID := serviceConfig.SolomonProjectID

	shardIds := make(map[string]bool)
	var shards []solomon.Shard

	for _, shardConfig := range shardConfigs {
		if shardConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}

			shard, err := createSolomonShard(shardConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := shardIds[shard.ID]; ok {
				return nil, fmt.Errorf("multiple shards with the same ID: %v", shard.ID)
			}
			shardIds[shard.ID] = false
			shards = append(shards, shard)
			continue
		}

		for _, clusterID := range shardConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					shard, err := createSolomonShard(shardConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := shardIds[shard.ID]; ok {
						return nil, fmt.Errorf("multiple shards with the same ID: %v", shard.ID)
					}
					shardIds[shard.ID] = false
					shards = append(shards, shard)
					break
				}
			}
		}
	}
	return shards, nil
}

func loadSolomonShardIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonShardIds, err := solomonClient.ListShards(ctx)
	if err != nil {
		return nil, err
	}

	solomonShards := make(map[string]bool)

	for _, shardID := range solomonShardIds {
		solomonShards[shardID] = false
	}

	return solomonShards, nil
}

////////////////////////////////////////////////////////////////////////////////

type ShardManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *ShardManager) Run(ctx context.Context) error {
	shardConfigs, err := loadShardsConfigsFromPath(a.opts.ShardsPath)
	if err != nil {
		return err
	}

	shards, err := createSolomonShards(shardConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonShardIds, err := loadSolomonShardIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedShardCount := 0
	changedShardCount := 0
	unchangedShardCount := 0
	for _, shard := range shards {
		if _, ok := solomonShardIds[shard.ID]; ok {
			solomonShardIds[shard.ID] = true

			solomonShard, err := a.solomonClient.GetShard(ctx, shard.ID)
			if err != nil {
				return err
			}

			shard.Version = solomonShard.Version

			if reflect.DeepEqual(solomonShard, shard) {
				unchangedShardCount++
			} else {
				changedShardCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update shard: ", shard.ID)
					_, err := a.solomonClient.UpdateShard(ctx, shard)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update shard: ", shard.ID)
				}

				if a.opts.ShowDiff {
					oldShard, err := json.MarshalIndent(solomonShard, "", "  ")
					if err != nil {
						return err
					}

					newShard, err := json.MarshalIndent(shard, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(shard.ID, oldShard, newShard)
					if err != nil {
						return err
					}
				}
			}

		} else {
			addedShardCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add shard: ", shard.ID)
				_, err := a.solomonClient.AddShard(ctx, shard)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add shard: ", shard.ID)
			}
		}
	}

	var untrackedShardIds []string
	for shardID, tracked := range solomonShardIds {
		if tracked {
			continue
		}

		untrackedShardIds = append(untrackedShardIds, shardID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedShardIds)
		for _, shardID := range untrackedShardIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete shard: ", shardID)
				err = a.solomonClient.DeleteShard(ctx, shardID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete shard: ", shardID)
			}
		}
	}

	if addedShardCount != 0 {
		fmt.Println(colorGreen+"Added shards count:", addedShardCount, colorReset)
	}

	if changedShardCount != 0 {
		fmt.Println(colorYellow+"Changed shards count:", changedShardCount, colorReset)
	}

	fmt.Println("Unchanged shards count:", unchangedShardCount)

	if !a.opts.Remove && len(untrackedShardIds) != 0 {
		fmt.Printf(colorRed+"Untracked shards count:%d, %v\n"+colorReset, len(untrackedShardIds), untrackedShardIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewShardManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *ShardManager {

	return &ShardManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
