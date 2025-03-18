# -*- coding: utf-8 -*-
import copy

from library.python.monitoring.solo.objects.yasm import Signal


def build_deploy_signal(stage, deploy_unit, signal, color=None):
    if isinstance(stage, list):
        stage = ",".join(stage)

    if isinstance(deploy_unit, list):
        deploy_unit = ",".join(deploy_unit)

    tag = "itype=deploy;deploy_unit={};stage={}".format(deploy_unit, stage)
    new_signal = copy.copy(signal)
    new_signal.host = "ASEARCH"
    new_signal.tag = tag
    new_signal.color = color
    return new_signal


# Общие сигналы Голована - https://wiki.yandex-team.ru/golovan/common-signals/

counter_instance_tmmv = Signal(name="counter-instance_tmmv")

# porto CPU
portoinst_cpu_usage_cores_txxx = Signal(name="portoinst-cpu_usage_cores_txxx")
portoinst_cpu_usage_cores_tmmv = Signal(name="portoinst-cpu_usage_cores_tmmv")
portoinst_cpu_usage_cores_tvvv = Signal(name="portoinst-cpu_usage_cores_tvvv")
portoinst_cpu_usage_cores_hgram = Signal(name="portoinst-cpu_usage_cores_hgram")
portoinst_cpu_usage_system_cores_tmmv = Signal(name="portoinst-cpu_usage_system_cores_tmmv")
portoinst_cpu_usage_system_cores_hgram = Signal(name="portoinst-cpu_usage_system_cores_hgram")
portoinst_cpu_wait_cores_tmmv = Signal(name="portoinst-cpu_wait_cores_tmmv")
portoinst_cpu_wait_cores_hgram = Signal(name="portoinst-cpu_wait_cores_hgram")
portoinst_cpu_throttled_cores_tmmv = Signal(name="portoinst-cpu_throttled_cores_tmmv")
portoinst_cpu_throttled_cores_txxx = Signal(name="portoinst-cpu_throttled_cores_txxx")
portoinst_cpu_limit_cores_tmmv = Signal(name="portoinst-cpu_limit_cores_tmmv")
portoinst_cpu_limit_cores_thhh = Signal(name="portoinst-cpu_limit_cores_thhh")
portoinst_cpu_limit_slot_cores_tmmv = Signal(name="portoinst-cpu_limit_slot_cores_tmmv")
portoinst_cpu_guarantee_cores_tmmv = Signal(name="portoinst-cpu_guarantee_cores_tmmv")
portoinst_cpu_guarantee_cores_thhh = Signal(name="portoinst-cpu_guarantee_cores_thhh")
portoinst_cpu_limit_usage_perc_hgram = Signal(name="portoinst-cpu_limit_usage_perc_hgram")
portoinst_cpu_guarantee_usage_perc_hgram = Signal(name="portoinst-cpu_guarantee_usage_perc_hgram")
portoinst_cpu_system_limit_usage_perc_hgram = Signal(name="portoinst-cpu_system_limit_usage_perc_hgram")

# porto Network
portoinst_net_limit_mb_summ = Signal(name="portoinst-net_limit_mb_summ")
portoinst_net_guarantee_mb_summ = Signal(name="portoinst-net_guarantee_mb_summ")
portoinst_net_tx_mb_summ = Signal(name="portoinst-net_tx_mb_summ")
portoinst_net_fastbone_tx_mb_summ = Signal(name="portoinst-net_fastbone_tx_mb_summ")
portoinst_net_rx_mb_summ = Signal(name="portoinst-net_rx_mb_summ")
portoinst_net_fastbone_rx_mb_summ = Signal(name="portoinst-net_fastbone_rx_mb_summ")
portoinst_net_tx_packets_summ = Signal(name="portoinst-net_tx_packets_summ")
portoinst_net_fastbone_tx_packets_summ = Signal(name="portoinst-net_fastbone_tx_packets_summ")
portoinst_net_rx_packets_summ = Signal(name="portoinst-net_rx_packets_summ")
portoinst_net_fastbone_rx_packets_summ = Signal(name="portoinst-net_fastbone_rx_packets_summ")
portoinst_net_tx_drops_summ = Signal(name="portoinst-net_tx_drops_summ")
portoinst_net_fastbone_tx_drops_summ = Signal(name="portoinst-net_fastbone_tx_drops_summ")
portoinst_net_rx_drops_summ = Signal(name="portoinst-net_rx_drops_summ")
portoinst_net_fastbone_rx_drops_summ = Signal(name="portoinst-net_fastbone_rx_drops_summ")

