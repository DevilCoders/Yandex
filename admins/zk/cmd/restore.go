package main

import (
	"encoding/json"
	"io"
	"os"
	"path"
	"regexp"
	"sync"

	"cuelang.org/go/pkg/strings"
	gozk "github.com/go-zookeeper/zk"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/admins/zk"
)

func cmdRestore(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "restore prefix [-f filterRegexp] [-i input.file] [-p N]",
		Short: "restore zk subtree",
		Long: `
Restore zk subtree from stream of json docs, created by dump command

Example:
	# restore only node /subtree
	$ zk restore / -f '^/subtree$' -i dump.json

	# restore dump under specified /prefix node
	# following command restore /subtreeA into /subtreeB/subtreeA
	$ zk dump /subtreeA | zk restore /subtreeB
`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			prefix := fixPath(args[0])

			var filterRegexp *regexp.Regexp
			filter, err := cmd.Flags().GetString("filter")
			must(err, "flag filter")
			if filter != "" {
				filterRegexp = regexp.MustCompile(filter)
			}

			var input io.Reader
			input = os.Stdin
			flagInput, err := cmd.Flags().GetString("input")
			must(err)
			if flagInput != "" {
				input, err = os.Open(flagInput)
				must(err)
			}
			parallel, err := cmd.Flags().GetUint8("parallel")
			must(err, "flag parallel")
			if parallel < 1 {
				parallel = 1 // safety check
			}

			logger.Debugf("Run restore in %d threads", parallel)
			tokens := make(chan struct{}, parallel)

			flags := int32(0)
			acl := gozk.WorldACL(gozk.PermAll)
			decoder := json.NewDecoder(input)
			var wg sync.WaitGroup
			for {
				var node zkNode
				err := decoder.Decode(&node)
				if err == io.EOF {
					break
				}
				must(err, "decoder.Decode")
				if strings.HasPrefix(node.Path+"/", "/zookeeper/") {
					logger.Debugf("Skip internal zk node: %s", node.Path)
					continue
				}
				if filterRegexp != nil && !filterRegexp.MatchString(node.Path) {
					logger.Debugf("Node %s not match regexp filter: %q, skip", node.Path, flagFilter)
					continue
				}
				targetPath := path.Join(prefix, node.Path)
				targetData := node.Data
				logger.Debugf("Restore node: %v=%q", targetPath, targetData)

				dir := path.Dir(targetPath)
				err = client.EnsureZkPathCached(ctx, dir)
				must(err, "EnsureZkPathCached", dir)
				tokens <- struct{}{}
				wg.Add(1)
				go func() {
					defer func() {
						wg.Done()
						<-tokens
					}()
					_, err = client.Create(ctx, targetPath, targetData, flags, acl)
					if err == zk.ErrNodeExists {
						err = nil
						if targetData != nil {
							_, err = client.Set(ctx, targetPath, targetData, -1)
							must(err, "Set", targetPath)
						}
					}
					must(err, "Create", targetPath)
				}()
			}
			wg.Wait()
		},
	}
	_ = cmd.Flags().StringP("filter", "f", "", "filter nodes during a restore (include regexp filter)")
	_ = cmd.Flags().StringP("input", "i", "", "restore from file (default stdin)")
	_ = cmd.Flags().Uint8P("parallel", "p", 1, "run N threads in parallel")
	return cmd
}
