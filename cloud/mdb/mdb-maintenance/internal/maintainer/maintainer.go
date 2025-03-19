package maintainer

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/notifier"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/cms"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/stat"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	TasksVersion            = 2
	DatetimeTemplate string = "2006-01-02T15:04:05"
)

type Config struct {
	CreatedByUser          string            `json:"created_by_user" yaml:"created_by_user"`
	AllowClustersWithoutMW bool              `json:"allow_clusters_without_mw" yaml:"allow_clusters_without_mw"`
	NewlyPlannedLimit      uint64            `json:"newly_planned_limit" yaml:"newly_planned_limit"`
	NewlyPlannedClouds     int               `json:"newly_planned_clouds" yaml:"newly_planned_clouds"`
	TaskIDPrefixes         map[string]string `json:"task_id_prefixes" yaml:"task_id_prefixes"`
	NotifyUserDirectly     bool              `json:"notify_user_directly" yaml:"notify_user_directly"`
	// Cloud IDs which have to be notified by cloud id, instead of user id.
	NotifyUserDirectlyDenylist []string `json:"notify_user_directly_denylist" yaml:"notify_user_directly_denylist"`
	// Maximum of various elements to log, ex. clusterIDs
	MaxElementsLog int `json:"max_clusters_log" yaml:"max_clusters_log"`
	Priority       int `json:"priority" yaml:"priority"`
}

func DefaultConfig() Config {
	return Config{
		NewlyPlannedClouds: 100,
		NewlyPlannedLimit:  1000,
		MaxElementsLog:     100,
	}
}

type MaintainAction struct {
	cluster  models.Cluster
	planTS   time.Time
	createTS time.Time
	typ      actionType
}

type actionType string

const (
	actionCreate  actionType = "create"
	actionRepeat  actionType = "repeat"
	actionRestore actionType = "restore" // Restore cancelled by user task
	actionRetry   actionType = "retry"   // Reschedule failed task
	actionCancel  actionType = "cancel"
)

func (at actionType) NeedNotification() bool {
	switch at {
	case actionCreate, actionRetry, actionRepeat:
		return true
	}
	return false
}

type Maintainer struct {
	CMS                   cms.CMS
	L                     log.Logger
	MetaDB                metadb.MetaDB
	Cal                   holidays.Calendar
	Config                Config
	taskIDGens            map[string]generator.IDGenerator
	stat                  stat.MaintenanceStat
	sender                notifier.API
	resClient             resmanager.Client
	now                   func() time.Time
	hasHigherPriorityTask map[string]int
}

func New(cms cms.CMS, L log.Logger, MetaDB metadb.MetaDB, Config Config, stat stat.MaintenanceStat, sender notifier.API, resClient resmanager.Client, cal holidays.Calendar) *Maintainer {
	generators := make(map[string]generator.IDGenerator, len(Config.TaskIDPrefixes))
	for t, p := range Config.TaskIDPrefixes {
		generators[t] = generator.NewTaskIDGenerator(p)
	}
	prioritiesMap := make(map[string]int)
	return &Maintainer{
		CMS:                   cms,
		L:                     L,
		MetaDB:                MetaDB,
		Cal:                   cal,
		Config:                Config,
		taskIDGens:            generators,
		stat:                  stat,
		sender:                sender,
		resClient:             resClient,
		now:                   time.Now,
		hasHigherPriorityTask: prioritiesMap,
	}
}

func (m *Maintainer) SelectClusters(ctx context.Context, taskConfig models.MaintenanceTaskConfig) ([]models.Cluster, error) {
	if taskConfig.ClustersSelection.Empty() {
		return nil, xerrors.Errorf("selection is empty %s", taskConfig.Info)
	} else if taskConfig.ClustersSelection.DB != "" {
		ctxlog.Debug(ctx, m.L, "using meta for cluster selection")
		return m.MetaDB.SelectClusters(ctx, taskConfig)
	} else if m.CMS == nil {
		ctxlog.Errorf(ctx, m.L, "no CMS configured, needed for %q", taskConfig.Info)
		return nil, nil
	} else {
		ctxlog.Debug(ctx, m.L, "using cms for cluster selection")
		return cms.SelectFromCms(ctx, m.CMS, taskConfig.ClustersSelection.CMS)
	}
}

