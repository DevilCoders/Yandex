package core

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/mem"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	dnsqmem "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq/mem"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var defaultConfig = DefaultDMConfig()

type cInfo struct {
	cid    string
	sid    string
	sname  string
	prim   string
	second string
	pkey   []byte
	cType  metadb.ClusterType
	env    metadb.Environment
}

func generateRandomClusterInfoArray(t *testing.T, ctx context.Context, count int, mdbmock *mocks.MockMetaDB) []cInfo {
	hil := make([]cInfo, 0, count)
	for i := 0; i < count; i++ {
		privKey, err := crypto.GeneratePrivateKeyForTest()
		require.NoError(t, err)
		require.NotNil(t, privKey)

		ci := cInfo{
			cid:    "cid-" + uuid.Must(uuid.NewV4()).String(),
			prim:   "prim-" + uuid.Must(uuid.NewV4()).String(),
			second: "second-" + uuid.Must(uuid.NewV4()).String(),
			pkey:   privKey.GetPublicKey().EncodeToPKCS1(),
			cType:  metadb.PostgresqlCluster,
			env:    metadb.DevEnvironment,
		}

		hil = append(hil, ci)
		mdbmock.EXPECT().IsReady(gomock.Any()).AnyTimes().Return(nil)
		mdbmock.EXPECT().ClusterInfo(gomock.Any(), ci.cid).Return(metadb.ClusterInfo{PubKey: ci.pkey, CType: ci.cType, Env: ci.env}, nil)
	}
	return hil
}

func generateRandWithShards(t *testing.T, ctx context.Context, count int, mdbmock *mocks.MockMetaDB) []cInfo {
	cil := generateRandomClusterInfoArray(t, ctx, count, mdbmock)
	// use one cid, first is cluster common
	cid := cil[0].cid
	for i := range cil {
		if i == 0 {
			continue
		}
		cil[i].cid = cid
		cil[i].sid = "sid-" + uuid.Must(uuid.NewV4()).String()
		cil[i].sname = "sname-" + uuid.Must(uuid.NewV4()).String()
		mdbmock.EXPECT().ShardByID(gomock.Any(), cil[i].sid).Return(metadb.ShardInfo{ShardName: cil[i].sname}, nil)
	}
	return cil
}

func preloadPubKeys(ctx context.Context, t *testing.T, dm *DNSManager, cis []cInfo) {
	for _, ci := range cis {
		if ci.sid != "" {
			continue
		}
		cachedKey, err := dm.GetPublicKey(ctx, ci.cid)
		require.NoError(t, err)
		require.Equal(t, ci.pkey, cachedKey)
	}
}

func updateDNS(ctx context.Context, dm *DNSManager, ts time.Time, ci cInfo) (bool, error) {
	return dm.UpdateDNS(ctx, ts, ci.cid, ci.sid, ci.prim, ci.second)
}

func updateRequestAll(ctx context.Context, t *testing.T, dm *DNSManager, cis []cInfo, ts time.Time, shouldUpd bool) {
	for _, ci := range cis {
		acc, err := updateDNS(ctx, dm, ts, ci)
		require.Equal(t, shouldUpd, acc)
		require.NoError(t, err)
	}
}

func updateRequestAllUpdateOnlyIndex(ctx context.Context, t *testing.T, dm *DNSManager, cis []cInfo, ts time.Time, upIndex int) {
	for i, ci := range cis {
		acc, err := updateDNS(ctx, dm, ts, ci)
		require.Equal(t, i == upIndex, acc)
		require.NoError(t, err)
	}
}

func checkNoPrimRecords(t *testing.T, damem *mem.Client, dac *DAClient, ci cInfo) {
	getPrim, upPrimCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
	require.Equal(t, 0, upPrimCount)
	require.Equal(t, "", getPrim)
}

func checkNoSecondRecords(t *testing.T, damem *mem.Client, dac *DAClient, ci cInfo) {
	getSecond, upSecondCount := damem.GetRecord(SecondaryFQDNByCid(dac, defaultConfig, ci.cid))
	require.Equal(t, 0, upSecondCount)
	require.Equal(t, "", getSecond)
}

