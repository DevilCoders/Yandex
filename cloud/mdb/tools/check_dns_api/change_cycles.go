package main

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	dnsconf "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/config"
	dnscore "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdChangeCycles = initChangeCycles()
)

func initChangeCycles() *cli.Command {
	cmd := &cobra.Command{
		Use:   "change cycles of test records",
		Short: "change cycles of test records",
		Long:  "change cycles of test records",
	}

	return &cli.Command{Cmd: cmd, Run: changeCycles}
}

const (
	recSuffix = "test.db.yandex.net"
	recNums   = 4

	recCheck    = time.Second
	recUpdate   = 8 * time.Second
	recShowStat = 30 * time.Second
	recChange   = 240 * time.Second
)

type eventStat struct {
	first time.Time
	last  time.Time
	count int
}

func (e *eventStat) upStat(now time.Time) {
	e.count++
	if e.first.IsZero() {
		e.first = now
	}
	e.last = now
}

type recStat struct {
	init     time.Time
	wellDone eventStat
	nxDomain eventStat
	oldRec   eventStat
	errors   eventStat
}

type base struct {
	ctx        context.Context
	logger     log.Logger
	dac        *dnscore.DAClient
	stats      []recStat
	fqdns      []string
	targetRecs []string
}

func newBase(ctx context.Context, logger log.Logger, dac *dnscore.DAClient) *base {
	fqdns := make([]string, recNums)
	for i := range fqdns {
		fqdns[i] = fmt.Sprintf("test-%0d.%s", i, recSuffix)
	}
	return &base{
		ctx:    ctx,
		logger: logger,
		dac:    dac,
		stats:  make([]recStat, recNums),
		fqdns:  fqdns,
		targetRecs: []string{
			"mdb-dns01i.db.yandex.net",
			"mdb-dns01f.db.yandex.net",
			"mdb-dns01h.db.yandex.net",
			"mdb-dns01k.db.yandex.net",
		},
	}
}

func (b *base) checkRecForUpdate(stat *recStat, fqdn, expectfqdn string) (bool, string) {
	curfqdn, ok := b.dac.DQ.LookupCNAME(b.ctx, fqdn, "")
	if !ok {
		stat.nxDomain.upStat(time.Now())
		return true, ""
	}
	if curfqdn != expectfqdn {
		stat.oldRec.upStat(time.Now())
		return true, curfqdn
	}
	stat.wellDone.upStat(time.Now())
	return false, ""
}

func (b *base) recCheckAndUpdate(now time.Time, stat *recStat, basefqdn, expectfqdn string) {
	needUpdate, curfqdn := b.checkRecForUpdate(stat, basefqdn, expectfqdn)
	if needUpdate {
		recs := make(map[string]dnsapi.Update, 1)
		recs[basefqdn] = dnsapi.Update{
			CNAMEOld: curfqdn,
			CNAMENew: expectfqdn,
		}
		_, err := b.dac.DA.UpdateRecords(b.ctx, &dnsapi.RequestUpdate{
			Records: recs,
		})
		if err != nil {
			b.logger.Warnf("update %s error: %v", basefqdn, err)
			stat.errors.upStat(time.Now())
		}
	}
}

func formatEvent(init time.Time, name string, stat *eventStat) string {
	firstDur := "-"
	if !stat.first.IsZero() {
		firstDur = stat.first.Sub(init).String()
	}
	lastDur := "-"
	if !stat.last.IsZero() {
		lastDur = stat.last.Sub(init).String()
	}
	return fmt.Sprintf("%s: %d [%s, %s]", name, stat.count, firstDur, lastDur)
}

func formatStat(stat *recStat) string {
	events := []string{
		formatEvent(stat.init, "well", &stat.wellDone),
		formatEvent(stat.init, "oldRec", &stat.oldRec),
		formatEvent(stat.init, "nxDomain", &stat.nxDomain),
		formatEvent(stat.init, "error", &stat.errors),
	}
	return strings.Join(events, "; ")
}

func (b *base) loopForRecordCheckAndUpdate(wg *sync.WaitGroup, i int) {
	defer wg.Done()
	tickCheck := time.NewTicker(recCheck)
	defer tickCheck.Stop()
	tickUpdate := time.NewTicker(recUpdate)
	defer tickUpdate.Stop()
	tickShowStat := time.NewTicker(recShowStat)
	defer tickShowStat.Stop()
	tickChange := time.NewTicker(recChange)
	defer tickChange.Stop()

	var targetRec int
	b.stats[i] = recStat{init: time.Now()}
	stat := &b.stats[i]
	basefqdn := b.fqdns[i]
	expectfqdn := b.targetRecs[int(targetRec)%len(b.targetRecs)]

	for {
		select {
		case <-tickCheck.C:
			b.checkRecForUpdate(stat, basefqdn, expectfqdn)
		case <-tickUpdate.C:
			b.recCheckAndUpdate(time.Now(), stat, basefqdn, expectfqdn)
		case <-tickShowStat.C:
			b.logger.Warnf("stat %s info\n%v", basefqdn, formatStat(stat))
		case <-tickChange.C:
			targetRec++
			expectfqdn = b.targetRecs[int(targetRec)%len(b.targetRecs)]
			b.logger.Warnf("switch %s to %v and clear stats", basefqdn, expectfqdn)
			b.stats[i] = recStat{init: time.Now()}
			stat = &b.stats[i]
		case <-b.ctx.Done():
			return
		}
	}
}

func changeCycles(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	logger := env.Logger

	cfgMdbdns := dnsconf.DefaultConfig()
	cfgMdbdns.DNSAPI.Slayer.FQDNSuffix = recSuffix
	ctx, dac, _ := createCommon(ctx, logger, cfgMdbdns, env, false)

	base := newBase(ctx, logger, dac)

	logger.Infof("create %d loops for update records", len(base.fqdns))
	var wg sync.WaitGroup
	for i := range base.fqdns {
		wg.Add(1)
		go base.loopForRecordCheckAndUpdate(&wg, i)
	}
	logger.Infof("processing, wait for stat each %s", recShowStat)
	wg.Wait()

	logger.Info("shutdown, show last stat")
	for i, stat := range base.stats {
		base.logger.Warnf("stat %s info\n%v", base.fqdns[i], formatStat(&stat))
	}
}
