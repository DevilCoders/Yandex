package placementgroup

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	placement_group_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *placement_group_config.Config,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	storage resources.Storage,
	nbsFactory nbs.Factory,
) error {

	err := taskRegistry.Register(ctx, "placement_group.CreatePlacementGroup", func() tasks.Task {
		return &createPlacementGroupTask{
			storage:    storage,
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "placement_group.DeletePlacementGroup", func() tasks.Task {
		return &deletePlacementGroupTask{
			storage:    storage,
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "placement_group.AlterPlacementGroupMembership", func() tasks.Task {
		return &alterPlacementGroupMembershipTask{
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	deletedPlacementGroupExpirationTimeout, err :=
		time.ParseDuration(config.GetDeletedPlacementGroupExpirationTimeout())
	if err != nil {
		return err
	}

	clearDeletedPlacementGroupsTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedPlacementGroupsTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	const gcTaskType = "placement_groups.ClearDeletedPlacementGroups"

	err = taskRegistry.Register(
		ctx,
		gcTaskType,
		func() tasks.Task {
			return &clearDeletedPlacementGroupsTask{
				storage:           storage,
				expirationTimeout: deletedPlacementGroupExpirationTimeout,
				limit:             int(config.GetClearDeletedPlacementGroupsLimit()),
			}
		},
	)
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		gcTaskType,
		"",
		clearDeletedPlacementGroupsTaskScheduleInterval,
		1,
	)

	return nil
}
