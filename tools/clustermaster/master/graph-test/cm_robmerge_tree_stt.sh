#!/usr/local/bin/bash


_scenario() {

    GLOBAL = !hostcfg:worm shape=ellipse res=cpu:0
    WALRUS = !hostcfg:h shape=rectangle res=cpu:0
    WALRUSclusters = !hostcfg:h !hostcfg:C shape=hexagon res=cpu:1/8
    WALRUSseghosts = !hostcfg:h !hostcfg:h shape=octagon res=cpu:1.0000001/2

    GLOBAL update_configs_ya:
    GLOBAL ya_init_finished: update_configs_ya
    GLOBAL make_mirrors_hash: ya_init_finished
    GLOBAL splitgeosharddata: make_mirrors_hash
    GLOBAL splitcrawllist: ya_init_finished
    GLOBAL prepare_repass: ya_init_finished
    GLOBAL btp_stop_daemon: ya_init_finished
    GLOBAL imp_copy_polytrie: ya_init_finished
    GLOBAL btp_preparefiles: btp_stop_daemon make_mirrors_hash prepare_repass imp_copy_polytrie
    GLOBAL btp_makemetafile: btp_preparefiles
    GLOBAL btp_prepare_globalconfig: btp_makemetafile
    WALRUS update_configs_walrus: update_configs_ya
    WALRUS walrus_init_finished: update_configs_walrus
    WALRUS btp_prepare_globalconfig_kludge: btp_prepare_globalconfig walrus_init_finished
    WALRUS rmtempfiles: btp_prepare_globalconfig_kludge 
    WALRUS waitconvert: walrus_init_finished
    WALRUS checkfreespace: rmtempfiles waitconvert 
    WALRUS mergetag: checkfreespace
    WALRUS import: mergetag
    WALRUS save_mirrors_hash: import
    WALRUS semiduplogs: mergetag
    WALRUS copysitemaps: mergetag
    WALRUS moveziplogs: mergetag
    WALRUS foreignlogs: moveziplogs
    WALRUS sitemaplogs: copysitemaps
    WALRUS makethin: import
    WALRUS backup_sitemaps: sitemaplogs
    WALRUS movemirrors: import make_mirrors_hash
    WALRUS findmirrors: import
    WALRUS create_multilanguage_host_logels: checkfreespace
    WALRUSclusters clusters_movemirrordocs: movemirrors res=cpu:1/20
    WALRUSseghosts copyforeignmirrors: movemirrors res=cpu:0,net:1/10
    GLOBAL all_copyforeignmirrors: copyforeignmirrors
    WALRUS backupmirrorlogs: clusters_movemirrordocs all_copyforeignmirrors
    WALRUS waitcopylogs: makethin semiduplogs foreignlogs backup_sitemaps backupmirrorlogs create_multilanguage_host_logels
    WALRUS copylogs: waitcopylogs
    WALRUS copycrawllist: splitcrawllist import
    WALRUS prepare_urllists: copycrawllist
    WALRUS prepare_crawlranklist: mergetag
    WALRUS mesh_import: mergetag
    WALRUS convert_oldlogels: copylogs
    WALRUS sortdocuid: checkfreespace
    WALRUS logreader: copylogs backupmirrorlogs prepare_urllists convert_oldlogels create_multilanguage_host_logels sortdocuid findmirrors
    WALRUS movesemidups: logreader
    WALRUS adddoc: movesemidups
    WALRUS rmcurlogs: logreader
    WALRUS robotsmerge: logreader
    WALRUS addmainmirrorhosts: robotsmerge
    WALRUS preparerobotstxtdeny: robotsmerge
    GLOBAL robotstxtdenydiff: preparerobotstxtdeny
    WALRUS findsoftmirrors: logreader
    WALRUSseghosts prepare_sendsoftmirrors: findsoftmirrors
    WALRUS preparesitemapsdata: logreader robotsmerge
    WALRUS sitemapprocessor: logreader preparesitemapsdata
    WALRUS wait_spam: logreader res=cpu:0
    WALRUS spamdelta_rst: adddoc rmcurlogs
    WALRUSclusters clusters_prewalrus: adddoc spamdelta_rst
    WALRUSclusters clusters_checkind: clusters_prewalrus res=cpu:0,io:1/1
    WALRUSclusters clusters_send_rsslinks: clusters_prewalrus
    WALRUS all_clusters_prewalrus: clusters_prewalrus
    WALRUSclusters clusters_mergearch: all_clusters_prewalrus
    WALRUSclusters clusters_docsign: clusters_mergearch
    WALRUS rmparseddocs: clusters_prewalrus clusters_docsign
    WALRUS mergeurl: rmparseddocs sitemapprocessor
    WALRUS updhost: mergeurl
    WALRUS paramstatsmake: mergetag
    WALRUS paramstatsrfl: paramstatsmake
    WALRUS paramstatscopy: paramstatsrfl
    WALRUS checksoftmirrors: checkfreespace
    WALRUS filtercache: robotsmerge checksoftmirrors
    WALRUS robotzonecache: import mergetag
    WALRUS correctshingle: logreader
    WALRUS calchops: logreader
    WALRUS copyiplimits: mergetag
    WALRUS updurl: updhost paramstatscopy filtercache robotzonecache correctshingle calchops sitemapprocessor copyiplimits
    WALRUS updsitemap: sitemapprocessor updurl
    WALRUS deldoc: updurl
    WALRUS simhash: deldoc
    WALRUS genactions: deldoc
    WALRUS mergehosts: genactions
    WALRUS mergeintlinks: mergehosts
    WALRUS get_mr_data: updurl
    WALRUS mergeextlinks: mergehosts get_mr_data
    WALRUS mergeinclinks: mergeextlinks
    WALRUS mergeweights: logreader
    WALRUS updurlfinal: mergeintlinks mergeinclinks mergeweights copyiplimits updsitemap get_mr_data prepare_crawlranklist mesh_import
    WALRUS copygeosharddata: splitgeosharddata mergetag
    WALRUS copysitemaporange: updurlfinal
    WALRUS mergesig: deldoc clusters_docsign
    WALRUS docsig_split: mergesig
    WALRUS dcmpinesort: mergesig
    WALRUS dcmpine_clean: docsig_split dcmpinesort
    WALRUS dcmpine: dcmpine_clean
    WALRUS dcmpine_child: dcmpine
    WALRUS indexsigdoc: deldoc
    WALRUS makehostsigdoc: deldoc
    WALRUS crawlpolitics: updurlfinal
    WALRUS metrics: get_mr_data
    WALRUS prepareegraph: crawlpolitics metrics
    WALRUS dbcheckup: dcmpine_child indexsigdoc crawlpolitics
    WALRUS mergetexts: mergeintlinks mergeextlinks
    WALRUS mergeexturls: mergeextlinks
    WALRUS rmlinkwork: updurlfinal
    WALRUS linkcheckup: mergetexts mergeexturls rmlinkwork
    WALRUS shinglemerge: updurlfinal
    WALRUS shinglecheckup: shinglemerge
    WALRUS checkup: dbcheckup linkcheckup shinglecheckup
    WALRUS urlweighter: rmlinkwork
    WALRUS linkextract: updurlfinal mergetexts
    WALRUS updstat: updurlfinal dcmpine_child
    WALRUS shingletestgen: correctshingle
    WALRUS shingletest: shingletestgen
    WALRUSclusters clusters_copyspamdelta: clusters_prewalrus res=net:0.51/1,cpu:0
    WALRUS copyspamdelta: clusters_copyspamdelta
    WALRUSclusters clusters_kciter: clusters_prewalrus
    WALRUS kcsum: clusters_kciter
    WALRUS kcrotate: kcsum deldoc
    WALRUS selectgeo: updurlfinal
    WALRUS selectgeoshard: updurlfinal copygeosharddata
    WALRUS copygeourls: selectgeo
    WALRUS copygeoshardurls: selectgeoshard
    WALRUSclusters clusters_mergetags: dbcheckup linkcheckup shinglecheckup urlweighter linkextract updstat
    WALRUS makerfl: paramstatsrfl updurl
    WALRUS robotrankruler: updurl
    WALRUS copy_sitemap_files: updsitemap
    WALRUS deleteold: copy_sitemap_files dbcheckup linkcheckup shinglecheckup urlweighter linkextract updstat shingletest kcrotate copygeourls copygeoshardurls clusters_mergetags prepareegraph clusters_checkind save_mirrors_hash makerfl simhash robotrankruler 
    WALRUS renamedb: deleteold
    WALRUS renameindex: deleteold
    WALRUS rename: renamedb renameindex
    WALRUS copystat: rename
    WALRUS copychisla: checkup
    WALRUSseghosts seghosts_copyforeign: checkup urlweighter linkextract prepare_sendsoftmirrors res=cpu:0,net:1/10
    WALRUSseghosts seghosts_copyrecent: linkextract res=cpu:0,net:1/10
    WALRUS copy_url_stat: updstat
    WALRUS copydocip: crawlpolitics
    WALRUS copyall: copystat copychisla seghosts_copyforeign seghosts_copyrecent copy_url_stat copydocip prepareegraph copysitemaporange
    WALRUS sendmainmirrors: rename
    WALRUS local_datmerge: makerfl rename copyall wait_spam sendmainmirrors clusters_send_rsslinks
    GLOBAL all_sendsoftmirrors: local_datmerge
    WALRUS joinsoftmirrors: all_sendsoftmirrors
    GLOBAL paramstatsmerge: local_datmerge
    GLOBAL paramstatsclean: paramstatsmerge
    GLOBAL aggr_shingle_change: local_datmerge
    GLOBAL cat_urls_stat: local_datmerge
    GLOBAL geobase: local_datmerge
    GLOBAL geoshard: local_datmerge
    GLOBAL iplimits: local_datmerge
    GLOBAL sitemaporangeexport: local_datmerge
    GLOBAL egraph: sitemaporangeexport
    GLOBAL local_datmerge_kludge: local_datmerge
    GLOBAL mail_chisla: local_datmerge_kludge ?ENVIRONMENT~production
    GLOBAL update_host_stat: mail_chisla
    GLOBAL datmerge: makehostsigdoc copyspamdelta paramstatsclean aggr_shingle_change cat_urls_stat geobase geoshard iplimits egraph update_host_stat ?ENVIRONMENT~production robotstxtdenydiff joinsoftmirrors addmainmirrorhosts pinned=retry-run-path
	
    GLOBAL run_cm_from_cm: datmerge
}