func (m *Maintainer) Maintain(ctx context.Context, taskConfig models.MaintenanceTaskConfig, priorities map[string]int) error {
	ctxlog.Infof(ctx, m.L, "Maintain config %v", taskConfig)
	var clusters []models.Cluster
	if !taskConfig.Disabled {
		clusters, err := m.SelectClusters(ctx, taskConfig)
		if err != nil {
			return err
		}
		if err := m.cancelAll(ctx, taskConfig, clusters, priorities); err != nil {
			return err
		}
	}

	clusters, err := m.SelectClusters(ctx, taskConfig)
	if err != nil {
		return err
	}

	var readyForNewTask []models.Cluster
	for _, cluster := range clusters {
		priority, alreadyInSomeTaskSelection := m.hasHigherPriorityTask[cluster.ID]
		// If it's first matching (in config - priority order) config for this cluster
		// or if not, but previous ones has priority like current [or lower, but it's not possible i guess]
		// then we can plan current config
		if !alreadyInSomeTaskSelection || (alreadyInSomeTaskSelection && priority <= priorities[taskConfig.ID]) {
			readyForNewTask = append(readyForNewTask, cluster)
		}

		// We should store maximum of cluster matching config priorities in hasHigherPriorityTask map.
		// So, we don't need to update existing values
		if !alreadyInSomeTaskSelection {
			m.hasHigherPriorityTask[cluster.ID] = taskConfig.Priority
		}
	}

	if len(clusters) <= m.Config.MaxElementsLog {
		ctxlog.Debugf(ctx, m.L, "selected clusters %v", toClusterIDs(clusters))
	} else {
		ctxlog.Debugf(ctx, m.L, "selected %d clusters", len(clusters))
	}

	if len(readyForNewTask) <= m.Config.MaxElementsLog {
		ctxlog.Debugf(ctx, m.L, "selected clusters that does not have higher priority task %v", toClusterIDs(readyForNewTask))
	} else {
		ctxlog.Debugf(ctx, m.L, "selected %d clusters that does not have higher priority task", len(readyForNewTask))
	}

	m.stat.ConfigSelectedClusterCount.With(map[string]string{"config_id": taskConfig.ID}).Add(int64(len(readyForNewTask)))

	existedTasks, err := m.MetaDB.MaintenanceTasks(ctx, taskConfig.ID)
	if err != nil {
		return err
	}
	existedTasksByClusterID := make(map[string]models.MaintenanceTask, len(existedTasks))
	for _, t := range existedTasks {
		existedTasksByClusterID[t.ClusterID] = t
	}

	if err := m.cancelConfig(ctx, readyForNewTask, existedTasksByClusterID, taskConfig); err != nil {
		return err
	}

	if taskConfig.Disabled {
		ctxlog.Infof(ctx, m.L, "config is disabled")
		return nil
	}

	if err := m.planConfig(ctx, readyForNewTask, existedTasksByClusterID, taskConfig); err != nil {
		return err
	}

	return nil
}

func (m *Maintainer) cancelAll(ctx context.Context, exceptTaskConfig models.MaintenanceTaskConfig, clusters []models.Cluster, priorities map[string]int) error {
	var CIDs []string
	for _, c := range clusters {
		CIDs = append(CIDs, c.ID)
	}

	tasks, err := m.MetaDB.MaintenanceTasksByCIDs(ctx, CIDs)
	if err != nil {
		return err
	}
	for _, task := range tasks {
		if priorities[task.ConfigID] < priorities[exceptTaskConfig.ID] && task.Status.IsCancellable() {
			ctxlog.Debugf(ctx, m.L, "cancel task '%q' (cluster '%q') ", task.TaskID.String, task.ClusterID)
			err = m.cancelMaintenanceTask(ctx, task)
			if err != nil {
				return xerrors.Errorf("cancel maintenance task: %w", err)
			}
		}
	}
	return nil
}