# porto Memory
portoinst_memory_usage_gb_tmmv = Signal(name="portoinst-memory_usage_gb_tmmv")
portoinst_memory_usage_gb_tvvv = Signal(name="portoinst-memory_usage_gb_tvvv")
portoinst_max_rss_gb_tmmv = Signal(name="portoinst-max_rss_gb_tmmv")
portoinst_max_rss_gb_tvvv = Signal(name="portoinst-max_rss_gb_tvvv")
portoinst_anon_usage_gb_tmmv = Signal(name="portoinst-anon_usage_gb_tmmv")
portoinst_anon_usage_gb_txxx = Signal(name="portoinst-anon_usage_gb_txxx")
portoinst_anon_limit_tmmv = Signal(name="portoinst-anon_limit_tmmv")
portoinst_minor_page_faults_summ = Signal(name="portoinst-minor_page_faults_summ")
portoinst_major_page_faults_summ = Signal(name="portoinst-major_page_faults_summ")
portoinst_memory_limit_gb_tmmv = Signal(name="portoinst-memory_limit_gb_tmmv")
portoinst_memory_limit_slot_gb_tmmv = Signal(name="portoinst-memory_limit_slot_gb_tmmv")
portoinst_memory_guarantee_gb_tmmv = Signal(name="portoinst-memory_guarantee_gb_tmmv")
portoinst_memory_limit_usage_perc_hgram = Signal(name="portoinst-memory_limit_usage_perc_hgram")
portoinst_memory_guarantee_usage_perc_hgram = Signal(name="portoinst-memory_guarantee_usage_perc_hgram")
portoinst_anon_limit_usage_perc_hgram = Signal(name="portoinst-anon_limit_usage_perc_hgram")
portoinst_memory_unevictable_limit_usage_perc_hgram = Signal(name="portoinst-memory_unevictable_limit_usage_perc_hgram")
portoinst_memory_unevictable_guarantee_usage_perc_hgram = Signal(name="portoinst-memory_unevictable_guarantee_usage_perc_hgram")
portoinst_ooms_summ = Signal(name="portoinst-ooms_summ")
portoinst_ooms_slot_hgram = Signal(name="portoinst-ooms_slot_hgram")

# porto IO
portoinst_io_read_bytes_tmmv = Signal(name="portoinst-io_read_bytes_tmmv")
portoinst_io_write_bytes_tmmv = Signal(name="portoinst-io_write_bytes_tmmv")
portoinst_io_ops_tmmv = Signal(name="portoinst-io_ops_tmmv")
portoinst_io_load_max = Signal(name="portoinst-io_load_max")
portoinst_io_read_fs_bytes_tmmv = Signal(name="portoinst-io_read_fs_bytes_tmmv")
portoinst_io_read_fs_megabytes_tmmv = Signal(name="div(portoinst-io_read_fs_bytes_tmmv,1048576)")
portoinst_io_write_fs_bytes_tmmv = Signal(name="portoinst-io_write_fs_bytes_tmmv")
portoinst_io_write_fs_megabytes_tmmv = Signal(name="div(portoinst-io_write_fs_bytes_tmmv,1048576)")
portoinst_io_ops_fs_tmmv = Signal(name="portoinst-io_ops_fs_tmmv")
portoinst_io_limit_bytes_tmmv = Signal(name="portoinst-io_limit_bytes_tmmv")
portoinst_io_limit_megabytes_tmmv = Signal(name="div(portoinst-io_limit_bytes_tmmv,1048576)")


