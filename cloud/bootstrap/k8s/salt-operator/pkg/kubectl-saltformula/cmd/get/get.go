package get

import (
	"context"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"

	"github.com/olekukonko/tablewriter"
	"github.com/spf13/cobra"
	"k8s.io/apimachinery/pkg/util/duration"

	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/info"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/options"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/pkg/kubectl-saltformula/viewcontroller"
)

const (
	getExample = `
	# Get a salt formula
	%[1]s get bootstrap`

	tableFormat  = "%-17s%v\n"
	nameFormat   = "%s%s\u00A0%s"
	statusFormat = "%s\u00A0%s"
)

const (
	IconSaltFormula = "ƒ"
	IconPod         = "□"
	IconJob         = "⊞"
)

type Options struct {
	Watch          bool
	NoColor        bool
	TimeoutSeconds int

	options.SaltFormulaOptions
}

// ANSI escape codes
const (
	escape    = "\x1b"
	noFormat  = 0
	Bold      = 1
	FgBlack   = 30
	FgRed     = 31
	FgGreen   = 32
	FgYellow  = 33
	FgBlue    = 34
	FgMagenta = 35
	FgCyan    = 36
	FgWhite   = 37
	FgDefault = 39
	FgHiBlue  = 94
)

var (
	colorMapping = map[string]int{
		// Colors for icons
		info.IconWaiting:     FgYellow,
		info.IconProgressing: FgHiBlue,
		info.IconWarning:     FgRed,
		info.IconUnknown:     FgYellow,
		info.IconOK:          FgGreen,
		info.IconBad:         FgRed,
	}
)

// NewCmdGet returns a new instance of an `get` command
func NewCmdGet(o *options.SaltFormulaOptions) *cobra.Command {
	getOptions := Options{
		SaltFormulaOptions: *o,
	}

	var cmd = &cobra.Command{
		Use:          "get SALT_FORMULA_NAME",
		Short:        "Get details about a salt formula",
		Long:         "Get details about and visual representation of a salt formula. It returns a bunch of metadata on a resource and a tree view of the child resources created by the parent.",
		Example:      o.Example(getExample),
		SilenceUsage: true,
		RunE: func(c *cobra.Command, args []string) error {
			if len(args) != 1 {
				return o.UsageErr(c)
			}
			name := args[0]
			client, err := o.Clientset()
			if err != nil {
				return err
			}
			ns, err := o.Namespace(c)
			if err != nil {
				return err
			}
			controller := viewcontroller.NewSaltFormulaViewController(ns, name, client)
			ctx, cancel := context.WithCancel(o.SetupSignalHandler())
			defer cancel()

			sfi, err := controller.GetSaltFormulaInfo(ctx)
			if err != nil {
				return err
			}
			getOptions.PrintSaltFormula(sfi)

			return nil
		},
	}
	cmd.Flags().BoolVar(&getOptions.NoColor, "no-color", false, "Do not colorize output")
	return cmd
}

func mustPrintf(w io.Writer, format string, a ...any) {
	_, err := fmt.Fprintf(w, format, a...)
	if err != nil {
		panic(fmt.Errorf("cannot write to output, err: %w", err))
	}
}

func printAligned(w io.Writer, a ...any) {
	mustPrintf(w, tableFormat, a...)
}

func (o *Options) PrintSaltFormula(sfInfo *info.SaltFormulaInfo) {
	printAligned(o.Out, "Name:", sfInfo.ObjectMeta.Name)
	printAligned(o.Out, "Namespace:", sfInfo.ObjectMeta.Namespace)
	printAligned(o.Out, "BaseRole:", sfInfo.BaseRole)
	printAligned(o.Out, "Role:", sfInfo.Role)
	printAligned(o.Out, "Desired:", sfInfo.Desired)
	printAligned(o.Out, "Current:", sfInfo.Current)
	printAligned(o.Out, "Ready:", sfInfo.Ready)
	printAligned(o.Out, "Epoch:", sfInfo.Epoch)

	mustPrintf(o.Out, "\n")
	o.PrintSaltFormulaTree(sfInfo)
}