func checkNoRecords(t *testing.T, damem *mem.Client, dac *DAClient, cis []cInfo) {
	for _, ci := range cis {
		checkNoPrimRecords(t, damem, dac, ci)
		checkNoSecondRecords(t, damem, dac, ci)
	}
}

func checkUpdateCountForRecPrimSecond(t *testing.T, damem *mem.Client, dac *DAClient, ci cInfo, expectUpdPrimCount, expectUpdSecondCount int) {
	var getPrim, getSecond string
	var upPrimCount, upSecondCount int
	if ci.sid == "" {
		getPrim, upPrimCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
		getSecond, upSecondCount = damem.GetRecord(SecondaryFQDNByCid(dac, defaultConfig, ci.cid))
	} else {
		getPrim, upPrimCount = damem.GetRecord(PrimaryShardFQDN(dac, defaultConfig, ci.cid, ci.sname))
		getSecond, upSecondCount = damem.GetRecord(SecondaryShardFQDN(dac, defaultConfig, ci.cid, ci.sname))
	}
	require.Equal(t, expectUpdPrimCount, upPrimCount)
	if expectUpdPrimCount > 0 {
		require.Equal(t, ci.prim, getPrim)
	} else {
		require.Equal(t, "", getPrim)
	}
	require.Equal(t, expectUpdSecondCount, upSecondCount)
	if expectUpdSecondCount > 0 {
		require.Equal(t, ci.second, getSecond)
	} else {
		require.Equal(t, "", getSecond)
	}
}

func checkUpdateCountForRec(t *testing.T, damem *mem.Client, dac *DAClient, ci cInfo, expectUpdCount int) {
	checkUpdateCountForRecPrimSecond(t, damem, dac, ci, expectUpdCount, expectUpdCount)
}

func checkUpdateCountPrimSecond(t *testing.T, damem *mem.Client, dac *DAClient, cis []cInfo, expectUpdPrimCount, expectUpdSecondCount int) {
	for _, ci := range cis {
		checkUpdateCountForRecPrimSecond(t, damem, dac, ci, expectUpdPrimCount, expectUpdSecondCount)
	}
}

func checkUpdateCountExcept(t *testing.T, damem *mem.Client, dac *DAClient, cis []cInfo, expectUpdCount, exceptInd int) {
	for i, ci := range cis {
		if i == exceptInd {
			continue
		}
		checkUpdateCountForRec(t, damem, dac, ci, expectUpdCount)
	}
}

func checkUpdateCountExceptList(t *testing.T, damem *mem.Client, dac *DAClient, cis []cInfo, expectUpdCount int, exceptInd map[int]bool) {
	for i, ci := range cis {
		_, ok := exceptInd[i]
		if ok {
			continue
		}
		checkUpdateCountForRec(t, damem, dac, ci, expectUpdCount)
	}
}

func checkUpdateCount(t *testing.T, damem *mem.Client, dac *DAClient, cis []cInfo, expectUpdCount int) {
	checkUpdateCountExcept(t, damem, dac, cis, expectUpdCount, -1)
}

func getLogger() log.Logger {
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	return logger
}

func initDMCustom(t *testing.T, logger log.Logger, maxRec int, dnsq dnsq.Client,
) (context.Context, *mocks.MockMetaDB, *mem.Client, *DNSManager, *DAClient) {
	ctx := context.Background()
	dam := mem.New()
	dac := &DAClient{
		Client: DNSSlayer,
		Suffix: "db.test.net",
		DQ:     dnsq,
		DA:     dam,
		MaxRec: uint(maxRec),
		UpdThr: 0, // to check unbuffered channel
	}
	var dal []*DAClient
	dal = append(dal, dac)
	dmc := DefaultDMConfig()
	ctrl := gomock.NewController(logger)
	mdb := mocks.NewMockMetaDB(ctrl)
	dm := NewDNSManager(ctx, logger, dmc, dal, mdb)
	require.NotNil(t, dm)
	return ctx, mdb, dam, dm, dac
}

