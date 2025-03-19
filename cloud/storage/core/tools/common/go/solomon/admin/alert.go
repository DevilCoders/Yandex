package admin

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"reflect"
	"sort"
	"strings"

	"gopkg.in/yaml.v2"

	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type AlertChannel struct {
	ID                  string   `yaml:"id"`
	NotifyAboutStatuses []string `yaml:"notify_about_statuses"`
	ReNotification      uint     `yaml:"re_notification"`
}

type AlertAnnotations struct {
	Tags               string `yaml:"tags"`
	Host               string `yaml:"host"`
	Service            string `yaml:"service"`
	Content            string `yaml:"content"`
	GraphLink          string `yaml:"graph_link"`
	JugglerDescription string `yaml:"juggler_description"`
}

type AlertExpression struct {
	Program string `yaml:"program"`
	Check   string `yaml:"check"`
}

type AlertConfig struct {
	ID                  string            `yaml:"id"`
	Clusters            []string          `yaml:"clusters"`
	Name                string            `yaml:"name"`
	State               *string           `yaml:"state"`
	Description         string            `json:"description"`
	Debug               string            `json:"debug"`
	WindowSecs          *uint             `yaml:"window_secs"`
	DelaySecs           *uint             `yaml:"delay_secs"`
	GroupByLabels       []string          `yaml:"group_by_labels"`
	Channels            []AlertChannel    `yaml:"channels"`
	Annotations         *AlertAnnotations `yaml:"annotations"`
	Expression          AlertExpression   `yaml:"expression"`
	ResolvedEmptyPolicy string            `yaml:"resolved_empty_policy"`
}

type AlertConfigs struct {
	Alerts []AlertConfig `yaml:"alerts"`
}

////////////////////////////////////////////////////////////////////////////////

func loadAlertConfigsFromFile(filePath string) ([]AlertConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	alertConfigs := &AlertConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), alertConfigs); err != nil {
		return nil, err
	}

	return alertConfigs.Alerts, nil
}

func loadAlertConfigsFromPath(path string) ([]AlertConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []AlertConfig

	for _, file := range files {
		if !strings.HasSuffix(file, ".yaml") {
			continue
		}
		alertConfigs, err := loadAlertConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, alertConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func prepareAlertConfigs(
	alertConfigs []AlertConfig,
	serviceConfig *ServiceConfig,
) error {

	defaults := &serviceConfig.Default

	for i := 0; i < len(alertConfigs); i++ {
		alertConfig := &alertConfigs[i]

		if alertConfig.State == nil {
			alertConfig.State = &defaults.AlertState
		}
		if alertConfig.WindowSecs == nil {
			alertConfig.WindowSecs = &defaults.AlertWindowSecs
		}
		if alertConfig.DelaySecs == nil {
			alertConfig.DelaySecs = &defaults.AlertDelaySecs
		}
	}
	return nil
}

func createSolomonAlert(
	alertConfig AlertConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Alert, error) {

	var err error

	var alert solomon.Alert
	alert.ProjectID = solomonProjectID

	alert.ID, err = readTmplString(alertConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Alert{}, err
	}

	alert.Name, err = readTmplString(alertConfig.Name, clusterConfig)
	if err != nil {
		return solomon.Alert{}, err
	}

	alert.GroupByLabels = append(alert.GroupByLabels, alertConfig.GroupByLabels...)
	alert.ResolvedEmptyPolicy = "RESOLVED_EMPTY_DEFAULT"
	if len(alertConfig.ResolvedEmptyPolicy) != 0 {
		alert.ResolvedEmptyPolicy = alertConfig.ResolvedEmptyPolicy
	}

	if alertConfig.Annotations != nil {
		alert.Annotations.Tags, err = readTmplString(alertConfig.Annotations.Tags, clusterConfig)
		if err != nil {
			return solomon.Alert{}, err
		}

		alert.Annotations.Host, err = readTmplString(alertConfig.Annotations.Host, clusterConfig)
		if err != nil {
			return solomon.Alert{}, err
		}

		alert.Annotations.Service = alertConfig.Annotations.Service
		alert.Annotations.Content = alertConfig.Annotations.Content
		alert.Annotations.GraphLink = alertConfig.Annotations.GraphLink
		alert.Annotations.JugglerDescription = alertConfig.Annotations.JugglerDescription
	}

	alert.Channels = make([]solomon.AlertChannel, 0)

	for _, channel := range alertConfig.Channels {
		var solomonChannel solomon.AlertChannel
		solomonChannel.ID = channel.ID
		solomonChannel.Config.RepeatDelaySecs = channel.ReNotification
		solomonChannel.Config.NotifyAboutStatuses = append(
			solomonChannel.Config.NotifyAboutStatuses,
			channel.NotifyAboutStatuses...)

		alert.Channels = append(alert.Channels, solomonChannel)
	}

	alert.Annotations.Debug = alertConfig.Debug

	alert.State = *alertConfig.State
	alert.PeriodMillis = *alertConfig.WindowSecs * 1000
	alert.WindowSecs = *alertConfig.WindowSecs
	alert.Description = alertConfig.Description
	alert.DelaySeconds = *alertConfig.DelaySecs
	alert.DelaySecs = *alertConfig.DelaySecs

	alert.Type.Expression.Program, err = readTmplString(alertConfig.Expression.Program, clusterConfig)
	if err != nil {
		return solomon.Alert{}, err
	}
	alert.Type.Expression.CheckExpression, err = readTmplString(alertConfig.Expression.Check, clusterConfig)
	if err != nil {
		return solomon.Alert{}, err
	}

	return alert, nil
}

func createSolomonAlerts(
	alertConfigs []AlertConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Alert, error) {

	projectID := serviceConfig.SolomonProjectID

	alertIds := make(map[string]bool)
	var alerts []solomon.Alert

	for _, alertConfig := range alertConfigs {
		if alertConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}
			alert, err := createSolomonAlert(alertConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := alertIds[alert.ID]; ok {
				return nil, fmt.Errorf("multiple alerts with the same ID: %v", alert.ID)
			}
			alertIds[alert.ID] = false
			alerts = append(alerts, alert)
			continue
		}

		for _, clusterID := range alertConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					alert, err := createSolomonAlert(alertConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := alertIds[alert.ID]; ok {
						return nil, fmt.Errorf("multiple alerts with the same ID: %v", alert.ID)
					}
					alertIds[alert.ID] = false
					alerts = append(alerts, alert)
					break
				}
			}
		}
	}
	return alerts, nil
}

func loadSolomonAlertIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonAlertIds, err := solomonClient.ListAlerts(ctx)
	if err != nil {
		return nil, err
	}

	solomonAlerts := make(map[string]bool)

	for _, alertID := range solomonAlertIds {
		solomonAlerts[alertID] = false
	}

	return solomonAlerts, nil
}

////////////////////////////////////////////////////////////////////////////////

type AlertManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *AlertManager) Run(ctx context.Context) error {
	alertConfigs, err := loadAlertConfigsFromPath(a.opts.AlertsPath)
	if err != nil {
		return err
	}

	err = prepareAlertConfigs(alertConfigs, a.config)
	if err != nil {
		return err
	}

	alerts, err := createSolomonAlerts(alertConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonAlertIds, err := loadSolomonAlertIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedAlertCount := 0
	changedAlertCount := 0
	unchangedAlertCount := 0
	for _, alert := range alerts {
		if _, ok := solomonAlertIds[alert.ID]; ok {
			solomonAlertIds[alert.ID] = true

			solomonAlert, err := a.solomonClient.GetAlert(ctx, alert.ID)
			if err != nil {
				return err
			}

			alert.Version = solomonAlert.Version

			// Normalize statuses.
			for _, ch := range solomonAlert.Channels {
				sort.Strings(ch.Config.NotifyAboutStatuses)
			}
			for _, ch := range alert.Channels {
				sort.Strings(ch.Config.NotifyAboutStatuses)
			}

			if reflect.DeepEqual(solomonAlert, alert) {
				unchangedAlertCount++
			} else {
				changedAlertCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update alert: ", alert.ID)
					_, err := a.solomonClient.UpdateAlert(ctx, alert)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update alert: ", alert.ID)
				}

				if a.opts.ShowDiff {
					oldAlert, err := json.MarshalIndent(solomonAlert, "", "  ")
					if err != nil {
						return err
					}

					newAlert, err := json.MarshalIndent(alert, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(alert.ID, oldAlert, newAlert)
					if err != nil {
						return nil
					}

					err = printDiff(
						alert.ID,
						[]byte(solomonAlert.Type.Expression.Program),
						[]byte(alert.Type.Expression.Program),
					)
					if err != nil {
						return nil
					}
				}
			}

		} else {
			addedAlertCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add alert: ", alert.ID)
				_, err := a.solomonClient.AddAlert(ctx, alert)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add alert: ", alert.ID)
			}
		}
	}

	var untrackedAlertIds []string
	for alertID, tracked := range solomonAlertIds {
		if tracked {
			continue
		}

		untrackedAlertIds = append(untrackedAlertIds, alertID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedAlertIds)
		for _, alertID := range untrackedAlertIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete alert: ", alertID)
				err = a.solomonClient.DeleteAlert(ctx, alertID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete alert: ", alertID)
			}
		}
	}

	if addedAlertCount != 0 {
		fmt.Println(colorGreen+"Added alerts count:", addedAlertCount, colorReset)
	}

	if changedAlertCount != 0 {
		fmt.Println(colorYellow+"Changed alerts count:", changedAlertCount, colorReset)
	}

	fmt.Println("Unchanged alerts count:", unchangedAlertCount, colorReset)

	if !a.opts.Remove && len(untrackedAlertIds) != 0 {
		fmt.Printf(colorRed+"Untracked alerts count:%d, %v\n"+colorReset, len(untrackedAlertIds), untrackedAlertIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewAlertManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *AlertManager {

	return &AlertManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