function markme
{
    local step_name="$1"
    touch "$datdir/$step_name.step"
    echo "markme "$1
}

function target_update_configs_ya {
	markme "update_configs_ya"
}

function target_ya_init_finished {
	markme "ya_init_finished"
}

function target_make_mirrors_hash {
	markme "make_mirrors_hash"
}

function target_splitgeosharddata {
	markme "splitgeosharddata"
}

function target_splitcrawllist {
	markme "splitcrawllist"
}

function target_prepare_repass {
	markme "prepare_repass"
}

function target_btp_stop_daemon {
	markme "btp_stop_daemon"
}

function target_imp_copy_polytrie {
	markme "imp_copy_polytrie"
}

function target_btp_preparefiles {
	markme "btp_preparefiles"
}

function target_btp_makemetafile {
	markme "btp_makemetafile"
}

function target_btp_prepare_globalconfig {
	markme "btp_prepare_globalconfig"
}

function target_update_configs_walrus {
	markme "update_configs_walrus"
}

function target_walrus_init_finished {
	markme "walrus_init_finished"
}

function target_btp_prepare_globalconfig_kludge {
	markme "btp_prepare_globalconfig_kludge"
}


function target_rmtempfiles {
	markme "rmtempfiles"
}

function target_waitconvert {
	markme "waitconvert"
}