func (m *Maintainer) cancelConfig(ctx context.Context, clusters []models.Cluster, existedTasksByClusterID map[string]models.MaintenanceTask, taskConfig models.MaintenanceTaskConfig) error {
	toCancel := m.toCancel(clusters, existedTasksByClusterID, taskConfig.Disabled)

	ctxlog.Debugf(ctx, m.L, "cancel %d tasks", len(toCancel))
	for _, task := range toCancel {
		ctxlog.Infof(ctx, m.L, "cancel maintenance task for cluster %q", task.ClusterID)
		if err := m.cancelMaintenanceTask(ctx, task); err != nil {
			return xerrors.Errorf("cancel maintenance task: %w", err)
		}
		m.stat.ConfigPlanClusterCount.With(map[string]string{"config_id": taskConfig.ID, "action": string(actionCancel)}).Inc()
	}
	return nil
}

func (m *Maintainer) planConfig(ctx context.Context, clusters []models.Cluster, existedTasksByClusterID map[string]models.MaintenanceTask, taskConfig models.MaintenanceTaskConfig) error {
	toCreate, err := m.toCreate(ctx, clusters, existedTasksByClusterID, taskConfig.MinDays, taskConfig.MaxDays, taskConfig.Repeatable, taskConfig.EnvOrderDisabled, taskConfig.HoursDelay)
	if err != nil {
		return err
	}
	toRestore, err := m.toRestore(ctx, clusters, existedTasksByClusterID, taskConfig.MinDays, taskConfig.HoursDelay)
	if err != nil {
		return err
	}

	actionsByCloud := m.limitToCreate(ctx, toCreate)

	for _, action := range toRestore {
		actionsByCloud[action.cluster.CloudExtID] = append(actionsByCloud[action.cluster.CloudExtID], action)
	}

	ctxlog.Debugf(ctx, m.L, "apply %d plan actions for clusters", len(actionsByCloud))
	for cloudID, actions := range actionsByCloud {

		ctxlog.Debugf(ctx, m.L, "maintain cloud %q", cloudID)

		if err := m.notifyCloud(ctx, cloudID, actions, taskConfig.Info); err != nil {
			return err
		}

		for _, action := range actions {
			ctxlog.Infof(ctx, m.L, "plan maintenance %q task for cluster %q in cloud %q", action.typ, action.cluster.ID, cloudID)
			if err := m.planTask(ctx, action.cluster, taskConfig, action.planTS, action.createTS); err != nil {
				return err
			}
			m.stat.ConfigPlanClusterCount.With(map[string]string{"config_id": taskConfig.ID, "action": string(action.typ)}).Inc()
		}
	}
	return nil
}