func initDM(t *testing.T, maxRec int) (context.Context, *mocks.MockMetaDB, *mem.Client, *DNSManager, *DAClient) {
	logger := getLogger()
	dnsq := dnsqmem.New(logger)
	return initDMCustom(t, logger, maxRec, dnsq)
}

func prepAndUpdateBoth(ctx context.Context, dm *DNSManager, dac *DAClient, ts time.Time) {
	dm.prepAndUpdateCycle(ctx, ts, dac, updPrimary, metadb.PostgresqlCluster, metadb.DevEnvironment)
	dm.prepAndUpdateCycle(ctx, ts, dac, updSecond, metadb.PostgresqlCluster, metadb.DevEnvironment)
}

func TestSetOnce(t *testing.T) {
	maxRec := 1
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, 1, mdbmock)

	ts := time.Now()
	acc, err := updateDNS(ctx, dm, ts, cis[0])
	require.Error(t, err, "no precached cid, because no GetPublicKey call")
	require.False(t, acc)

	preloadPubKeys(ctx, t, dm, cis)

	acc, err = updateDNS(ctx, dm, ts, cis[0])
	require.NoError(t, err)
	require.True(t, acc)

	acc, err = updateDNS(ctx, dm, ts, cis[0])
	require.NoError(t, err, "same timestamp should be ok")
	require.False(t, acc, "record not accepted, because it is same")

	acc, err = updateDNS(ctx, dm, ts.Add(-time.Second), cis[0])
	require.Error(t, err, "previous timestamp should return error")
	require.False(t, acc)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))

	// 2 updates because of secondary
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())

	setPrim, upPrimCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	setSecond, upSecondCount := damem.GetRecord(SecondaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, cis[0].prim, setPrim)
	require.Equal(t, 1, upPrimCount)
	require.Equal(t, cis[0].second, setSecond)
	require.Equal(t, 1, upSecondCount)

	noPrim, noCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].prim))
	require.Equal(t, "", noPrim)
	require.Equal(t, 0, noCount)
}

func TestSetFewTimes(t *testing.T) {
	maxRec := 1
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, 1, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)

	ts := time.Now()
	_, err := updateDNS(ctx, dm, ts, cis[0])
	require.NoError(t, err)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))

	getPrim, upCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 1, upCount)
	require.Equal(t, cis[0].prim, getPrim)

	newPrim := "new-" + cis[0].prim
	_, err = dm.UpdateDNS(ctx, ts.Add(time.Second), cis[0].cid, cis[0].sid, newPrim, "")
	require.NoError(t, err)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*time.Second))

	getPrim, upCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 2, upCount)
	require.Equal(t, newPrim, getPrim)

	_, err = updateDNS(ctx, dm, ts.Add(2*time.Second), cis[0])
	require.NoError(t, err)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(3*time.Second))

	getPrim, upCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 3, upCount)
	require.Equal(t, cis[0].prim, getPrim)
}

func TestSetFewCid(t *testing.T) {
	maxRec := 1
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, 2, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	_, err := updateDNS(ctx, dm, ts, cis[0])
	require.NoError(t, err)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))

	getPrim1, upCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 1, upCount)
	require.Equal(t, cis[0].prim, getPrim1)
	noPrim, noCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[1].cid))
	require.Equal(t, 0, noCount)
	require.Equal(t, "", noPrim)

	_, err = updateDNS(ctx, dm, ts.Add(time.Second), cis[1])
	require.NoError(t, err)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*time.Second))

	getPrim1, upCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 1, upCount)
	require.Equal(t, cis[0].prim, getPrim1)
	getPrim2, upCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[1].cid))
	require.Equal(t, 1, upCount)
	require.Equal(t, cis[1].prim, getPrim2)

	_, err = dm.UpdateDNS(ctx, ts.Add(-time.Second), cis[0].cid, cis[0].sid, cis[1].prim, cis[1].second)
	require.Error(t, err, "a time stamp earlier than it existed")
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*time.Second))

	// same timestamp not change counter
	getPrim1, upCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 1, upCount)
	require.Equal(t, cis[0].prim, getPrim1)

	// next timestamp for cis[0].cid is ok
	_, err = dm.UpdateDNS(ctx, ts.Add(time.Second), cis[0].cid, cis[0].sid, cis[1].prim, cis[1].second)
	require.NoError(t, err)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*time.Second))

	getPrim1, upCount = damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, cis[0].cid))
	require.Equal(t, 2, upCount)
	require.Equal(t, cis[1].prim, getPrim1)
}

