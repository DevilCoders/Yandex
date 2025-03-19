{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% do config_site.update({'tez': salt['grains.filter_by']({
    'Debian': {
        'tez.lib.uris': 'file:/usr/lib/tez,file:/usr/lib/tez/lib',
        'tez.use.cluster.hadoop-libs': 'true',
        'tez.history.logging.service.class': 'org.apache.tez.dag.history.logging.ats.ATSHistoryLoggingService',
        'tez.tez-ui.history-url.base': 'http://' + masternode + ':8188/tez-ui/',
        'tez.am.node-blacklisting.enabled': 'false',
        'tez.session.am.dag.submit.timeout.secs': 15
    }
}, merge=salt['pillar.get']('data:properties:tez'))}) %}
