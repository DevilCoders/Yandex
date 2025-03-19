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

type ChannelTelegramMethod struct {
	GroupTitle     string `yaml:"group_title"`
	TextTemplate   string `yaml:"text_template"`
	SendScreenshot bool   `yaml:"send_screenshot"`
}

type ChannelJugglerMethod struct {
	Host        string   `yaml:"host"`
	Service     string   `yaml:"service"`
	Instance    string   `yaml:"instance"`
	Description string   `yaml:"description"`
	Tags        []string `yaml:"tags"`
}

type ChannelMethod struct {
	Telegram *ChannelTelegramMethod `yaml:"telegram,omitempty"`
	Juggler  *ChannelJugglerMethod  `yaml:"juggler,omitempty"`
}

type ChannelConfig struct {
	ID       string   `yaml:"id"`
	Clusters []string `yaml:"clusters"`
	Name     string   `yaml:"name"`

	Method ChannelMethod `yaml:"method"`
}

type ChannelConfigs struct {
	Channels []ChannelConfig `yaml:"channels"`
}

////////////////////////////////////////////////////////////////////////////////

func loadChannelConfigsFromFile(filePath string) ([]ChannelConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	channelConfigs := &ChannelConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), channelConfigs); err != nil {
		return nil, err
	}

	return channelConfigs.Channels, nil
}

func loadChannelConfigsFromPath(path string) ([]ChannelConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []ChannelConfig

	for _, file := range files {
		channelConfigs, err := loadChannelConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, channelConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonChannel(
	channelConfig ChannelConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Channel, error) {

	var err error

	var channel solomon.Channel
	channel.ProjectID = solomonProjectID

	channel.ID, err = readTmplString(channelConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Channel{}, err
	}

	channel.Name, err = readTmplString(channelConfig.Name, clusterConfig)
	if err != nil {
		return solomon.Channel{}, err
	}

	if channelConfig.Method.Juggler != nil {
		juggler := channelConfig.Method.Juggler

		channel.Method.Juggler = &solomon.ChannelJugglerMethod{}
		channel.Method.Juggler.Host, err = readTmplString(juggler.Host, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}

		channel.Method.Juggler.Service, err = readTmplString(juggler.Service, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}

		channel.Method.Juggler.Instance, err = readTmplString(juggler.Instance, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}

		channel.Method.Juggler.Description, err = readTmplString(juggler.Description, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}

		channel.Method.Juggler.Tags, err = readTmplStringArr(juggler.Tags, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}
	}
	if channelConfig.Method.Telegram != nil {
		telegram := channelConfig.Method.Telegram
		channel.Method.Telegram = &solomon.ChannelTelegramMethod{
			SendScreenshot: telegram.SendScreenshot,
		}

		channel.Method.Telegram.GroupTitle, err = readTmplString(telegram.GroupTitle, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}

		channel.Method.Telegram.TextTemplate, err = readTmplString(telegram.TextTemplate, clusterConfig)
		if err != nil {
			return solomon.Channel{}, err
		}
	}

	return channel, nil
}

func createSolomonChannels(
	channelConfigs []ChannelConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Channel, error) {

	projectID := serviceConfig.SolomonProjectID

	channelIds := make(map[string]bool)
	var channels []solomon.Channel

	for _, channelConfig := range channelConfigs {
		if channelConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}

			channel, err := createSolomonChannel(channelConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := channelIds[channel.ID]; ok {
				return nil, fmt.Errorf("multiple channels with the same ID: %v", channel.ID)
			}
			channelIds[channel.ID] = false
			channels = append(channels, channel)
			continue
		}

		for _, clusterID := range channelConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					channel, err := createSolomonChannel(channelConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := channelIds[channel.ID]; ok {
						return nil, fmt.Errorf("multiple channels with the same ID: %v", channel.ID)
					}
					channelIds[channel.ID] = false
					channels = append(channels, channel)
					break
				}
			}
		}
	}
	return channels, nil
}

func loadSolomonChannelIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonChannelIds, err := solomonClient.ListChannels(ctx)
	if err != nil {
		return nil, err
	}

	solomonChannels := make(map[string]bool)

	for _, channelID := range solomonChannelIds {
		solomonChannels[channelID] = false
	}

	return solomonChannels, nil
}

////////////////////////////////////////////////////////////////////////////////

type ChannelManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *ChannelManager) Run(ctx context.Context) error {
	channelConfigs, err := loadChannelConfigsFromPath(a.opts.ChannelsPath)
	if err != nil {
		return err
	}

	channels, err := createSolomonChannels(channelConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonChannelIds, err := loadSolomonChannelIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedChannelCount := 0
	changedChannelCount := 0
	unchangedChannelCount := 0
	for _, channel := range channels {
		if _, ok := solomonChannelIds[channel.ID]; ok {
			solomonChannelIds[channel.ID] = true

			solomonChannel, err := a.solomonClient.GetChannel(ctx, channel.ID)
			if err != nil {
				return err
			}

			channel.Version = solomonChannel.Version

			if reflect.DeepEqual(solomonChannel, channel) {
				unchangedChannelCount++
			} else {
				changedChannelCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update channel: ", channel.ID)
					_, err := a.solomonClient.UpdateChannel(ctx, channel)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update channel: ", channel.ID)
				}

				if a.opts.ShowDiff {
					oldChannel, err := json.MarshalIndent(solomonChannel, "", "  ")
					if err != nil {
						return err
					}

					newChannel, err := json.MarshalIndent(channel, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(channel.ID, oldChannel, newChannel)
					if err != nil {
						return err
					}
				}
			}

		} else {
			addedChannelCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add channel: ", channel.ID)
				_, err := a.solomonClient.AddChannel(ctx, channel)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add channel: ", channel.ID)
			}
		}
	}

	var untrackedChannelIds []string
	for channelID, tracked := range solomonChannelIds {
		if tracked {
			continue
		}

		untrackedChannelIds = append(untrackedChannelIds, channelID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedChannelIds)
		for _, channelID := range untrackedChannelIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete channel: ", channelID)
				err = a.solomonClient.DeleteChannel(ctx, channelID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete channel: ", channelID)
			}
		}
	}

	if addedChannelCount != 0 {
		fmt.Println(colorGreen+"Added channels count:", addedChannelCount, colorReset)
	}

	if changedChannelCount != 0 {
		fmt.Println(colorYellow+"Changed channels count:", changedChannelCount, colorReset)
	}

	fmt.Println("Unchanged channels count:", unchangedChannelCount)

	if !a.opts.Remove && len(untrackedChannelIds) != 0 {
		fmt.Printf(colorRed+"Untracked channels count:%d, %v\n"+colorReset, len(untrackedChannelIds), untrackedChannelIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewChannelManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *ChannelManager {

	return &ChannelManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
