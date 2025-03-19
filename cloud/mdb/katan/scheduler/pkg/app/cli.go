package app

import (
	"context"
	"fmt"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
)

func Cli(configPath, matchTags, commands, asUser string) int {
	type clusterState = katandb.ClusterRolloutState
	ctx := context.Background()
	app, err := newAppFromConfig(ctx, configPath)
	if err != nil {
		fmt.Println("application initialization fail with: ", err)
		return 1
	}

	rolloutID, err := app.AddRollout(ctx, matchTags, commands, asUser)
	if err != nil {
		fmt.Println("failed to add rollout: ", err)
		return 2
	}
	fmt.Printf("add rollout id: '%d'\n", rolloutID)

	for {
		rollState, err := app.RolloutState(ctx, rolloutID)
		if err != nil {
			fmt.Println("failed to get rollout state: ", err)
			return 1
		}

		stateCounts := make(map[clusterState]int)
		for _, cr := range rollState.ClustersRollouts {
			stateCounts[cr.State] = stateCounts[cr.State] + 1
		}
		statesOrder := make([]clusterState, 0, len(stateCounts))
		for s := range stateCounts {
			statesOrder = append(statesOrder, s)
		}

		sort.Slice(statesOrder, func(i, j int) bool {
			return int(statesOrder[i]) < int(statesOrder[j])
		})

		for _, s := range statesOrder {
			fmt.Printf("%10s %-5d\n", s, stateCounts[s])
		}

		if stateCounts[katandb.ClusterRolloutRunning] > 0 {
			fmt.Print("    running on:")
			for _, cr := range rollState.ClustersRollouts {
				if cr.State == katandb.ClusterRolloutRunning {
					fmt.Print(cr.ClusterID, " ")
				}
			}
			fmt.Println()
		}
		fmt.Println()

		if rollState.Rollout.FinishedAt.Valid {
			fmt.Println("rollout finished")
			if rollState.Rollout.Comment.Valid {
				fmt.Println(rollState.Rollout.Comment.String)
			}
			return 0
		}
		time.Sleep(time.Second)
	}
}
