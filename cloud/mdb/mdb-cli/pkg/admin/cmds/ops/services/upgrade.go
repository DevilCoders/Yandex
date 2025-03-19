package services

import (
	"bytes"
	"context"
	"embed"
	"encoding/json"
	"fmt"
	"regexp"
	"strings"
	"text/template"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/internal/tracker"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	trackerintergration "a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/tracker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	flagNameTicket = "ticket"
)

var (
	cmdUpgrade    = initUpgrade()
	releaseTicket string
	//go:embed templates/*
	templatesFS embed.FS
)

func initUpgrade() *cli.Command {
	cmd := &cobra.Command{
		Use:               "upgrade <service name>",
		Short:             "Upgrade content of service",
		Long:              "Upgrade content of service, run 'hs' with wrappers specific for service type",
		Args:              cobra.ExactArgs(1),
		ValidArgsFunction: compServices,
	}
	cmd.Flags().DurationVar(
		&flagTimeout,
		flagNameTimeout,
		20*time.Minute,
		"Timeout for high state command and entire shipment",
	)
	cmd.Flags().StringVarP(
		&releaseTicket,
		flagNameTicket,
		"t",
		"",
		"Release ticket (MDB-XXX)",
	)
	_ = cmd.MarkFlagRequired(flagNameTicket)
	return &cli.Command{Cmd: cmd, Run: Upgrade}
}

type serviceUpgradeContext struct {
	Name string
	Env  string
	UI   string
}

type startUpgradeContext struct {
	Shipment models.Shipment
	Service  serviceUpgradeContext
}

func formatTemplateAsComment(templateName string, templeContext interface{}) (tracker.Comment, error) {
	tBody, err := templatesFS.ReadFile(fmt.Sprintf("templates/%s.gotxt", templateName))
	if err != nil {
		return tracker.Comment{}, xerrors.Errorf("read template: %w", err)
	}
	t, err := template.New(templateName).Parse(string(tBody))
	if err != nil {
		return tracker.Comment{}, xerrors.Errorf("parse template: %w", err)
	}
	var text strings.Builder
	if err := t.Execute(&text, templeContext); err != nil {
		return tracker.Comment{}, fmt.Errorf("unable to format template: %w", err)
	}
	return tracker.Comment{Text: text.String()}, nil
}

func formStartUpgradeComment(commentContext startUpgradeContext) (tracker.Comment, error) {
	return formatTemplateAsComment("start_upgrade", commentContext)
}

type finishUpdateContext struct {
	Service  serviceUpgradeContext
	Shipment models.Shipment
	Changes  map[string]map[string]string
}

func formFinishUpgradeComment(commentContext finishUpdateContext) (tracker.Comment, error) {
	return formatTemplateAsComment("finish_upgrade", commentContext)
}

func prettyChange(change json.RawMessage, comment string) string {
	newStrictDecoder := func() *json.Decoder {
		dec := json.NewDecoder(bytes.NewReader(change))
		dec.DisallowUnknownFields()
		return dec
	}
	//  {"mdb-katan": {"new": "1.8100164", "old": "1.8067935"}}
	var pkgChange map[string]struct {
		New string `json:"new"`
		Old string `json:"old"`
	}
	if err := newStrictDecoder().Decode(&pkgChange); err == nil && len(pkgChange) > 0 {
		var pretty []string
		for name, versions := range pkgChange {
			var p string
			switch {
			case versions.Old != "" && versions.New != "":
				p = fmt.Sprintf("%s updated %s -> %s", name, versions.Old, versions.New)
			case versions.New != "":
				p = fmt.Sprintf("%s=%s installed", name, versions.New)
			case versions.Old != "":
				p = fmt.Sprintf("%s=%s uninstalled", name, versions.Old)
			}
			pretty = append(pretty, p)
		}
		return strings.Join(pretty, " ")
	}
	// Supervisor services restarts
	// Comment = Restarting service: mdb-katan
	// {"mdb-katan": "Restarting service: mdb-katan"}
	if strings.HasPrefix(comment, "Restarting service:") {
		return comment
	}
	// systemd service restart
	// Comment = Service restarted
	// Changes = {"mdb-ping-salt-master": true}
	if matched, _ := regexp.MatchString(`^Service \w+$`, comment); matched {
		// sadly, but that change doesn't have a service name in the comment,
		// extract it from changes
		var serviceMap map[string]interface{}
		if err := newStrictDecoder().Decode(&serviceMap); err == nil && len(serviceMap) > 0 {
			for serviceName := range serviceMap {
				return fmt.Sprintf("%s: %s", comment, serviceName)
			}
		}

	}
	return string(change)
}