func (m *Maintainer) toCreate(ctx context.Context, clusters []models.Cluster, existedTasksByClusterID map[string]models.MaintenanceTask,
	minDays int, maxDelayDays int, repeatConfig bool, envOrderDisabled bool, hoursDelay int) ([]MaintainAction, error) {
	actions := make([]MaintainAction, 0)

	if !envOrderDisabled {
		preFiltered := clusters
		clusters = m.filterByEnv(ctx, preFiltered)

		// log filtered out cluster IDs
		filteredOut := excludeClusters(preFiltered, clusters)
		if len(filteredOut) <= m.Config.MaxElementsLog {
			ctxlog.Debugf(ctx, m.L, "filtered by env: %v", toClusterIDs(filteredOut))
		} else {
			ctxlog.Debugf(ctx, m.L, "filtered by env: %d", len(filteredOut))
		}
	}

	var haveActiveTasks []string
	// check that there is no active task, and it is right time to plan
	for _, cluster := range clusters {
		if task, hasTaskForConfig := existedTasksByClusterID[cluster.ID]; !cluster.HasActiveTasks {
			if hasTaskForConfig && !repeatConfig {
				ctxlog.Debugf(ctx, m.L, "config is not repeatable, cluster %q has previous task, skipping", cluster.ID)
				continue
			}

			typ := actionCreate
			if hasTaskForConfig {
				if task.Status.IsRepeatable() {
					typ = actionRepeat
				}
			}

			maxDelay := m.now().Add(time.Duration(maxDelayDays) * 24 * time.Hour)
			minDelay := time.Hour * (time.Duration(24*minDays) + time.Duration(hoursDelay))
			planTS, err := models.PlanMaintenanceTime(ctx, cluster.Settings, minDelay, m.now(), maxDelay, m.Cal)
			if err != nil {
				return nil, xerrors.Errorf("plan maintenance time for cid %s: %w", cluster.ID, err)
			}
			actions = append(actions, MaintainAction{
				cluster:  cluster,
				createTS: m.now().UTC(),
				planTS:   planTS,
				typ:      typ,
			})
		} else {
			haveActiveTasks = append(haveActiveTasks, cluster.ID)
		}
	}

	if len(haveActiveTasks) <= m.Config.MaxElementsLog {
		ctxlog.Debugf(ctx, m.L, "clusters %v have active maintenance task, skip", haveActiveTasks)
	} else {
		ctxlog.Debugf(ctx, m.L, "%d clusters have active maintenance task, skip", len(haveActiveTasks))
	}

	return actions, nil
}

func (m *Maintainer) toRestore(ctx context.Context, clusters []models.Cluster, existedTasksByClusterID map[string]models.MaintenanceTask, minDays, hoursDelay int) ([]MaintainAction, error) {
	actions := make([]MaintainAction, 0)
	for _, c := range clusters {
		if t, ok := existedTasksByClusterID[c.ID]; ok && t.Status.IsRescheduable() {
			action := MaintainAction{
				cluster: c,
				planTS:  t.PlanTS.Time,
			}
			// We have to reschedule task if maintenance window settings are changed
			windowChanged := c.Settings.Valid && !models.IsTimeInMaintenanceWindow(c.Settings.Weekday, c.Settings.UpperBoundMWHour, t.PlanTS.Time) && t.Status == models.MaintenanceTaskCanceled

			switch {
			case t.Status == models.MaintenanceTaskRejected || windowChanged:
				minDelay := time.Hour * (time.Duration(24*minDays) + time.Duration(hoursDelay))
				ts, err := models.PlanMaintenanceTime(ctx, c.Settings, minDelay, m.now(), t.MaxDelay, m.Cal)
				if err != nil {
					return nil, xerrors.Errorf("plan maintenance time for cid %s: %w", c.ID, err)
				}
				action.planTS = ts
				action.createTS = m.now().UTC() // reset time for failed tasks
				action.typ = actionRetry
			case t.Status == models.MaintenanceTaskCanceled:
				if !t.PlanTS.Valid {
					return nil, xerrors.Errorf("plan time of maintenance task %q for cluster %q inconsistently empty", t.ConfigID, c.ID)
				}
				action.createTS = t.CreateTS // leave original value for cancelled tasks
				action.typ = actionRestore
			default:
				ctxlog.Errorf(ctx, m.L, "unexpected rescheduable %q task for cluster %q", t.TaskID.String, t.ClusterID)
				continue
			}
			actions = append(actions, action)
		}
	}
	return actions, nil
}