func TestSetFewHotAtOnceUpdate(t *testing.T) {
	maxRec := 3
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	for _, ci := range cis {
		_, err := updateDNS(ctx, dm, ts, ci)
		require.NoError(t, err)
	}

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())

	checkUpdateCount(t, damem, dac, cis, 1)
}

func TestSetFewHotAtOnceUpdateByOne(t *testing.T) {
	maxRec := 3
	mulRecs := 3
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec*mulRecs-1, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)

	for _, ci := range cis {
		noPrim, noCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
		require.Equal(t, 0, noCount)
		require.Equal(t, "", noPrim)
	}

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2*mulRecs, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)
}

func TestEliminateNotActive(t *testing.T) {
	maxRec := 4
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	require.Equal(t, maxRec, len(dm.cc.ci))

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	for i, ci := range cis {
		if i == 0 {
			continue
		}
		_, err := dm.UpdateDNS(ctx, ts.Add(time.Minute), ci.cid, ci.sid, ci.prim, "")
		require.NoError(t, err)
	}
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(dm.dmc.CacheTTL+time.Second))
	dm.serviceCycle(ts.Add(dm.dmc.CacheTTL+time.Second), dm.dmc.UpdDur)

	require.Equal(t, maxRec-1, len(dm.cc.ci))
	require.Equal(t, 1, len(dm.cc.lst))
	require.Contains(t, dm.cc.lst, clusterKey{metadb.PostgresqlCluster, metadb.DevEnvironment})
	lst := dm.cc.lst[clusterKey{metadb.PostgresqlCluster, metadb.DevEnvironment}]
	require.Equal(t, dm.cc.ls.ActiveClients, lst.ActiveClients)
	require.Equal(t, dm.cc.ls.LiveStatistic, lst.LiveStatistic)
	require.Equal(t, dm.cc.ls.LiveSlayer.LastResErr, lst.LiveSlayer.LastResErr)
	require.Equal(t, dm.cc.ls.LiveSlayer.ResErr, lst.LiveSlayer.ResErr)
	require.Equal(t, dm.cc.ls.LiveSlayer.Primary.LastFailedCycles, lst.LiveSlayer.Primary.LastFailedCycles)
}

func TestNoStrongUpdate(t *testing.T) {
	maxRec := 3
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	// active clients (came for update up to 2 TTL) should not update before few TTL cycles
	updateRequestAll(ctx, t, dm, cis, ts.Add(2*dm.dmc.DNSTTL), false)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*dm.dmc.DNSTTL))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	// other cycle is send again only primary, because of lazy update on secondary
	updateRequestAll(ctx, t, dm, cis, ts.Add(6*dm.dmc.DNSTTL), true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(6*dm.dmc.DNSTTL))
	require.Equal(t, 2*2-1, damem.GetUpdateRecordsCallCount())
	// only primary is updated twice when secondary only once
	checkUpdateCountPrimSecond(t, damem, dac, cis, 2, 1)

	// next cycles should update secondary
	updateRequestAll(ctx, t, dm, cis, ts.Add(7*dm.dmc.DNSTTL), false)
	// next cycles should update secondary
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(7*dm.dmc.DNSTTL))
	require.Equal(t, 2*2, damem.GetUpdateRecordsCallCount())
	// all updated twice
	checkUpdateCount(t, damem, dac, cis, 2)
}