# Метрики CPU
cpu_usage_cores_hgram = Signal(name="cpu-usage_cores_hgram")
cpu_usage_system_cores_hgram = Signal(name="cpu-usage_system_cores_hgram")
cpu_wait_cores_hgram = Signal(name="cpu-wait_cores_hgram")
cpu_idle_cores_hgram = Signal(name="cpu-idle_cores_hgram")
cpu_id_avg = Signal(name="cpu-id_avg")
cpu_id_hgram = Signal(name="cpu-id_hgram")
cpu_si_avg = Signal(name="cpu-si_avg")
cpu_si_hgram = Signal(name="cpu-si_hgram")
cpu_sy_avg = Signal(name="cpu-sy_avg")
cpu_sy_hgram = Signal(name="cpu-sy_hgram")
cpu_us_avg = Signal(name="cpu-us_avg")
cpu_us_hgram = Signal(name="cpu-us_hgram")
cpu_wa_avg = Signal(name="cpu-wa_avg")
cpu_wa_hgram = Signal(name="cpu-wa_hgram")


# Работа с диском
usage_space_gb_total_tmmv = Signal(name="usage_space_gb_*_tmmv")
usage_inode_total_tmmv = Signal(name="usage_inode_*_tmmv")
free_space_gb_tmmv = Signal(name="free_space_gb_tmmv")
free_space_gb_tnnv = Signal(name="free_space_gb_tnnv")
free_inode_tmmv = Signal(name="free_inode_tmmv")
free_inode_tnnv = Signal(name="free_inode_tnnv")
read_bytes_total_tmmv = Signal(name="read_bytes_*_tmmv")
write_bytes_total_tmmv = Signal(name="write_bytes_*_tmmv")
ops_total_tmmv = Signal(name="ops_*_tmmv")
load_total_hgram = Signal(name="load_*_hgram")


# Метрики HBF
hbf4_packets_drop_summ = Signal(name="hbf4-packets_drop_summ")
hbf6_packets_drop_summ = Signal(name="hbf6-packets_drop_summ")
hbf4_packets_log_summ = Signal(name="hbf4-packets_log_summ")
hbf6_packets_log_summ = Signal(name="hbf6-packets_log_summ")
hbf4_packets_output_inet_log_summ = Signal(name="hbf4-packets_output_inet_log_summ")
hbf6_packets_output_inet_log_summ = Signal(name="hbf6-packets_output_inet_log_summ")
hbf4_packets_output_log_summ = Signal(name="hbf4-packets_output_log_summ")
hbf6_packets_output_log_summ = Signal(name="hbf6-packets_output_log_summ")
hbf4_packets_output_reject_summ = Signal(name="hbf4-packets_output_reject_summ")
hbf6_packets_output_reject_summ = Signal(name="hbf6-packets_output_reject_summ")


# Информация об инстансах
instances_exec_duration = Signal(name="instances-exec_duration")
instances_unique = Signal(name="instances-unique")
instances_total = Signal(name="instances-total")
instances_tags = Signal(name="instances-tags")
instances_discarded_instances = Signal(name="instances-discarded_instances")
instances_discarded_tags = Signal(name="instances-discarded_tags")
instances_host_aggr_duration = Signal(name="instances-host_aggr_duration")
instances_counter_tmmm = Signal(name="instances-counter_tmmm")
instances_current_time_nnnn = Signal(name="instances-current-time_nnnn")
instances_update_count_tmmm = Signal(name="instances-update-count_tmmm")


# Потребление памяти
mem_total_avg = Signal(name="mem-total_avg")
mem_total_tvvv = Signal(name="mem-total_tvvv")
mem_total_thhh = Signal(name="mem_total_thhh")
mem_mem_free_avg = Signal(name="mem-mem_free_avg")
mem_mem_free_tvvv = Signal(name="mem-mem_free_tvvv")
mem_mem_free_thhh = Signal(name="mem_mem_free_thhh")
mem_cached_avg = Signal(name="mem-cached_avg")
mem_cached_tvvv = Signal(name="mem-cached_tvvv")
mem_cached_thhh = Signal(name="mem_cached_thhh")
mem_free_avg = Signal(name="mem-free_avg")
mem_free_tvvv = Signal(name="mem-free_tvvv")
mem_free_thhh = Signal(name="mem_free_thhh")
mem_used_avg = Signal(name="mem-used_avg")
mem_used_tvvv = Signal(name="mem-used_tvvv")
mem_used_thhh = Signal(name="mem_used_thhh")


# Средняя загрузка хоста
loadavg_abs_xhhh = Signal(name="loadavg-abs_xhhh")
loadavg_norm_xhhh = Signal(name="loadavg-norm_xhhh")


