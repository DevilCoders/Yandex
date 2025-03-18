<DaemonConfig>
    LoggerType : ${LOG_PATH or "/var/log"}/current-global-rtyserver${LOG_POSTFIX or ".log"}
    <!-- library/cpp/logger/priority.h:
         TLOG_EMERG       = 0,   /* system is unusable */
         TLOG_ALERT       = 1,   /* action must be taken immediately */
         TLOG_CRIT        = 2,   /* critical conditions */
         TLOG_ERR         = 3,   /* error conditions */
         TLOG_WARNING     = 4,   /* warning conditions */
         TLOG_NOTICE      = 5,   /* normal but significant condition */
         TLOG_INFO        = 6,   /* informational */
         TLOG_DEBUG       = 7    /* debug-level messages */
    -->
    LogLevel : ${LOG_LEVEL}
    LogRotation : false
    EnableStatusControl : true
    MetricsMaxAge : 21
    MetricsPrefix : Fusion_
    StdErr : ${LOG_PATH or "/var/log"}/current-rtyserver-stderr${LOG_POSTFIX or ".log"}
    StdOut : ${LOG_PATH or "/var/log"}/current-rtyserver-stdout${LOG_POSTFIX or ".log"}
    <Controller>
        ConfigsRoot : ${CONFIG_PATH and CONFIG_PATH or _BIN_DIRECTORY}/
        StateRoot : ${STATE_ROOT and STATE_ROOT or _BIN_DIRECTORY}/
        ClientTimeout : 200
        MaxConnections : 0
        MaxQueue : 0
        Port : ${BACKEND_CONTROLLER_PORT or ( BasePort +3 )}
        StartServer : 1
        Threads : 20
        EnableNProfile: 0
        Log : ${LOG_PATH or "/var/log"}/current-controller-rtyserver${LOG_POSTFIX or ".log"}
    </Controller>