func (m *Maintainer) toCancel(clusters []models.Cluster, existedTasksByClusterID map[string]models.MaintenanceTask, configDisabled bool) []models.MaintenanceTask {
	clusterByClusterID := make(map[string]models.Cluster, len(clusters))
	for _, cluster := range clusters {
		clusterByClusterID[cluster.ID] = cluster
	}

	cancel := make([]models.MaintenanceTask, 0)
	// pick existing tasks with clusters not eligible for current maintenance
	for cid, task := range existedTasksByClusterID {
		if _, goodForMaintenance := clusterByClusterID[cid]; (!goodForMaintenance || configDisabled) && task.Status.IsCancellable() {
			cancel = append(cancel, task)
		}
	}
	return cancel
}

func (m *Maintainer) filterByEnv(ctx context.Context, clusters []models.Cluster) []models.Cluster {
	clustersByCloud := make(map[string][]models.Cluster)
	for _, cluster := range clusters {
		clustersByCloud[cluster.CloudExtID] = append(clustersByCloud[cluster.CloudExtID], cluster)
	}
	filteredByEnv := make([]models.Cluster, 0)
	for cloudID, cloudClusters := range clustersByCloud {
		sort.Sort(models.ClustersByEnv(cloudClusters))

		cloudEnv := cloudClusters[0].Env // start with the lowest-end env
		ctxlog.Debugf(ctx, m.L, "using lowest env %v for cloud %q", cloudEnv, cloudID)
		for _, cloudCluster := range cloudClusters {
			// use only one env
			if cloudCluster.Env == cloudEnv {
				filteredByEnv = append(filteredByEnv, cloudCluster)
			} else {
				break
			}
		}
	}
	return filteredByEnv
}

func (m *Maintainer) limitToCreate(ctx context.Context, toCreate []MaintainAction) map[string][]MaintainAction {
	actionsByCloud := make(map[string][]MaintainAction)

	// so clouds will be processed deterministically
	sort.Slice(toCreate, func(i, j int) bool {
		return toCreate[i].cluster.CloudExtID < toCreate[j].cluster.CloudExtID
	})

	newlyPlannedLimit := m.Config.NewlyPlannedLimit
	newlyPlannedLimitReached := false
	for _, action := range toCreate {
		if newlyPlannedLimit <= 0 {
			newlyPlannedLimitReached = true
		}
		_, cloudExists := actionsByCloud[action.cluster.CloudExtID]
		// we want all clusters for cloud to be processed at the same time
		if !cloudExists && newlyPlannedLimitReached {
			ctxlog.Debugf(ctx, m.L, "reached newly_planned_limit")
			break
		}
		if len(actionsByCloud) == m.Config.NewlyPlannedClouds && !cloudExists {
			ctxlog.Debugf(ctx, m.L, "reached newly_planned_clouds")
			break
		}
		actionsByCloud[action.cluster.CloudExtID] = append(actionsByCloud[action.cluster.CloudExtID], action)
		newlyPlannedLimit--
	}

	return actionsByCloud
}

func (m *Maintainer) notifyCloud(ctx context.Context, cloudID string, actions []MaintainAction, maintenanceInfo string) error {
	var notifyActions []MaintainAction
	for _, action := range actions {
		if action.typ.NeedNotification() {
			notifyActions = append(notifyActions, action)
		}
	}
	if len(notifyActions) == 0 {
		return nil
	}

	var payload map[string]interface{}
	var template notifier.TemplateID
	if len(notifyActions) == 1 {
		payload = notifyActionPayload(notifyActions[0], maintenanceInfo)
		template = notifier.MaintenanceScheduleTemplate
		ctxlog.Debugf(ctx, m.L, "Single cluster notification will be sent for cloud %q", cloudID)
	} else {
		payload = notifyActionsPayload(notifyActions, maintenanceInfo)
		template = notifier.MaintenancesScheduleTemplate
		ctxlog.Debugf(ctx, m.L, "Multiple clusters notification will be sent for cloud %q", cloudID)
	}

	if m.Config.NotifyUserDirectly {
		ctxlog.Infof(ctx, m.L, "send notification for cloud %q directly to users", cloudID)
		userIDs, err := m.collectUserFromCloud(ctx, cloudID)
		if err != nil {
			return xerrors.Errorf("collecting users for cloud %q: %w", cloudID, err)
		}
		for _, userID := range userIDs {
			if err := m.sender.NotifyUser(ctx, userID, template, payload, []notifier.TransportID{notifier.TransportMail}); err != nil {
				return xerrors.Errorf("send notification for cloud %q failed: %w", cloudID, err)
			}
		}
	} else {
		ctxlog.Infof(ctx, m.L, "Send notification for cloud %q", cloudID)
		if err := m.sender.NotifyCloud(ctx, cloudID, template, payload, []notifier.TransportID{notifier.TransportMail}); err != nil {
			return xerrors.Errorf("send notification for cloud %q failed: %w", cloudID, err)
		}
	}
	return nil
}