func TestStrongUpdateAfterNoActivity(t *testing.T) {
	maxRec := 3
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	// we MUST update if not active during few cycles (more than 2 TTL) and send only primary because of lazy
	updateRequestAll(ctx, t, dm, cis, ts.Add(3*dm.dmc.DNSTTL), true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(3*dm.dmc.DNSTTL))
	require.Equal(t, 2*2-1, damem.GetUpdateRecordsCallCount())
	// only primary is updated twice when secondary only once
	checkUpdateCountPrimSecond(t, damem, dac, cis, 2, 1)

	// next active should not up primary and send lazy secondary
	updateRequestAll(ctx, t, dm, cis, ts.Add(5*dm.dmc.DNSTTL), false)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(5*dm.dmc.DNSTTL))
	require.Equal(t, 2*2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 2)
}

func TestNoStrongUpdateWithMulti(t *testing.T) {
	maxRec := 3
	mulRecs := 3
	notFull := 1
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec*mulRecs-notFull, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2*mulRecs, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	// accurate cycle update
	updateRequestAll(ctx, t, dm, cis, ts.Add(4*dm.dmc.DNSTTL), true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(4*dm.dmc.DNSTTL))
	require.Equal(t, (2+1)*mulRecs, damem.GetUpdateRecordsCallCount())
	// and lazy after cycle
	dm.prepAndUpdateCycle(ctx, ts.Add(5*dm.dmc.DNSTTL), dac, updSecond, metadb.PostgresqlCluster, metadb.DevEnvironment)
	require.Equal(t, 2*2*mulRecs, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 2)

	oneUpd := 0
	twoUpd := 0
	otherUpd := 0
	for _, ci := range cis {
		getPrim, upCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
		if upCount == 1 {
			oneUpd++
		} else if upCount == 2 {
			twoUpd++
		} else {
			otherUpd++
		}
		require.Equal(t, ci.prim, getPrim)
	}
	require.Equal(t, 0, otherUpd)
	require.Equal(t, len(cis), twoUpd)
	require.Equal(t, 0, oneUpd)

	// accurate next cycle update
	updateRequestAll(ctx, t, dm, cis, ts.Add(31*dm.dmc.DNSTTL), true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(31*dm.dmc.DNSTTL))
	require.Equal(t, (2*2+1)*mulRecs, damem.GetUpdateRecordsCallCount())
	// and lazy after cycle
	dm.prepAndUpdateCycle(ctx, ts.Add(32*dm.dmc.DNSTTL), dac, updSecond, metadb.PostgresqlCluster, metadb.DevEnvironment)
	require.Equal(t, 3*2*mulRecs, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 3)

	oneUpd = 0
	twoUpd = 0
	for _, ci := range cis {
		getPrim, upCount := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
		if upCount == 1 {
			oneUpd++
		} else if upCount == 2 {
			twoUpd++
		} else {
			otherUpd++
		}
		require.Equal(t, ci.prim, getPrim)
	}
	require.Equal(t, len(cis), otherUpd)
	require.Equal(t, 0, twoUpd)
	require.Equal(t, 0, oneUpd)
}

func TestLazyUpdateOnEveryCyclePrimaryUpdates(t *testing.T) {
	maxRec := 3
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2, damem.GetUpdateRecordsCallCount())
	checkUpdateCount(t, damem, dac, cis, 1)

	for c := 2; c < 7; c++ {
		// change primary for all
		for i := 0; i < maxRec; i++ {
			cis[i].prim = cis[i].prim + "-"
		}
		updateRequestAll(ctx, t, dm, cis, ts.Add(time.Duration(c)*dm.dmc.DNSTTL), true)
		prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Duration(c)*dm.dmc.DNSTTL))
		require.Equal(t, 2+(c-1), damem.GetUpdateRecordsCallCount())
		checkUpdateCountPrimSecond(t, damem, dac, cis, c, 1)
	}

	// MUST update lazy because of cycle count
	for i := 0; i < maxRec; i++ {
		cis[i].prim = cis[i].prim + "-"
	}
	updateRequestAll(ctx, t, dm, cis, ts.Add(9*dm.dmc.DNSTTL), true)
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(9*dm.dmc.DNSTTL))
	require.Equal(t, 9, damem.GetUpdateRecordsCallCount())
	checkUpdateCountPrimSecond(t, damem, dac, cis, 7, 2)
}