func (o *Options) PrintSaltFormulaTree(sfInfo *info.SaltFormulaInfo) {
	w := tablewriter.NewWriter(o.Out)
	o.PrintHeader(w)
	w.Append([]string{fmt.Sprintf(statusFormat, IconSaltFormula, sfInfo.Name), "SaltFormula", "", info.Age(*sfInfo.ObjectMeta)})
	for i, job := range sfInfo.Jobs {
		isLast := i == len(sfInfo.Jobs)-1
		prefix, subpfx := getPrefixes(isLast, "")
		o.PrintJob(w, sfInfo, job, prefix, subpfx)
	}
	w.Render()
}

func (o *Options) PrintJob(w *tablewriter.Table, sfInfo *info.SaltFormulaInfo, jobInfo *info.JobInfo, prefix string, subpfx string) {
	var infoMsg string
	if jobInfo.StartTime != nil && jobInfo.CompletionTime != nil {
		startTime := *jobInfo.StartTime
		infoMsg = fmt.Sprintf("Duration: %s", duration.HumanDuration(jobInfo.CompletionTime.Sub(startTime.Time)))
	}
	w.Append([]string{
		fmt.Sprintf(nameFormat, prefix,
			IconJob,
			jobInfo.ObjectMeta.Name),
		"Job",
		fmt.Sprintf(statusFormat, o.colorize(jobInfo.Icon), jobInfo.Status),
		info.Age(*jobInfo.ObjectMeta),
		infoMsg,
	})
	for i, podInfo := range jobInfo.Pods {
		isLast := i == len(jobInfo.Pods)-1
		podPrefix, _ := getPrefixes(isLast, subpfx)
		w.Append([]string{fmt.Sprintf(nameFormat, podPrefix, IconPod, podInfo.ObjectMeta.Name), "Pod", fmt.Sprintf(statusFormat, o.colorize(podInfo.Icon), podInfo.Status), info.Age(*podInfo.ObjectMeta), ""})
	}
}

func (o *Options) PrintHeader(w *tablewriter.Table) {
	w.SetHeader([]string{"Name", "Kind", "Status", "Age", "Info"})
	w.SetBorders(tablewriter.Border{Left: false, Top: false, Right: false, Bottom: false})
	w.SetHeaderAlignment(tablewriter.ALIGN_LEFT)
	w.SetRowLine(false)
	w.SetCenterSeparator("")
	w.SetColumnSeparator("")
	w.SetRowSeparator("")
	w.SetAlignment(tablewriter.ALIGN_LEFT)
	w.SetHeaderLine(false)
}

// colorize adds ansi color codes to the string based on well known words
func (o *Options) colorize(s string) string {
	if o.NoColor {
		return s
	}
	color := colorMapping[s]
	return o.ansiFormat(s, color)
}

// ansiFormat wraps ANSI escape codes to a string to format the string to a desired color.
// NOTE: we still apply formatting even if there is no color formatting desired.
// The purpose of doing this is because when we apply ANSI color escape sequences to our
// output, this confuses the tabwriter library which miscalculates widths of columns and
// misaligns columns. By always applying a ANSI escape sequence (even when we don't want
// color, it provides more consistent string lengths so that tabwriter can calculate
// widths correctly.
func (o *Options) ansiFormat(s string, codes ...int) string {
	if o.NoColor || os.Getenv("TERM") == "dumb" || len(codes) == 0 {
		return s
	}
	codeStrs := make([]string, len(codes))
	for i, code := range codes {
		codeStrs[i] = strconv.Itoa(code)
	}
	sequence := strings.Join(codeStrs, ";")
	return fmt.Sprintf("%s[%sm%s%s[%dm", escape, sequence, s, escape, noFormat)
}

// returns an appropriate tree prefix characters depending on whether or not the element is the
// last element of a list
func getPrefixes(isLast bool, subPrefix string) (string, string) {
	var childPrefix, childSubpfx string
	if !isLast {
		childPrefix = subPrefix + "├──"
		childSubpfx = subPrefix + "│  "
	} else {
		childPrefix = subPrefix + "└──"
		childSubpfx = subPrefix + "   "
	}
	return childPrefix, childSubpfx
}
