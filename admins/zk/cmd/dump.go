package main

import (
	"encoding/json"
	"io"
	"os"
	"regexp"
	"sync"

	"a.yandex-team.ru/admins/zk"
	"github.com/spf13/cobra"
)

var flagFilter string
var flagOutput string
var filterRe *regexp.Regexp

type zkNode struct {
	Path string
	Data []byte
}

var flagParallel uint8

func cmdDump(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "dump path [-f filterRegexp] [-o output.file] [-p N]",
		Short: "dump zk subtree",
		Long: `
Dump zk subtree to json file or stdout

Example:
    $ zk dump /subtree
	{"Path":"/subtree","Data":""}
	{"Path":"/subtree/1","Data":"MQ=="}
	{"Path":"/subtree/2","Data":"ZGF0YTI="}
	{"Path":"/subtree/2/3","Data":""}`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := fixPath(args[0])
			if flagFilter != "" {
				filterRe = regexp.MustCompile(flagFilter)
			}

			var output io.WriteCloser
			var err error
			output = os.Stdout
			if flagOutput != "" {
				output, err = os.OpenFile(flagOutput, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
				must(err)
				defer func() {
					must(output.Close())
				}()
			}

			if flagParallel < 1 {
				flagParallel = 1 // safety check
			}
			dumpCh := make(chan *zkNode, flagParallel)
			go func() {
				encoder := json.NewEncoder(output)
				for node := range dumpCh {
					logger.Debugf("Dump node: %s", node.Path)
					must(encoder.Encode(node))
				}
			}()

			tokens := make(chan struct{}, flagParallel)
			logger.Debugf("Dump in %d parallel threads", flagParallel)
			var wg sync.WaitGroup

			dumpCallback := func(path string) bool {
				if filterRe != nil && !filterRe.MatchString(path) {
					logger.Debugf("Node %s not match regexp filter: %q, skip", path, flagFilter)
					return true
				}

				tokens <- struct{}{}
				wg.Add(1)
				go func() {
					defer func() {
						wg.Done()
						<-tokens
					}()
					data, stat, err := client.Get(ctx, path)
					if stat.EphemeralOwner != 0 {
						logger.Debugf("Skip ephemeral node %s", path)
						// ephemeral node can't contains children
						// so, we can safely return skip=false
						return
					}
					must(err)
					dumpCh <- &zkNode{path, data}
				}()
				return false
			}

			data, stat, err := client.Get(ctx, path)
			if stat.EphemeralOwner != 0 {
				logger.Fatalf("Can't dump ephemeral node: %s", path)
				return
			}
			must(err)

			dumpCh <- &zkNode{path, data}
			processChildrenRecursivelyPreorder(ctx, client, path, dumpCallback)
			wg.Wait()
			close(dumpCh)
		},
	}
	cmd.Flags().StringVarP(&flagFilter, "filter", "f", "", "filter nodes during a dump (include regexp filter)")
	cmd.Flags().StringVarP(&flagOutput, "output", "o", "", "dump output file (default stdout)")
	cmd.Flags().Uint8VarP(&flagParallel, "parallel", "p", 1, "run N threads in parallel")
	return cmd
}
