package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"sort"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/library/go/yatool"
)

var (
	flagCheckID   string
	flagStat      bool
	flagMigration bool
	flagStatDepth int
)

type checkStatus struct {
	Status  string `json:"status"`
	Name    string `json:"name"`
	Snippet string `json:"snippet"`
	Path    string `json:"path"`
	Type    string `json:"type"`
}

type checkOutput struct {
	Status string `json:"status"`

	ImportantChanges []checkStatus `json:"important_changes"`
}

var yolintCmd = &cobra.Command{
	Use:          "yolint",
	Short:        "inspect yolint errors from a given PR",
	SilenceUsage: true,
	RunE: func(cmd *cobra.Command, args []string) error {
		ya, err := yatool.Ya()
		if err != nil {
			return err
		}

		var stderr bytes.Buffer
		checkCmd := exec.Command(ya, "check", "--attach", flagCheckID, "--json-output", "--snippet-limit=0")
		checkCmd.Stderr = &stderr
		checkJSON, err := checkCmd.Output()
		if err != nil {
			return err
		}

		if len(checkJSON) == 0 {
			_, _ = stderr.WriteTo(os.Stderr)
			return fmt.Errorf("yolint: ya check finished with error")
		}

		var status checkOutput
		if err := json.Unmarshal(checkJSON, &status); err != nil {
			return err
		}

		issues := map[string]map[string]struct{}{}
		for _, issue := range status.ImportantChanges {
			if !strings.HasSuffix(issue.Name, ".vet.txt") || issue.Type != "style" {
				continue
			}

			issueSet := issues[issue.Path]
			if issueSet == nil {
				issueSet = map[string]struct{}{}
				issues[issue.Path] = issueSet
			}

			for _, line := range strings.Split(issue.Snippet, "\n") {
				issueSet[line] = struct{}{}
			}
		}

		if flagMigration {
			emit := func(path string) {
				fmt.Printf("      - a.yandex-team.ru/%s\n", path)
			}

			for arcadiaPath := range issues {
				if strings.HasSuffix(arcadiaPath, "/gotest") {
					arcadiaPath = strings.TrimSuffix(arcadiaPath, "/gotest")
					emit(arcadiaPath)
					emit(arcadiaPath + "_test")
				} else {
					emit(arcadiaPath)
				}
			}
		} else if !flagStat {
			var problemModules []string
			for name := range issues {
				problemModules = append(problemModules, name)
			}
			sort.Strings(problemModules)

			for _, name := range problemModules {
				issueSet := issues[name]

				var issueList []string
				for issue := range issueSet {
					issueList = append(issueList, issue)
				}
				sort.Strings(issueList)

				fmt.Println(name)
				for _, issue := range issueList {
					fmt.Println(issue)
				}
			}
		} else {
			stats := map[string]int{}
			for name, issueSet := range issues {
				key := strings.Split(name, "/")
				if len(key) > flagStatDepth {
					key = key[:flagStatDepth]
				}

				stats[strings.Join(key, "/")] += len(issueSet)
			}

			var problemModules []string
			for name := range stats {
				problemModules = append(problemModules, name)
			}
			sort.Strings(problemModules)

			for _, name := range problemModules {
				fmt.Printf("%d %s\n", stats[name], name)
			}
		}

		return nil
	},
}

func init() {
	yolintCmd.Flags().StringVarP(&flagCheckID, "check-id", "", "", "check ID from CI")
	_ = yolintCmd.MarkFlagRequired("check-id")

	yolintCmd.Flags().BoolVarP(&flagStat, "stat", "", false, "compute aggregate statistics")
	yolintCmd.Flags().IntVarP(&flagStatDepth, "depth", "", 1, "depth for statistics aggregation")
	yolintCmd.Flags().BoolVarP(&flagMigration, "migration", "", false, "output error list in format complatible with build/rules/go/migrations.yml")
}