func notifyActionsPayload(actions []MaintainAction, info string) map[string]interface{} {
	var notifyClusters []map[string]interface{}
	for _, action := range actions {
		notifyClusters = append(notifyClusters, map[string]interface{}{
			"clusterId":    action.cluster.ID,
			"clusterType":  strings.TrimSuffix(action.cluster.ClusterType, "_cluster"),
			"clusterName":  action.cluster.ClusterName,
			"folderId":     action.cluster.FolderExtID,
			"delayedUntil": action.planTS.UTC().Format(DatetimeTemplate),
		})
	}
	payload := map[string]interface{}{"info": info, "resources": notifyClusters}
	return payload
}

func notifyActionPayload(action MaintainAction, info string) map[string]interface{} {
	return map[string]interface{}{
		"clusterId":    action.cluster.ID,
		"clusterType":  strings.TrimSuffix(action.cluster.ClusterType, "_cluster"),
		"clusterName":  action.cluster.ClusterName,
		"folderId":     action.cluster.FolderExtID,
		"delayedUntil": action.planTS.UTC().Format(DatetimeTemplate),
		"info":         info,
	}
}

func (m *Maintainer) planTask(ctx context.Context, cluster models.Cluster, taskConfig models.MaintenanceTaskConfig, planTS time.Time, createTS time.Time) error {
	if !cluster.Settings.Valid && !m.Config.AllowClustersWithoutMW {
		ctxlog.Infof(ctx, m.L, "skip cluster %q cause it maintenance settings not set and AllowClustersWithoutMW set to false", cluster.ID)
		return nil
	}

	if planTS.IsZero() {
		ctxlog.Infof(ctx, m.L, "skip cluster %q cause it's maintenance time %+v not comes yet", cluster.ID, cluster.Settings)
		return nil
	}

	taskIDGen, ok := m.taskIDGens[cluster.ClusterType]
	if !ok {
		ctxlog.Warnf(ctx, m.L,
			"skip cluster %q cause don't know task prefix for %q clusters. Fill task_id_prefixes config section",
			cluster.ID,
			cluster.ClusterType,
		)
		return nil
	}

	txCtx, err := m.MetaDB.Begin(ctx)
	if err != nil {
		return err
	}
	defer func() {
		if rollErr := m.MetaDB.Rollback(txCtx); rollErr != nil {
			ctxlog.Warn(ctx, m.L, "rollback failed", log.Error(rollErr))
		}
	}()

	if err := func(ctx context.Context) error {
		revs, err := m.MetaDB.LockFutureCluster(ctx, cluster.ID)
		if err != nil {
			return err
		}

		var taskArgs map[string]interface{}
		if taskConfig.Worker.TaskArgsQuery != "" {
			taskArgs, err = m.MetaDB.SelectTaskArgs(ctx, cluster.ID, taskConfig)
			if err != nil {
				return err
			}
		} else {
			taskArgs = taskConfig.Worker.TaskArgs
		}
		ctxlog.Debugf(ctx, m.L, "selected target host from cms: %s", cluster.TargetMaintenanceVtypeID)
		if cluster.TargetMaintenanceVtypeID != "" {
			taskArgs["target_maint_vtype_id"] = cluster.TargetMaintenanceVtypeID
		}

		taskArgsBytes, err := json.Marshal(taskArgs)
		if err != nil {
			return fmt.Errorf("marshal task args: %w", err)
		}

		err = m.MetaDB.ChangePillar(ctx, cluster.ID, taskConfig)
		if err != nil {
			return err
		}

		taskID, err := taskIDGen.Generate()
		if err != nil {
			return err
		}

		var timeout time.Duration
		if taskConfig.Worker.TimeoutQuery != "" {
			timeout, err = m.MetaDB.SelectTimeout(ctx, cluster.ID, taskConfig)
			if err != nil {
				return err
			}
		} else {
			timeout = taskConfig.Worker.Timeout.Duration
		}

		err = m.MetaDB.PlanMaintenanceTask(ctx,
			models.PlanMaintenanceTaskRequest{
				MaxDelay:      time.Now().Add(24 * time.Hour * time.Duration(taskConfig.MaxDays)),
				ID:            taskID,
				ClusterID:     cluster.ID,
				ConfigID:      taskConfig.ID,
				FolderID:      cluster.FolderID,
				OperationType: taskConfig.Worker.OperationType,
				TaskType:      taskConfig.Worker.TaskType,
				TaskArgs:      string(taskArgsBytes),
				Version:       TasksVersion,
				Metadata:      "{}",
				UserID:        m.Config.CreatedByUser,
				Rev:           revs.Rev,
				TargetRev:     revs.NextRev,
				PlanTS:        planTS,
				CreateTS:      createTS,
				Timeout:       timeout,
				Info:          taskConfig.Info,
			},
		)
		if err != nil {
			return err
		}

		err = m.MetaDB.CompleteFutureClusterChange(ctx, cluster.ID, revs.Rev, revs.NextRev)
		if err != nil {
			return err
		}

		return nil
	}(txCtx); err != nil {
		return err
	}
	return m.MetaDB.Commit(txCtx)
}