function target_checkfreespace {
	markme "checkfreespace"
}

function target_mergetag {
	markme "mergetag"
}

function target_import {
	markme "import"
}

function target_save_mirrors_hash {
	markme "save_mirrors_hash"
}

function target_semiduplogs {
	markme "semiduplogs"
}

function target_copysitemaps {
	markme "copysitemaps"
}

function target_moveziplogs {
	markme "moveziplogs"
}

function target_foreignlogs {
	markme "foreignlogs"
}

function target_sitemaplogs {
	markme "sitemaplogs"
}

function target_makethin {
	markme "makethin"
}

function target_backup_sitemaps {
	markme "backup_sitemaps"
}

function target_movemirrors {
	markme "movemirrors"
}

function target_findmirrors {
	markme "findmirrors"
}

function target_create_multilanguage_host_logels {
	markme "create_multilanguage_host_logels"
}

function target_clusters_movemirrordocs {
	markme "clusters_movemirrordocs."$1
}

function target_copyforeignmirrors {
	markme "copyforeignmirrors."$1
}

function target_all_copyforeignmirrors {
	markme "all_copyforeignmirrors"
}

function target_backupmirrorlogs {
	markme "backupmirrorlogs"
}

function target_waitcopylogs {
	markme "waitcopylogs"
}