func gatherUpgradeChanges(ctx context.Context, l log.Logger, dapi deployapi.Client, shipment models.Shipment, interestingSLS []string) map[string]map[string]string {
	byShipment := optional.NewString(shipment.ID.String())
	paging := deployapi.Paging{Size: 100}

	commandsAttr := deployapi.SelectCommandsAttrs{ShipmentID: byShipment}
	commands, _, err := dapi.GetCommands(ctx, commandsAttr, paging)
	if err != nil {
		l.Warnf("failed to load shipment commands: %s", err)
		return nil
	}
	commands2fqdn := make(map[models.CommandID]string, len(shipment.FQDNs))
	for _, c := range commands {
		commands2fqdn[c.ID] = c.FQDN
	}
	jobsAttrs := deployapi.SelectJobsAttrs{ShipmentID: byShipment}
	jobsAttrs.ShipmentID.Set(shipment.ID.String())
	jobs, _, err := dapi.GetJobs(ctx, jobsAttrs, paging)
	if err != nil {
		l.Warnf("failed to load shipment jobs: %s", err)
		return nil
	}

	changes := make(map[string]map[string]string, len(shipment.FQDNs))
	for _, job := range jobs {
		jrAttrs := deployapi.SelectJobResultsAttrs{
			ExtJobID: optional.NewString(job.ExtID),
			FQDN:     optional.NewString(commands2fqdn[job.CommandID]),
		}
		jr, _, err := dapi.GetJobResults(ctx, jrAttrs, deployapi.Paging{Size: 1})
		if err != nil {
			l.Warnf("failed to get job %q result: %s", job.ExtID, err)
			continue
		}
		if len(jr) == 0 {
			l.Warnf("got no job results for %s", job.ExtID)
			continue
		}
		rr, errs := saltapi.ParseReturnResult(jr[0].Result, time.Hour*3)
		if errs != nil {
			l.Warnf("parse job result result for hosts: %s", errs)
			continue
		}
		for _, s := range rr.ReturnStates {
			if string(s.Changes) == "{}" {
				continue
			}
			hostChanges := changes[rr.FQDN]
			if slices.ContainsString(interestingSLS, s.ID) {
				if hostChanges == nil {
					hostChanges = make(map[string]string, 1)
				}
				hostChanges[s.ID] = prettyChange(s.Changes, string(s.Comment))
			}
			changes[rr.FQDN] = hostChanges
		}
	}
	return changes
}

