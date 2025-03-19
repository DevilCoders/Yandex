package masters

import (
	"context"
	"crypto/rand"
	"fmt"
	"math"
	"math/big"
	"sort"

	statsapi "github.com/montanaflynn/stats"
	"github.com/olekukonko/tablewriter"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/paging"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	cmdBalance = initBalance()
	flagForce  bool
)

const (
	flagNameForce      = "force"
	significantVarCoef = 0.10
)

func initBalance() *cli.Command {
	cmd := &cobra.Command{
		Use:   "balance <group>",
		Short: "Balance salt minions by masters",
		Long:  "Balance salt minions by masters in the group.",
		Args:  cobra.ExactArgs(1),
	}

	cmd.Flags().BoolVar(
		&flagForce,
		flagNameForce,
		false,
		"Ignore all warnings and do balancing even if it's unuseful",
	)
	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: Balance}
}

type minionStat struct {
	minionCount     int64
	minionDebt      float64
	fromProbability float64
	toProbability   float64
	resultCount     int64
}
type minionsStat map[string]*minionStat
type masterBounds struct {
	fqdn  string
	lower float64
	upper float64
}

func hasMinionShipment(ctx context.Context, dapi deployapi.Client, fqdn string) (bool, error) {
	attrs := deployapi.SelectShipmentsAttrs{
		FQDN:   optional.NewString(fqdn),
		Status: optional.NewString(string(models.ShipmentStatusInProgress)),
	}
	shipments, _, err := dapi.GetShipments(ctx, attrs, paging.Paging())
	if err != nil {
		return true, xerrors.Errorf("check minion %q is busy: %w", fqdn, err)
	}
	if len(shipments) > 0 {
		return true, nil
	}
	return false, nil
}

func isMinionMovable(ctx context.Context, dapi deployapi.Client, minion models.Minion, logger log.Logger) (bool, error) {
	if !isMinionSignificant(minion) {
		return false, nil
	}
	if !minion.AutoReassign {
		return false, nil
	}
	hasShipment, err := hasMinionShipment(ctx, dapi, minion.FQDN)
	if err != nil {
		return false, err
	}
	if hasShipment {
		logger.Tracef("skip minion %s, because it have inprogress shipment", minion.FQDN)
		return false, nil
	}
	return true, nil
}

func isMinionSignificant(minion models.Minion) bool {
	return !minion.Deleted
}

type balancer struct {
	dapi   deployapi.Client
	logger log.Logger
	rnd    randFloat
}

func newBalancer(dapi deployapi.Client, logger log.Logger, rnd randFloat) balancer {
	return balancer{dapi: dapi, logger: logger, rnd: rnd}
}

func (b balancer) Balance(ctx context.Context, group string, dryRun, force bool) (minionsStat, error) {
	stats, err := b.stat(ctx, group)
	if err != nil {
		return stats, xerrors.Errorf("calculate stat: %w", err)
	}
	stats, varcoef, err := b.probabilities(stats)
	if err != nil {
		return stats, xerrors.Errorf("calculate probabilities: %w", err)
	}
	if varcoef < significantVarCoef {
		if flagForce {
			b.logger.Warn("balancing is not so useful, but the force be with you")
		} else {
			return stats, xerrors.New("balancing is not so useful, please use --force, Luke")
		}
	} else {
		b.logger.Infof("do balancing, dry run = %v", dryRun)
	}

	stats, err = b.move(ctx, stats, dryRun)
	if err != nil {
		return stats, err
	}
	return stats, nil
}

func (b balancer) bounds(stats minionsStat) []masterBounds {
	// Calc bounds
	var toMasterBounds []masterBounds
	var cumsum float64

	for master, s := range stats {
		if s.toProbability > 0 {
			toMasterBounds = append(toMasterBounds, masterBounds{
				fqdn: master,
			})
		}
	}
	sort.Slice(toMasterBounds, func(i, j int) bool {
		return toMasterBounds[i].fqdn < toMasterBounds[j].fqdn
	})
	for i, b := range toMasterBounds {
		upper := cumsum + stats[b.fqdn].toProbability
		toMasterBounds[i].upper = upper
		toMasterBounds[i].lower = cumsum
		cumsum = upper
	}
	return toMasterBounds
}