func TestCheckDNSAPIFailNotBreakUpdateOnlyPrimaries(t *testing.T) {
	logger := getLogger()
	maxRec := 3
	mulRec := 2
	dnsq := dnsqmem.New(logger)
	ctx, mdbmock, damem, dm, dac := initDMCustom(t, logger, maxRec, dnsq)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	// generate pack of random clusters
	cis := generateRandomClusterInfoArray(t, ctx, mulRec*maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)

	// check no clusters info exist
	checkNoRecords(t, damem, dac, cis)

	// update DNS requests
	ts := time.Now()
	for _, ci := range cis {
		acc, err := dm.UpdateDNS(ctx, ts, ci.cid, ci.sid, ci.prim, "")
		require.NoError(t, err)
		require.True(t, acc)
	}
	require.Equal(t, 2*maxRec, len(dm.cc.ci))

	// request update cycle 0 and check expected updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, mulRec, damem.GetUpdateRecordsCallCount())

	for _, ci := range cis {
		acc, err := dm.UpdateDNS(ctx, ts.Add(4*dm.dmc.DNSTTL), ci.cid, ci.sid, ci.prim, "")
		require.NoError(t, err)
		require.True(t, acc)
	}

	// request update cycle 1 and check expected updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(4*dm.dmc.DNSTTL+time.Second))
	require.Equal(t, 2*mulRec, damem.GetUpdateRecordsCallCount())

	for _, ci := range cis {
		fqdn := PrimaryFQDNByCid(dac, defaultConfig, ci.cid)
		prim, count := damem.GetRecord(fqdn)
		require.Equal(t, 2, count)
		require.Equal(t, ci.prim, prim)
		dnsq.UpdateCNAME(ctx, fqdn, prim)

		// check no actual update if resolve found
		acc, err := dm.UpdateDNS(ctx, ts.Add(5*dm.dmc.DNSTTL), ci.cid, ci.sid, ci.prim, "")
		require.NoError(t, err)
		require.False(t, acc)
	}

	// request update cycle 2 and check no new updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(2*dm.dmc.UpdDur+time.Second))
	require.Equal(t, 2*mulRec, damem.GetUpdateRecordsCallCount())

	// break some random index
	badIndex := maxRec >> 1
	dnsq.UpdateCNAME(ctx, PrimaryFQDNByCid(dac, defaultConfig, cis[badIndex].cid), "bad-cname")

	for i, ci := range cis {
		/// time stamp should be next power of accurate update, because target cname same
		acc, err := dm.UpdateDNS(ctx, ts.Add(31*dm.dmc.DNSTTL), ci.cid, ci.sid, ci.prim, "")
		require.NoError(t, err)
		// check for badIndex accepted for update
		if i == badIndex {
			require.True(t, acc)
		} else {
			require.False(t, acc)
		}
	}

	// request update cycle 3 and check expected updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(3*dm.dmc.UpdDur+time.Second))
	require.Equal(t, 2*mulRec+1, damem.GetUpdateRecordsCallCount())

	for i, ci := range cis {
		prim, count := damem.GetRecord(PrimaryFQDNByCid(dac, defaultConfig, ci.cid))
		// check for update broken index
		if i == badIndex {
			require.Equal(t, 3, count)
		} else {
			require.Equal(t, 2, count)
		}
		require.Equal(t, ci.prim, prim)
	}
}