func (m *Maintainer) cancelMaintenanceTask(ctx context.Context, task models.MaintenanceTask) error {
	txCtx, err := m.MetaDB.Begin(ctx)
	if err != nil {
		return err
	}
	defer func() {
		if rollErr := m.MetaDB.Rollback(txCtx); rollErr != nil {
			ctxlog.Warn(ctx, m.L, "rollback failed", log.Error(rollErr))
		}
	}()

	if err := func(ctx context.Context) error {
		if task.Status == models.MaintenanceTaskPlanned {
			// cancel worker task in future revision
			rev, err := m.MetaDB.LockCluster(ctx, task.ClusterID)
			if err != nil {
				return err
			}

			err = m.MetaDB.CompleteClusterChange(ctx, task.ClusterID, rev)
			if err != nil {
				return err
			}
		}

		err = m.MetaDB.CompleteMaintenanceTask(ctx, task.ClusterID, task.ConfigID)
		if err != nil {
			return err
		}
		return nil
	}(txCtx); err != nil {
		return err
	}

	return m.MetaDB.Commit(txCtx)
}

func (m *Maintainer) collectUserFromCloud(ctx context.Context, cloudID string) ([]string, error) {
	for _, c := range m.Config.NotifyUserDirectlyDenylist {
		if c == cloudID {
			ctxlog.Infof(ctx, m.L, "The cloud %q is in deny list", cloudID)
			return nil, nil
		}
	}

	bindings, err := m.resClient.ListAccessBindings(ctx, cloudID, true)
	if err != nil {
		return nil, err
	}

	var userIDs []string
	for _, b := range bindings {
		if b.Subject.Type == "userAccount" && b.RoleID == "mdb.admin" {
			userIDs = append(userIDs, b.Subject.ID)
		}
	}
	return userIDs, err
}