# Сетевые показатели
netstat = Signal(name="netstat")
netstat_tcpext_listen_overflows = Signal(name="netstat-tcpext_listen_overflows")
netstat_tcpext_listen_drops = Signal(name="netstat-tcpext_listen_drops")
netstat_tcpext_backlog_drop = Signal(name="netstat-tcpext_backlog_drop")
netstat_tcp_curr_estab = Signal(name="netstat-tcp_curr_estab")
netstat_tcp_retrans_segs = Signal(name="netstat-tcp_retrans_segs")
netstat_ibytes = Signal(name="netstat-ibytes")
netstat_ibits = Signal(name="netstat-ibits")
netstat_obytes = Signal(name="netstat-obytes")
netstat_obits = Signal(name="netstat-obits")
netstat_ierrs_total = Signal(name="netstat-ierrs*")
netstat_oerrs_total = Signal(name="netstat-oerrs*")
netstat_idrops_total = Signal(name="netstat-idrops*")
netstat_odrops_total = Signal(name="netstat-odrops*")
netstat_ipkts_total = Signal(name="netstat-ipkts*")
netstat_opkts_total = Signal(name="netstat-opkts*")
netstat_colls_total = Signal(name="netstat-colls*")
nf_conntrack_count = Signal(name="nf_conntrack_count")
netstat_nf_conntrack_count_max = Signal(name="netstat-nf_conntrack_count_max")
nf_conntrack_max = Signal(name="nf_conntrack_max")
netstat_nf_conntrack_max = Signal(name="netstat-nf_conntrack_max")

# Сетевые (IPv6) показатели
netstat6_ip6inhdrerrors_mmmm = Signal(name="netstat6-ip6inhdrerrors_mmmm")
netstat6_ip6intoobigerrors_mmmm = Signal(name="netstat6-ip6intoobigerrors_mmmm")
netstat6_ip6inaddrerrors_mmmm = Signal(name="netstat6-ip6inaddrerrors_mmmm")
netstat6_ip6indiscards_mmmm = Signal(name="netstat6-ip6indiscards_mmmm")
netstat6_ip6outdiscards_mmmm = Signal(name="netstat6-ip6outdiscards_mmmm")
netstat6_icmp6inerrors_mmmm = Signal(name="netstat6-icmp6inerrors_mmmm")
netstat6_icmp6outerrors_mmmm = Signal(name="netstat6-icmp6outerrors_mmmm")
netstat6_icmp6incsumerrors_mmmm = Signal(name="netstat6-icmp6incsumerrors_mmmm")
netstat6_udp6inerrors_mmmm = Signal(name="netstat6-udp6inerrors_mmmm")
netstat6_udp6rcvbuferrors_mmmm = Signal(name="netstat6-udp6rcvbuferrors_mmmm")
netstat6_udp6sndbuferrors_mmmm = Signal(name="netstat6-udp6sndbuferrors_mmmm")
netstat6_udp6incsumerrors_mmmm = Signal(name="netstat6-udp6incsumerrors_mmmm")
netstat6_udplite6inerrors_mmmm = Signal(name="netstat6-udplite6inerrors_mmmm")
netstat6_udplite6rcvbuferrors_mmmm = Signal(name="netstat6-udplite6rcvbuferrors_mmmm")
netstat6_udplite6sndbuferrors_mmmm = Signal(name="netstat6-udplite6sndbuferrors_mmmm")
netstat6_udplite6incsumerrors_mmmm = Signal(name="netstat6-udplite6incsumerrors_mmmm")
netstat6_ip6reasmfails_mmmm = Signal(name="netstat6-ip6reasmfails_mmmm")
netstat6_ip6fragfails_mmmm = Signal(name="netstat6-ip6fragfails_mmmm")

# vmstat
vmstat_us_xhhh = Signal(name="vmstat_us_xhhh")

# Статистика по сокетам хоста
sockstat_tcp_total_thhh = Signal(name="sockstat-tcp_*_thhh")
sockstat_unix_total_thhh = Signal(name="sockstat-unix_*_thhh")

# корки
portoinst_cores_total_hgram = Signal(name="portoinst-cores_total_hgram")
portoinst_cores_dumped_hgram = Signal(name="portoinst-cores_dumped_hgram")