// Upgrade service
func Upgrade(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	environment, ok := topology[cfg.SelectedDeployment]
	if !ok {
		env.L().Fatalf("can't find topology for deployment %q", cfg.SelectedDeployment)
	}
	updateService := serviceName(args[0])
	service, ok := environment[updateService]
	if !ok {
		env.L().Fatalf("can't find service with name %q for deployment %q", args[0], cfg.SelectedDeployment)
	}

	st := trackerintergration.NewAPI(env)
	hosts := helpers.ParseMultiTargets(ctx, []string{service.Group}, env.L())
	dapi := helpers.NewDeployAPI(env)
	var cmds []models.CommandDef

	// TODO: Check we have recent (5min) hs test with non zero changes. Run hs only on the ones.
	// TODO: Support DB service type (find master and ship on this firstly)
	switch service.Type {
	case API:
		// Sleep before and after API close to get a time to load balancer to close.
		cmds = append(cmds, []models.CommandDef{
			{Type: "file.touch", Args: []string{service.CloseFilePath.Must()}, Timeout: encodingutil.Duration{Duration: 30 * time.Second}},
			{Type: "test.sleep", Args: []string{"20"}, Timeout: encodingutil.Duration{Duration: 40 * time.Second}},
			{Type: "saltutil.sync_all", Timeout: encodingutil.Duration{Duration: 5 * time.Minute}},
			{Type: "state.highstate", Args: []string{"queue=True"}, Timeout: encodingutil.Duration{Duration: flagTimeout}},
			{Type: "test.sleep", Args: []string{"15"}, Timeout: encodingutil.Duration{Duration: 30 * time.Second}},
			{Type: "file.remove", Args: []string{service.CloseFilePath.Must()}, Timeout: encodingutil.Duration{Duration: 30 * time.Second}},
			{Type: "test.sleep", Args: []string{"15"}, Timeout: encodingutil.Duration{Duration: 30 * time.Second}},
		}...)
	default:
		cmds = append(cmds, []models.CommandDef{
			{Type: "saltutil.sync_all", Timeout: encodingutil.Duration{Duration: 5 * time.Minute}},
			{Type: "state.highstate", Args: []string{"queue=True"}, Timeout: encodingutil.Duration{Duration: flagTimeout}},
		}...)
	}
	if service.PingURL.Valid {
		cmds = append(cmds, models.CommandDef{
			Type:    "http.wait_for_successful_query",
			Args:    []string{service.PingURL.Must(), "backend=requests", "wait_for=120", "request_interval=10", "verify_ssl=False"},
			Timeout: encodingutil.Duration{Duration: 3 * time.Minute},
		})
	}

	parallel := int64(1)
	if len(hosts) > 3 {
		parallel = 2
	}

	totalTimeout := 5 * time.Minute // Minimum timeout for shipment infra.
	for _, cmd := range cmds {
		totalTimeout += cmd.Timeout.Duration
	}

	shipment, err := dapi.CreateShipment(ctx,
		hosts,
		cmds,
		parallel,
		1,
		totalTimeout,
	)

	if err != nil {
		env.L().Fatalf("create upgrade shipment: %s", err)
	}

	serviceContext := serviceUpgradeContext{
		Env:  cfg.SelectedDeployment,
		Name: string(updateService),
		UI:   cfg.Deployments[cfg.SelectedDeployment].MDBUIURI,
	}
	var commentID string
	releaseComment, err := formStartUpgradeComment(
		startUpgradeContext{
			Service:  serviceContext,
			Shipment: shipment,
		})
	if err != nil {
		env.L().Warnf("failed to format release comment: %s", err)
	} else {
		commentID, err = st.CreateComment(ctx, releaseTicket, releaseComment)
		if err != nil {
			env.L().Warnf("failed to create comment in release ticket: %s", err)
		}
	}

	shipment, err = waitForShipment(ctx, shipment, dapi)
	if err != nil {
		env.L().Fatalf("wait for upgrade shipment: %s", err)
	}
	env.L().Infof("wait for upgrade shipment completed with status %q", shipment.Status)

	if commentID != "" {
		finishComment, err := formFinishUpgradeComment(finishUpdateContext{
			Service:  serviceContext,
			Shipment: shipment,
			Changes:  gatherUpgradeChanges(ctx, env.L(), dapi, shipment, service.SLSs),
		})
		if err != nil {
			env.L().Warnf("failed to form finish release comment: %s", err)
		} else {
			if err := st.UpdateComment(ctx, releaseTicket, commentID, finishComment); err != nil {
				env.L().Warnf("failed to update comment in release ticket: %s", err)
			}
		}
	}

	Get(ctx, env, cmd, args)
}