</DaemonConfig>
<Server>
    AdditionalModules : DOCFETCHER, Synchronizer
    Components : OXY,DDK
    SearchersCountLimit : 15
    IndexDir : ${INDEX_DIRECTORY}
    IndexGenerator : OXY
    IsPrefixedIndex : 0
    PruneAttrSort : oxy
    <ModulesConfig>
        <DOCFETCHER>
            Enabled : true
            <!-- SearchOpenDocAgeSec : ${ 12 * 60 * 60 } -->
            LogFile : ${LOG_PATH or "/var/log"}/current-docfetcher-rtyserver${LOG_POSTFIX or ".log"}
            StateFile : ${INDEX_DIRECTORY}/df.state
            <Stream>
                Name : ImagesUltraExp
                ClientId : saas-refresh-imageultra/${HOSTNAME}:${BasePort}
                ConsistentClient : true
                DistAgeAsDocAge : false

                UseCompression : true
                ConsistentClientOptions : EnableDynamicSwitch=false,MaxPrimaryReplicas=1,RestrictEmptyCacheSwitch=false
                {% include 'rtyserver-replicas.tpl' %}

                DistributorStream : ${STREAM and STREAM or "imagesultra"}
                DistributorAttributes : ${DISTATTRS and DISTATTRS or ""}

                MaxAgeToGetSec : ${1 * 24 * 60 * 60}
                MaxDocAgeToKeepSec : ${7 * 24 * 60 * 60}
                MemoryIndexDistAgeThreshold : ${2 * 60}
                <!-- Name : Normal -->
                OverlapAge : 60
                ProxyType : RTYSERVER
                Rate : 500
                ShardMin : ${ tostring(math.floor(65533 * SHARD_ID / SHARDS_NUMBER)) }
                ShardMax : ${ tostring(math.floor(65533 * (SHARD_ID + 1) / SHARDS_NUMBER) - 1 + math.floor((SHARD_ID + 1) / SHARDS_NUMBER) ) }
                <!--
                Shard : ${SHARD_ID}
                NumShards : ${SHARDS_NUMBER}
                -->
                StreamId : 0
                StreamType : Distributor

                <!-- SyncThreshold : 129600
                SyncServer : saas-zookeeper1.search.yandex.net:14880,saas-zookeeper2.search.yandex.net:14880,saas-zookeeper3.search.yandex.net:14880,saas-zookeeper4.search.yandex.net:14880,saas-zookeeper5.search.yandex.net:14880
                SyncPath : /indexBackups/saas_refresh_production_images_base -->
            </Stream>
        </DOCFETCHER>
        <Synchronizer>
            DetachPath: ${DETACH_DIRECTORY}
        </Synchronizer>
    </ModulesConfig>
    <Searcher>
        <!-- AccessLog : ${LOG_PATH or "/var/log"}/current-loadlog-rtyserver${LOG_POSTFIX or ".log"} -->
        ArchivePolicy : INMEM
        AdditionalLockedFiles : indexannimgdata;indexannimgsent;indexanninv;indexannkey;indeximg3;indexsemdesc;indexinvhash;docindex_l1.megawad
        DelegateRequestOptimization: false
        DefaultBaseSearchConfig : ${CONFIG_PATH}/basesearch-refresh
        EnableUrlHash : true
        BroadcastFetch : true
        ExternalSearch : ${EXTERNAL_SEARCH and EXTERNAL_SEARCH or "imagesearch"}
        FactorsInfo :
        <!-- FiltrationModel : WEIGHT -->
        KeepAllDocuments : 0
        Limits : "d" 1000, "" 1000
        LoadLog : ${LOG_PATH or "/var/log"}/current-loadlog-fusion${LOG_POSTFIX or ".log"}
        LockIndexFiles : true
        PassageLog : ${LOG_PATH or "/var/log"}/current-passagelog-fusion${LOG_POSTFIX or ".log"}
        PrefetchSizeBytes : 10000000000
        ReArrangeOptions :
        RequestLimits : MergeCycles=5, MergeAndFetchCycles=10, PoolSize=1073741824
        <!-- ScatterTimeout : 150000 -->
        SkipSameDocids : true
        SearchPath : yandsearch
        UseRTYExtensions : false
        <!-- WildcardSearch : infix -->
        <HttpOptions>
            ClientTimeout : 10000
            CompressionEnabled : true
            MaxConnections : ${ 1 + NCPU * 20 }
            MaxFQueueSize : 100
            MaxQueue : 0
            MaxQueueSize : ${ 1 + NCPU * 4 }
            Port : ${BACKEND_SEARCH_PORT or ( BasePort + 0 )}
            Threads : ${ 1 + NCPU * 2 }
        </HttpOptions>
    </Searcher>
    <BaseSearchersServer>
        ClientTimeout : 200
        MaxConnections : 0
        MaxQueue : 0
        Port : ${BACKEND_BASESEARCH_PORT or ( BasePort + 1 )}
        Threads : ${ 1 + NCPU * 2 }
    </BaseSearchersServer>
    <Repair>
        Enabled : false
        <!-- Threads : 4 -->
    </Repair>
    <Merger>
        Enabled : true
        MaxDocumentsToMerge : 21000000
        MaxSegments : 1
        MergerCheckPolicy : NEWINDEX
        Threads : 4
        IndexSwitchSystemLockFile : /tmp/indexswitchlock
        MaxDeadlineDocs: 1000000
    </Merger>
    <Logger>
        JournalDir : ${ JournalDir and JournalDir or '/usr/local/www/logs' }
    </Logger>
    <Monitoring>
        Enabled : false
    </Monitoring>
    <Indexer>
        <Common>
            DefaultCharset : utf-8
            DefaultLanguage : rus
            DefaultLanguage2 : eng
            Groups : $docid$:1
            <!-- IndexLog : ${LOG_PATH or "/var/log"}/current-index-rtyserver${LOG_POSTFIX or ".log"} -->
            OxygenOptionsFile : ${CONFIG_PATH}/OxygenOptions.cfg
            RecognizeLibraryFile : NOTSET
            UseSlowUpdate : true
            <HttpOptions>
                ClientTimeout : 200
                MaxConnections : 0
                MaxQueue : 0
                Port : ${BACKEND_INDEXER_PORT or ( BasePort + 2 )}
                Threads : 4
            </HttpOptions>
        </Common>
        <Disk>
            SearchEnabled : true
            SearchObjectsDirectory : ${RTINDEX_DIRECTORY}
            ConnectionTimeout : 100
            DocumentsQueueSize : 10000
            MaxDocuments : 50000
            Threads : 1
            TimeToLiveSec : 300
            WaitCloseForMerger : true
        </Disk>
        <Memory>
            TimeToLiveSec : 15
            MaxDocuments : 80000
            ConnectionTimeout : 100
            DocumentsQueueSize : 10000
            Enabled : false
            GarbageCollectionTime : 50
            MaxDocumentsReserveCapacityCoeff : 3
            RealTimeExternalFilesPath : ${STATIC_DATA_DIRECTORY}
            RealTimeFeatureConfig : +useBinaryErf +useUtfNavigate
            RealTimeLoadC2P : geo geoa
        </Memory>
    </Indexer>
    <ComponentsConfig>
        <DDK>
            DefaultLifetimeMinutes : ${10 * 24 * 60}
        </DDK>
        <FULLARC>
            MaxPartCount : 64
            MinPartSizeFactor : 0.8
            ActiveLayers: full,merge
            <Layers>
                <merge>
                     MaxPartCount: 8
                     MinPartSizeFactor: 0.5
                </merge>
            </Layers>
        </FULLARC>
        <OXY>
            ArchiveLayersFilteredForIndex: full
            ArchiveLayersFilteredForMerge: merge
            AdditionalRequiredMergeTuples: Megawad ImgErf ImgRegErf ImgDlV7Knn ImgT2TKnn
        </OXY>
    </ComponentsConfig>
</Server>