func TestCheckResolveFailedAfterSuccessOne(t *testing.T) {
	logger := getLogger()
	maxRec := 3
	mulRec := 2
	dnsq := dnsqmem.New(logger)
	ctx, mdbmock, damem, dm, dac := initDMCustom(t, logger, maxRec, dnsq)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandomClusterInfoArray(t, ctx, mulRec*maxRec, mdbmock)
	preloadPubKeys(ctx, t, dm, cis)

	// check no clusters info exist
	checkNoRecords(t, damem, dac, cis)

	// update DNS requests
	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	require.Equal(t, 2*maxRec, len(dm.cc.ci))

	// request update cycle 0 and check expected updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	require.Equal(t, 2*mulRec, damem.GetUpdateRecordsCallCount())

	// prepare resolve table
	for _, ci := range cis {
		fqdnPrim := PrimaryFQDNByCid(dac, defaultConfig, ci.cid)
		fqdnSecond := SecondaryFQDNByCid(dac, defaultConfig, ci.cid)
		prim, countPrim := damem.GetRecord(fqdnPrim)
		second, countSecond := damem.GetRecord(fqdnSecond)
		require.Equal(t, 1, countPrim)
		require.Equal(t, ci.prim, prim)
		dnsq.UpdateCNAME(ctx, fqdnPrim, prim)
		require.Equal(t, 1, countSecond)
		require.Equal(t, ci.second, second)
		dnsq.UpdateCNAME(ctx, fqdnSecond, second)

		// check no actual update if resolve found
		acc, err := updateDNS(ctx, dm, ts.Add(2*dm.dmc.UpdDur), ci)
		require.NoError(t, err)
		require.False(t, acc)
	}

	// request update cycle 1
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(1*dm.dmc.UpdDur+time.Second))
	require.Equal(t, 2*mulRec, damem.GetUpdateRecordsCallCount())

	// emulate empty resolve
	badIndex := maxRec >> 1
	dnsq.UpdateCNAME(ctx, PrimaryFQDNByCid(dac, defaultConfig, cis[badIndex].cid), "")

	// check for only badIndex accepted for update
	updateRequestAllUpdateOnlyIndex(ctx, t, dm, cis, ts.Add(4*dm.dmc.DNSTTL), badIndex)

	// request update cycle 2
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(4*dm.dmc.DNSTTL+time.Second))
	require.Equal(t, 2*mulRec+1, damem.GetUpdateRecordsCallCount())

	// check for update only badIndex
	checkUpdateCountExcept(t, damem, dac, cis, 1, badIndex)
	checkUpdateCountForRecPrimSecond(t, damem, dac, cis[badIndex], 2, 1)
}

func TestUpdateOnFailingInBatch(t *testing.T) {
	logger := getLogger()
	maxRec := 50
	mulRec := 1
	failedMod := 13
	dnsq := dnsqmem.New(logger)
	ctx, mdbmock, damem, dm, dac := initDMCustom(t, logger, maxRec, dnsq)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	logger.Info(" + pregenerate")
	cis := generateRandomClusterInfoArray(t, ctx, mulRec*maxRec, mdbmock)
	logger.Info(" - postgenerate")
	preloadPubKeys(ctx, t, dm, cis)
	failedCids := make(map[string]string, 1+maxRec/failedMod)
	exceptList := make(map[int]bool)
	for i := failedMod; i < len(cis); i += failedMod {
		failedCids[cis[i].prim] = ""
		exceptList[i] = true
	}
	logger.Infof("failed cids: %s", failedCids)
	require.Equal(t, int(maxRec/failedMod), len(failedCids))
	damem.SetFailedTarget(failedCids)

	// check no clusters info exist
	checkNoRecords(t, damem, dac, cis)

	// update DNS requests
	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)
	require.Equal(t, mulRec*maxRec, len(dm.cc.ci))

	// request update cycle 0 and check expected updates
	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	// 7 and other magic is depends on how failed maxRec*mulRec splits by thirds
	require.LessOrEqual(t, 7, damem.GetUpdateRecordsFailedCount())
	require.GreaterOrEqual(t, 17, damem.GetUpdateRecordsFailedCount())
	require.LessOrEqual(t, 8, damem.GetUpdateRecordsCallCount())
	require.GreaterOrEqual(t, 18, damem.GetUpdateRecordsCallCount())
	// check correct record is update
	checkUpdateCountExceptList(t, damem, dac, cis, 1, exceptList)
	// check failed records not update at all
	for i := range exceptList {
		checkUpdateCountForRecPrimSecond(t, damem, dac, cis[i], 0, 1)
	}
}