function target_copylogs {
	markme "copylogs"
}

function target_copycrawllist {
	markme "copycrawllist"
}

function target_prepare_urllists {
	markme "prepare_urllists"
}

function target_prepare_crawlranklist {
	markme "prepare_crawlranklist"
}

function target_mesh_import {
	markme "mesh_import"
}

function target_convert_oldlogels {
	markme "convert_oldlogels"
}

function target_sortdocuid {
	markme "sortdocuid"
}

function target_logreader {
	markme "logreader"
}

function target_movesemidups {
	markme "movesemidups"
}

function target_adddoc {
	markme "adddoc"
}

function target_rmcurlogs {
	markme "rmcurlogs"
}

function target_robotsmerge {
	markme "robotsmerge"
}

function target_addmainmirrorhosts {
	markme "addmainmirrorhosts"
}

function target_preparerobotstxtdeny {
	markme "preparerobotstxtdeny"
}

function target_robotstxtdenydiff {
	markme "robotstxtdenydiff"
}

function target_findsoftmirrors {
	markme "findsoftmirrors"
}

function target_prepare_sendsoftmirrors {
	markme "prepare_sendsoftmirrors."$1
}

function target_preparesitemapsdata {
	markme "preparesitemapsdata"
}

function target_sitemapprocessor {
	markme "sitemapprocessor"
}

function target_wait_spam {
	markme "wait_spam"
}

function target_spamdelta_rst {
	markme "spamdelta_rst"
}

function target_clusters_prewalrus {
	markme "clusters_prewalrus."$1
}

function target_clusters_checkind {
	markme "clusters_checkind."$1
}

function target_clusters_send_rsslinks {
	markme "clusters_send_rsslinks."$1
}

function target_all_clusters_prewalrus {
	markme "all_clusters_prewalrus"
}

function target_clusters_mergearch {
	markme "clusters_mergearch."$1
}

function target_clusters_docsign {
	markme "clusters_docsign."$1
}

function target_rmparseddocs {
	markme "rmparseddocs"
}

function target_mergeurl {
	markme "mergeurl"
}

function target_updhost {
	markme "updhost"
}

function target_paramstatsmake {
	markme "paramstatsmake"
}

function target_paramstatsrfl {
	markme "paramstatsrfl"
}

function target_paramstatscopy {
	markme "paramstatscopy"
}

function target_checksoftmirrors {
	markme "checksoftmirrors"
}

function target_filtercache {
	markme "filtercache"
}

function target_robotzonecache {
	markme "robotzonecache"
}

function target_correctshingle {
	markme "correctshingle"
}

function target_calchops {
	markme "calchops"
}

function target_copyiplimits {
	markme "copyiplimits"
}

function target_updurl {
	markme "updurl"
}

function target_updsitemap {
	markme "updsitemap"
}

function target_deldoc {
	markme "deldoc"
}

function target_simhash {
	markme "simhash"
}

function target_genactions {
	markme "genactions"
}

function target_mergehosts {
	markme "mergehosts"
}

function target_mergeintlinks {
	markme "mergeintlinks"
}

function target_get_mr_data {
	markme "get_mr_data"
}

function target_mergeextlinks {
	markme "mergeextlinks"
}

function target_mergeinclinks {
	markme "mergeinclinks"
}

function target_mergeweights {
	markme "mergeweights"
}

function target_updurlfinal {
	markme "updurlfinal"
}

function target_copygeosharddata {
	markme "copygeosharddata"
}

function target_copysitemaporange {
	markme "copysitemaporange"
}

function target_mergesig {
	markme "mergesig"
}

