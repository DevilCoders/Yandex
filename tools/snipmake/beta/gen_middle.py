import sys

used = dict()
port = 8909 - 1

HEAD="""
<Server>
    #Port ${ ServerPort and ServerPort or 8031 }
    Port 8902
    Threads 8
    QueueSize 10
    AdminThreads 1
    #Compression true
    Compression false
    #LoadLog ${ LoadLog and LoadLog or '/usr/local/www/logs/loadlog-w' }
    #EventLog ${ EventLog and EventLog or '/usr/local/www/logs/wcmlog' }
    LoadLog middle_loadlog-w
    EventLog middle_wcmlog
</Server>

<Collection autostart="must" meta="yes" id="yandsearch">
    #RequestThreads 30
    RequestThreads 6
    RequestQueueSize 15
    ReAskThreads 1
    ReAskQueueSize 50

    SerializationProtocolVersion 5
    MaxSnippetsPerRequest 50

    #IndexDir /hol/webmeta/pure
    IndexDir /place/home/alivshits/SHARED/middlesearch_data/PURE
    #${ CompressionString and CompressionString or 'Compression true' }
    ${ CompressionString and CompressionString or 'Compression false' }

    <QueryCache>
        #Dir      ${ QueryCacheDir and QueryCacheDir or '/hol/webmeta/search.temp.1' }
        Dir      middle_cach
        #LifeTime 0
        LifeTime 1000
        MinCost  5000
        #CompressionLevel lzf
        Arenas 10
        ${ FileCacherString and FileCacherString or '' }
    </QueryCache>
    <Squid>
        LifeTime 0
        #CompressionLevel lzf
        MemoryLimit ${ SquidMemoryLimit and SquidMemoryLimit or 2000000000 }
        CacheLifeTime ${ CacheLifeTime and CacheLifeTime or '7200s' }
        Arenas 10
        BlockSize 1024
    </Squid>

    #Limits "d" 1000, "" 1000
    #${ RelevSuppString and RelevSuppString or ''}
    #MetaSearchOptions DontSearchOnMain TwoStepQuery DontWaitForAllSources SingleFileCache
    MetaSearchOptions DontSearchOnMain TwoStepQuery WaitForAllSources
    ReAskOptions ReAskIncompleteSources = yes
    #TimeoutTable ${ TimeoutTable and TimeoutTable or '52ms 62ms 82ms 110ms 146ms 190ms 242ms 302ms 8s'}
    TimeoutTable 150 200 250 300 8000 16000
    ConnectTimeout ${ ConnectTimeout and ConnectTimeout or '70ms' }
    #SearchReplyTimeoutLimited 300000
    SearchReplyTimeoutLimited 600000
    NGroupsForSourceMultiplier  1
    SmartReAskOptions ReAskSources=yes
    #${ MemSentryString and MemSentryString or '' }
    #ReArrangeOptions ${ ReArrangeOptions and ReArrangeOptions or 'DupCateg(CommercialLevel=0.1) WikiPedia Porno(BestRecall=3,AccidentalLevel=0.46,FalseLevel=0.2) Trash Foreign RearrDup(RangeLimit=70,RmDupLimit=50,RelevBound=0.01,DupRearrangeFactors=AuraDocLogAuthor:1) Vendor FilterSnip(Threshold=-1.0) Auto(ConfigRule=vendor:1_club:1) Diversity(Surjik=1,Diversity=1,Script=categorizer.txt) Pressportret Grunwald Route(route) DupCategFix' }
    #RearrangeDataDir ${RearrangeDataDir and RearrangeDataDir or '/hol/arkanavt/rearrange'}
    RearrangeDataDir /place/home/alivshits/SHARED/middlesearch_data/rearrange_data/r4
    NTiers 2
    TiersLimitTable 1
    TiersClassifierThresholdTable 0.2


"""

FOOTER = """



</Collection>


"""

print HEAD

for line in sys.stdin:
    srv = line.strip()
    p = used.get(srv, port) + 1
    used[srv] = p
    print """<SearchSource>
Options MaxAttempts = 1, RandomGroupSelection=yes
CgiSearchPrefix http://%s.yandex.ru:%d/yandsearch@80@0
</SearchSource>""" % (srv, p)

print FOOTER