func TestSetFewSid(t *testing.T) {
	maxRec := 1
	ctx, mdbmock, damem, dm, dac := initDM(t, maxRec)
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	cis := generateRandWithShards(t, ctx, 3, mdbmock)
	require.NotEmpty(t, cis[1].sid)
	preloadPubKeys(ctx, t, dm, cis)
	checkNoRecords(t, damem, dac, cis)

	ts := time.Now()
	updateRequestAll(ctx, t, dm, cis, ts, true)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(time.Second))
	checkUpdateCount(t, damem, dac, cis, 1)

	// update same not change any
	updateRequestAll(ctx, t, dm, cis, ts.Add(10*time.Second), false)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(11*time.Second))
	checkUpdateCount(t, damem, dac, cis, 1)

	// update one shard prim and second records
	newRec := 2
	cis[newRec].prim = "prim-up-" + uuid.Must(uuid.NewV4()).String()
	cis[newRec].second = "second-up-" + uuid.Must(uuid.NewV4()).String()
	updateRequestAllUpdateOnlyIndex(ctx, t, dm, cis, ts.Add(20*time.Second), newRec)

	prepAndUpdateBoth(ctx, dm, dac, ts.Add(21*time.Second))
	// and one force for lazy
	dm.prepAndUpdateCycle(ctx, ts.Add(41*time.Second), dac, updSecond, metadb.PostgresqlCluster, metadb.DevEnvironment)
	checkUpdateCountExcept(t, damem, dac, cis, 1, newRec)
	checkUpdateCountForRec(t, damem, dac, cis[newRec], 2)
}

func TestPrepRequestPart(t *testing.T) {
	recTotal := 6
	recInNet := 3
	maxRec := 2
	_, _, _, dm, dac := initDM(t, maxRec)
	updList := make([]ToUpd, recTotal)
	net := 0
	for i := 0; i < recTotal; i++ {
		if i%recInNet == 0 {
			net++
		}
		updList[i] = ToUpd{
			Fqdn:  fmt.Sprintf("fqdn-%d.%s", i, dac.Suffix),
			Netid: fmt.Sprintf("net-%d", net),
			Info: dnsapi.Update{
				CNAMENew: fmt.Sprintf("target-%d.%s", i, dac.Suffix),
			},
		}
	}

	reqUpdate0 := PrepReqPart(dm.logger, dac, updList)
	require.Equal(t, "net-1", reqUpdate0.GroupID)
	require.Equal(t, maxRec, len(reqUpdate0.Records))
	require.Equal(t, updList[0].Info, reqUpdate0.Records[updList[0].Fqdn])
	require.Equal(t, updList[1].Info, reqUpdate0.Records[updList[1].Fqdn])

	reqUpdate1 := PrepReqPart(dm.logger, dac, updList[2:])
	require.Equal(t, "net-1", reqUpdate1.GroupID)
	// only one because of new network_id
	require.Equal(t, 1, len(reqUpdate1.Records))
	require.Equal(t, updList[2].Info, reqUpdate1.Records[updList[2].Fqdn])

	reqUpdate2 := PrepReqPart(dm.logger, dac, updList[3:])
	require.Equal(t, "net-2", reqUpdate2.GroupID)
	require.Equal(t, maxRec, len(reqUpdate2.Records))
	require.Equal(t, updList[3].Info, reqUpdate2.Records[updList[3].Fqdn])
	require.Equal(t, updList[4].Info, reqUpdate2.Records[updList[4].Fqdn])

	reqUpdate3 := PrepReqPart(dm.logger, dac, updList[5:])
	require.Equal(t, "net-2", reqUpdate3.GroupID)
	// only one because left only one
	require.Equal(t, 1, len(reqUpdate3.Records))
	require.Equal(t, updList[5].Info, reqUpdate3.Records[updList[5].Fqdn])
}
