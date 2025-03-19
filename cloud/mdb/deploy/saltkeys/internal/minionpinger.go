package internal

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/library/go/core/log"
)

type MinionPingerConfig struct {
	PingPeriod       time.Duration `json:"ping_period" yaml:"ping_period"`
	PingTimeout      time.Duration `json:"ping_timeout" yaml:"ping_timeout"`
	MaxParallelPings int           `json:"max_parallel_pings" yaml:"max_parallel_pings"`
}

func DefaultMinionPingerConfig() MinionPingerConfig {
	return MinionPingerConfig{
		PingPeriod:       time.Minute,
		PingTimeout:      time.Second * 5,
		MaxParallelPings: 5,
	}
}

type MinionPinger struct {
	ctx context.Context
	l   log.Logger

	cfg    MinionPingerConfig
	client saltapi.Client
	auth   saltapi.Auth
	skeys  saltkeys.Keys

	ch           chan string
	runningPings *sync.Map
}

func NewMinionPinger(ctx context.Context, cfg MinionPingerConfig, client saltapi.Client, auth saltapi.Auth, skeys saltkeys.Keys, l log.Logger) *MinionPinger {
	mp := newMinionPinger(ctx, cfg, client, auth, skeys, l)

	// Setup pool of actual pingers
	for i := 0; i < mp.cfg.MaxParallelPings; i++ {
		go func() {
			mp.l.Debug("Started minions pinger")
			defer mp.l.Debug("Stopped minions pinger")

			for fqdn := range mp.ch {
				mp.pingMinion(fqdn)
			}
		}()
	}

	go mp.pingInitiator()
	return mp
}

func newMinionPinger(ctx context.Context, cfg MinionPingerConfig, client saltapi.Client, auth saltapi.Auth, skeys saltkeys.Keys, l log.Logger) *MinionPinger {
	return &MinionPinger{
		ctx:          ctx,
		l:            l,
		cfg:          cfg,
		client:       client,
		auth:         auth,
		skeys:        skeys,
		ch:           make(chan string),
		runningPings: &sync.Map{},
	}
}

func (mp *MinionPinger) pingInitiator() {
	mp.l.Debug("Started minions ping initiator")
	defer mp.l.Debug("Stopped minions ping initiator")

	ticker := time.NewTicker(mp.cfg.PingPeriod)
	for {
		select {
		case <-mp.ctx.Done():
			close(mp.ch)
			return
		case <-ticker.C:
			mp.doPings()
		}
	}
}

func (mp *MinionPinger) doPings() {
	mp.l.Debug("Pinging minions...")
	defer mp.l.Debug("Finished pinging minions")

	accepted, err := mp.skeys.List(saltkeys.Accepted)
	if err != nil {
		mp.l.Errorf("Failed to retrieve known minions: %s", err)
		return
	}

	for _, fqdn := range accepted {
		if _, loaded := mp.runningPings.LoadOrStore(fqdn, struct{}{}); !loaded {
			mp.ch <- fqdn
		}
	}
}

const (
	minionPingFunc = "dbaas.ping_minion"
)

func (mp *MinionPinger) pingMinion(fqdn string) {
	mp.l.Debugf("Executing ping for minion %q", fqdn)

	if err := mp.client.Run(
		mp.ctx,
		mp.auth,
		mp.cfg.PingTimeout,
		fqdn,
		minionPingFunc,
	); err != nil {
		mp.l.Debugf("Failed to ping minion %q: %s", fqdn, err)
	}

	mp.runningPings.Delete(fqdn)
}