func (b balancer) move(ctx context.Context, stats minionsStat, dryRun bool) (minionsStat, error) {
	// Balancing
	minions, page, err := b.dapi.GetMinions(ctx, paging.Paging())
	if err != nil {
		return stats, xerrors.Errorf("minions listing: %w", err)
	}
	if page.Size == int64(len(minions)) {
		return stats, xerrors.New("incomplete minions listing, please increase page size")
	}

	targetBounds := b.bounds(stats)
	b.logger.Infof("target bounds: %v", targetBounds)
	for _, minion := range minions {
		isMovable, err := isMinionMovable(ctx, b.dapi, minion, b.logger)
		if err != nil {
			return stats, xerrors.Errorf("minion movable check: %w", err)
		}
		if !isMovable {
			continue
		}
		st, ok := stats[minion.MasterFQDN]
		if !ok {
			continue
		}

		if st.fromProbability == 0 || b.rnd.Float64() >= st.fromProbability {
			stats[minion.MasterFQDN].resultCount++
			continue
		}
		toPoint := b.rnd.Float64()
		toFqdn := ""
		for _, bound := range targetBounds {
			if bound.lower <= toPoint && toPoint < bound.upper {
				toFqdn = bound.fqdn
				stats[toFqdn].resultCount++
				if !dryRun {
					_, err := b.dapi.UpsertMinion(ctx, minion.FQDN, deployapi.UpsertMinionAttrs{Master: optional.NewString(toFqdn)})
					if err != nil {
						return stats, xerrors.Errorf("upsert minion %s by moving from master %s to master %s: %w", minion.FQDN, minion.MasterFQDN, toFqdn, err)
					}
				}
				break
			}
		}
		if toFqdn == "" {
			return stats, xerrors.Errorf("the point %f is out of the bounds %v", toPoint, targetBounds)
		}
	}
	return stats, nil
}

func (b balancer) stat(ctx context.Context, group string) (minionsStat, error) {
	stats := make(minionsStat)
	b.logger.Infof("list masters from group %q", group)
	allMasters, page, err := b.dapi.GetMasters(ctx, paging.Paging())
	if err != nil {
		return nil, xerrors.Errorf("master listing: %w", err)
	}
	if page.Size == int64(len(allMasters)) {
		return nil, xerrors.New("incomplete masters listing, please increase page size")
	}

	//filter by group
	groupMasters := make([]models.Master, 0)
	for _, master := range allMasters {
		if master.Group != group {
			b.logger.Debugf("skip master %q from group: %q", master.FQDN, master.Group)
			continue
		}
		groupMasters = append(groupMasters, master)
	}

	b.logger.Infof("list minions by %d masters", len(groupMasters))
	for _, master := range groupMasters {
		if !master.IsOpen {
			return nil, xerrors.Errorf("master %q is closed, please open the master", master.FQDN)
		}
		minions, page, err := b.dapi.GetMinionsByMaster(ctx, master.FQDN, paging.Paging())
		if err != nil {
			return nil, xerrors.Errorf("minions listing: %w", err)
		}
		if page.Size == int64(len(minions)) {
			return nil, xerrors.New("incomplete minions listing, please increase page size")
		}

		var count int64
		for _, minion := range minions {
			if !isMinionSignificant(minion) {
				continue
			}
			count += 1
		}
		stats[master.FQDN] = &minionStat{minionCount: count}
	}
	return stats, nil
}

// TODO skip closed and not alive masters
func (b balancer) probabilities(stats minionsStat) (minionsStat, float64, error) {
	var masterCounts []float64
	for _, s := range stats {
		masterCounts = append(masterCounts, float64(s.minionCount))
	}

	mean, err := statsapi.Mean(masterCounts)
	if err != nil {
		return nil, 0.0, xerrors.Errorf("mean calc: %w", err)
	}
	b.logger.Infof("mean: %.4f", mean)

	sigma, err := statsapi.StdDevP(masterCounts)
	if err != nil {
		return nil, 0.0, xerrors.Errorf("sigma calc: %w", err)
	}
	b.logger.Infof("sigma: %.4f", sigma)

	varcoef := sigma / mean
	b.logger.Infof("varoef coef: %.4f", varcoef)

	var creditTotal float64
	for _, st := range stats {
		st.minionDebt = float64(st.minionCount) - mean
		if st.minionDebt > 0 {
			st.fromProbability = st.minionDebt / float64(st.minionCount)
		} else {
			creditTotal += st.minionDebt
		}
	}

	for _, st := range stats {
		if st.minionDebt < 0 {
			st.toProbability = st.minionDebt / creditTotal
		}
	}

	return stats, varcoef, nil
}

type randFloat interface {
	Float64() float64
}

func NewCryptoRandom(logger log.Logger) *cryptoRand {
	return &cryptoRand{logger: logger}
}

type cryptoRand struct {
	logger log.Logger
}

func (r cryptoRand) Float64() float64 {
	src, err := rand.Int(rand.Reader, big.NewInt(math.MaxInt64))
	if err != nil {
		r.logger.Fatalf("crypto random generation: %s", err)
	}

	return float64(src.Int64()) / (1 << 63)
}

// Balance salt master
func Balance(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	b := newBalancer(dapi, env.L(), NewCryptoRandom(env.L()))
	stats, err := b.Balance(ctx, args[0], env.IsDryRunMode(), flagForce)
	if err != nil {
		env.L().Fatalf("balancing: %s", err)
	}

	table := tablewriter.NewWriter(cmd.OutOrStdout())
	table.SetHeader([]string{"Master", "Count", "Debt", "P(From)", "P(To)", "Result"})
	for master, s := range stats {
		data := []string{
			master,
			fmt.Sprintf("%d", s.minionCount),
			fmt.Sprintf("%.3f", s.minionDebt),
			fmt.Sprintf("%.3f", s.fromProbability),
			fmt.Sprintf("%.3f", s.toProbability),
			fmt.Sprintf("%d", s.resultCount),
		}
		table.Rich(data, nil)
	}
	table.SetRowLine(true)
	table.Render()
}