function target_docsig_split {
	markme "docsig_split"
}

function target_dcmpinesort {
	markme "dcmpinesort"
}

function target_dcmpine_clean {
	markme "dcmpine_clean"
}

function target_dcmpine {
	markme "dcmpine"
}

function target_dcmpine_child {
	markme "dcmpine_child"
}

function target_indexsigdoc {
	markme "indexsigdoc"
}

function target_makehostsigdoc {
	markme "makehostsigdoc"
}

function target_crawlpolitics {
	markme "crawlpolitics"
}

function target_metrics {
	markme "metrics"
}

function target_prepareegraph {
	markme "prepareegraph"
}

function target_dbcheckup {
	markme "dbcheckup"
}

function target_mergetexts {
	markme "mergetexts"
}

function target_mergeexturls {
	markme "mergeexturls"
}

function target_rmlinkwork {
	markme "rmlinkwork"
}

function target_linkcheckup {
	markme "linkcheckup"
}

function target_shinglemerge {
	markme "shinglemerge"
}

function target_shinglecheckup {
	markme "shinglecheckup"
}

function target_checkup {
	markme "checkup"
}

function target_urlweighter {
	markme "urlweighter"
}

function target_linkextract {
	markme "linkextract"
}

function target_updstat {
	markme "updstat"
}

function target_shingletestgen {
	markme "shingletestgen"
}

function target_shingletest {
	markme "shingletest"
}

function target_clusters_copyspamdelta {
	markme "clusters_copyspamdelta."$1
}

function target_copyspamdelta {
	markme "copyspamdelta"
}

function target_clusters_kciter {
	markme "clusters_kciter."$1
}

function target_kcsum {
	markme "kcsum"
}

function target_kcrotate {
	markme "kcrotate"
}

function target_selectgeo {
	markme "selectgeo"
}

function target_selectgeoshard {
	markme "selectgeoshard"
}

function target_copygeourls {
	markme "copygeourls"
}

function target_copygeoshardurls {
	markme "copygeoshardurls"
}

function target_clusters_mergetags {
	markme "clusters_mergetags."$1
}

function target_makerfl {
	markme "makerfl"
}

function target_robotrankruler {
	markme "robotrankruler"
}

function target_copy_sitemap_files {
	markme "copy_sitemap_files"
}

function target_deleteold {
	markme "deleteold"
}

function target_renamedb {
	markme "renamedb"
}

function target_renameindex {
	markme "renameindex"
}

function target_rename {
	markme "rename"
}

function target_copystat {
	markme "copystat"
}

function target_copychisla {
	markme "copychisla"
}

function target_seghosts_copyforeign {
	markme "seghosts_copyforeign."$1
}

function target_seghosts_copyrecent {
	markme "seghosts_copyrecent."$1
}

function target_copy_url_stat {
	markme "copy_url_stat"
}

function target_copydocip {
	markme "copydocip"
}

function target_copyall {
	markme "copyall"
}

function target_sendmainmirrors {
	markme "sendmainmirrors"
}

function target_local_datmerge {
	markme "local_datmerge"
}

function target_all_sendsoftmirrors {
	markme "all_sendsoftmirrors"
}

function target_joinsoftmirrors {
	markme "joinsoftmirrors"
}

function target_paramstatsmerge {
	markme "paramstatsmerge"
}

function target_paramstatsclean {
	markme "paramstatsclean"
}

function target_aggr_shingle_change {
	markme "aggr_shingle_change"
}

function target_cat_urls_stat {
	markme "cat_urls_stat"
}

function target_geobase {
	markme "geobase"
}

function target_geoshard {
	markme "geoshard"
}

function target_iplimits {
	markme "iplimits"
}

function target_sitemaporangeexport {
	markme "sitemaporangeexport"
}

function target_egraph {
	markme "egraph"
}

function target_local_datmerge_kludge {
	markme "local_datmerge_kludge"
}

function target_mail_chisla {
	markme "mail_chisla"
}

function target_update_host_stat {
	markme "update_host_stat"
}

function target_datmerge {
	markme "datmerge"
}

function target_run_cm_from_cm {
	markme "run_cm_from_cm"
	true
}

target_${1:-none} ${2:-}
