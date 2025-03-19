# vim:ft=perl
package Ubic::Service::Elliptics::Config;

use strict;
use warnings;

use Template;
use File::Copy;
use Net::INET6Glue::INET_is_INET6;
use Socket;
use Socket6;
use LWP::UserAgent;
use JSON::XS;
use File::Path qw(make_path);
use Filesys::Df;

sub generate {

    my ($name, $c) = @_;

    master_nodes($c, $name);

    my $res = undef;
    $res = create_config($name, $c);

    return $res;
}

sub get_mounted_device {
    my $path = shift;

    open(FINDMNT, "findmnt -rn -o SOURCE $path |") || return "";
    my $dev = <FINDMNT>;
    close(FINDMNT);
    return undef if ! defined $dev;
    chomp $dev;
    return $dev;
}

sub is_rotational {
    my $devname = shift;
    return 1 if ! defined $devname;
    $devname =~ s#/dev/##; # Remove extra path if it is present
    open(FH, '<', "/sys/block/$devname/queue/rotational") || return 1; # Return that drvie is rotating by default
    my $rot = <FH>;
    chomp $rot;
    close(FH);
    return int($rot);
}

sub create_json {

    my ($name, $c) = @_;

    #
    # configure log (file type or syslog)
    #
    my $log_source = "syslog";
    if ($c->{'global'}->{'log_type'} eq "file") {
        $log_source = $c->{'global'}->{'log_dir'} . "/node-" . $name . ".log";
    } elsif ($c->{'global'}->{'log_type'} eq "none") {
        $log_source = "/dev/null";
    }

    my $flags =
        $c->{'global'}->{'enable_noroute'} == 0 || $name == '1' ? $c->{'global'}->{'flags'} : $c->{'global'}->{'flags'} | 2;
    my $port            = $c->{'global'}->{'base_port'} + $name;
    my $jconf           = ();
    my $back            = ();
    my %backends        = ();
    my @fifo_pools      = ();
    my @lifo_pools      = ();
    my $MAX_BACKENDS    = $c->{'global'}->{'max_backends'};
    my $MAX_DISKS       = $c->{'global'}->{'max_disks'};
    my $MAX_CACHE_DISKS = $c->{'global'}->{'max_cache_disks'};

    my $MAX_CONFIGURED_BACKENDS = 0;                     #number of preconfigured backends
    my $cocaine_run_path        = "/var/run/cocaine";    # this directory (used by cocaine) might have been removed on restart

    #
    # generate skeletron of dirs like 0/{0,1,2,3,4,5...}, 1/{0,1,2,3...}, 2/{0,1,2,3...}, ...
    #
    my $i = 0;    # first num of disk (mount_point)
    my $j = 0;    # first local num of backend at currect mount pount
    my $h = 0;    # first backend ID
    while ($i < $MAX_DISKS) {    #config format > mds26

        if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27') {

            while ($j < $MAX_BACKENDS) {

                #my $name = "name_${i}_${j}";
                my $name = "${i}/${j}";
                $backends{$name}{"id"}     = $h;
                $backends{$name}{"enable"} = 0;
                $backends{$name}{"path"}   = "$i/$j";
                $h++;            # backend ID
                $j++;            # local num of backend at currect mount pount
            }
            $j = 1;

        } else {    # eq 'json26'

            my $name = "${i}";
            $backends{$name}{"id"}     = $h;
            $backends{$name}{"enable"} = 0;
            $backends{$name}{"path"}   = "$i";
            $h++;    # backend ID
        }

        $i++;        # num of disks (mount_point)
    }

    #
    # add cached dirs to skeletron
    # https://st.yandex-team.ru/MDS-901

    if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27') {

        $i = 1;
        while ($i <= $MAX_CACHE_DISKS) {
            my $name = $c->{'global'}->{'cache_subdir'} . "/${i}";
            $backends{$name}{"id"}     = $h;
            $backends{$name}{"enable"} = 0;
            $backends{$name}{"path"}   = $c->{'global'}->{'cache_subdir'} . "/$i";
            $backends{$name}{"disk_type"} = "hdd";   # default for disabled backends
            $h++;    # backend ID
            $i++;    # local num of backend at currect mount pount
        }
    }

    $MAX_CONFIGURED_BACKENDS = $h;    #save number of configured backs

    my $disks = ();

    if ($c->{'global'}->{'config_format'} eq 'json26') {    # elliptics >=2.26 version, general

        my @dirs = glob($c->{'global'}->{'root_dir'} . "/*");
        my @mounted_disks =
            grep {/^[0-9]+$/}
            map {File::Spec->abs2rel($_, $c->{'global'}->{'root_dir'})} grep {-d $_ . "/" . $c->{'global'}->{'mount_flag'}} @dirs;
        foreach my $id (@mounted_disks) {

            #my $id = $d; $id =~ s/.*\///;  #strip left part of disk dir
            $disks->{$id}->{"id"}   = $id;
            $disks->{$id}->{"path"} = $id;
        }

        #
        # https://st.yandex-team.ru/MDS-296
        # multi backends under each hdd
        #
    } elsif ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27') {

        my @root_dirs     = glob($c->{'global'}->{'root_dir'} . "/*");
        my @mounted_disks = ();
        @mounted_disks = grep {/^.*\/[0-9]+$/} @root_dirs;
        foreach (@mounted_disks) {
            my $devname = get_mounted_device($_);
            my $rotational = is_rotational($devname);
            my @dirs_local = glob($_ . "/*");
            my @disks_local =
                grep {/^[0-9]+\/[0-9]+$/}
                map {File::Spec->abs2rel($_, $c->{'global'}->{'root_dir'})}
                grep {-d $_ . "/" . $c->{'global'}->{'mount_flag'}} @dirs_local;
            foreach my $id (@disks_local) {
                $disks->{$id}->{"id"}   = $id;
                $disks->{$id}->{"path"} = $id;
                $disks->{$id}->{"disk_type"} = $rotational ? "hdd" : "ssd";
            }
        }

        #
        # try to detact cached disks
        # https://st.yandex-team.ru/MDS-901
        #
        my @cache_root_dirs     = glob($c->{'global'}->{'root_dir'} . "/" . $c->{'global'}->{'cache_subdir'} . "/*");
        my @cache_mounted_disks = ();
        @cache_mounted_disks = grep {/^.*\/[0-9]+$/} @cache_root_dirs;

        my $ccc = $c->{'global'}->{'cache_subdir'};
        my @cache_disks_local =
            grep {/^$ccc\/[0-9]+$/}
            map {File::Spec->abs2rel($_, $c->{'global'}->{'root_dir'})}
            grep {-d $_ . "/" . $c->{'global'}->{'mount_flag'}} @cache_mounted_disks;

        foreach my $id (@cache_disks_local) {
            $disks->{$id}->{"id"}   = $id;
            $disks->{$id}->{"path"} = $id;
        }

    }    # end of  eq 'mds26' or 'mds27'

    foreach (keys %$disks) {    #find all valid (with kdb) disks and enable such backends

        my $id   = $disks->{$_}->{"id"};
        my $path = $disks->{$_}->{"path"};
        my $disk_type  = defined $disks->{$_}->{"disk_type"} ? $disks->{$_}->{"disk_type"} : "hdd";

        if (defined $backends{$path}) {
            $backends{$path}{"enable"} = 1;
            $backends{$path}{"disk_type"} = $disk_type;
        } else {
            die "\nERROR: invalid backend with path "
                . $c->{'global'}->{'root_dir'}
                . "/$path (with kdb dir), I am not permitted to skip it. Emergency stop.\n";
        }
    }

    foreach my $k (keys %backends) {

        my $id        = $backends{$k}{'id'};                          # backend id
        my $path      = $backends{$k}{'path'};                        # path to data(kdb)
        my $full_path = $c->{'global'}->{'root_dir'} . "/" . $path;
        my $enabled   = $backends{$k}{'enable'};                      # enabled (kdb exist) or no
        my $disk_type = $backends{$k}{'disk_type'};
        my $group     = get_group($c, $path, $enabled);
        if ($group == -1 || $group == 0) {
            $enabled = 0;
        }    #disk mounted (kdb dir exist), but no groupID file in kdb/group.id --> disable backend
        if (is_backend_stopped($id)) {$enabled = 0;}    #https://st.yandex-team.ru/MDS-3198

        my $blob_size_limit = undef;
        $blob_size_limit = get_blob_size_limit($c, $path, $enabled)
            if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');

        my $single_pass_file_size_threshold = undef;
        $single_pass_file_size_threshold = get_single_pass_file_size_threshold($c, $path, $enabled)
            if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');

        my $blob_flags = undef;
        $blob_flags = get_blob_flags($c, $path, $enabled)
            if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');

        my $pool_id = undef;
        if ($c->{'global'}->{'static_pool'} eq '1') {
            $pool_id = get_pool_id($c, $path, $enabled);
            # https://st.yandex-team.ru/ELL-838
            if ($enabled) {
                if ($pool_id =~ m/_fi/) {
                    push @fifo_pools, $pool_id;
                } else {
                    push @lifo_pools, $pool_id;
                }
            }
        }

        my $queue_timeout = undef;
        if (defined $c->{'backend'}->{'queue_timeout'}) {
            $queue_timeout = get_queue_timeout($c, $path, $enabled)
                if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');
        }

        my $cache_size = undef;
        if (   defined $c->{'global'}->{'cache_size'}
            && $c->{'global'}->{'cache_size'} ne "none"
            && $c->{'global'}->{'cache_size'} ne "per_backend")
        {
            $cache_size = get_cache_size($c, $path, $enabled)
                if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');
        } else {
            $cache_size = get_cache_size_per_backend($c, $path, $enabled)
                if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');
        }

        my $cache_base_dir = "cache";
        my $datasort_dir = $c->{'global'}->{'datasort_dir'} if (defined $c->{'global'}->{'datasort_dir'});
        my $datasort_disk_size = undef;
        if (defined $datasort_dir && $datasort_dir =~ m#^/$cache_base_dir#) {
            my @cache_stat = stat "/$cache_base_dir";
            my @root_stat  = stat "/";
            if ($cache_stat[0] == $root_stat[0]) {    # device numbers are equal, i. e. /cache is really on root FS
                undef $datasort_dir;
            } else {
                my $stats = df("/$cache_base_dir");
                if (defined($stats)) {
                    $datasort_disk_size = $stats->{blocks} * 1024; # size in bytes derived from total number of 1k blocks
                }
            }
        }
        my $datasort_path = $datasort_dir . "/" if (defined $datasort_dir);
        if (defined $datasort_dir and !-d $datasort_path and $enabled) {
            make_path($datasort_path);
        }

        #
        # save (to config) only last disabled (if it last, or enabled instead) backend
        # https://st.yandex-team.ru/MDS-1796
        #
        if (!$enabled && $id < ($MAX_CONFIGURED_BACKENDS - 1)) {
            next;
        }

        my $back = ();

        $back->{'backend_id'}         = int($id);
        $back->{'history'}            = $full_path . "/" . $c->{'global'}->{'mount_flag'};
        $back->{'datasort_dir'}       = $datasort_path if (defined $datasort_dir);
        $back->{'group'}              = $group;
        $back->{'disk_type'}          = $disk_type;
        $back->{'sync'}               = $c->{'backend'}->{'blob'}->{'sync'};
        $back->{'records_in_blob'}    = $c->{'backend'}->{'blob'}->{'records_in_blob'};
        $back->{'blob_size'}          = $c->{'backend'}->{'blob'}->{'size'};
        $back->{'blob_size_limit'}    = "$blob_size_limit" if (defined $blob_size_limit);
        $back->{'pool_id'}            = "$pool_id" if (defined $pool_id);
        $back->{'queue_timeout'}      = "$queue_timeout" if (defined $queue_timeout);
	$back->{'datasort_disk_size'} = $datasort_disk_size;

        # ELL-905
        if (defined $single_pass_file_size_threshold) {
            $back->{'single_pass_file_size_threshold'} = $single_pass_file_size_threshold;
        }


        if (defined $cache_size && $cache_size > 0) {
            $back->{'cache'}->{'size'} = int("$cache_size") if (defined $cache_size);
        }
        $back->{'data'} = $full_path . "/data";
        $back->{'type'} = $c->{'backend'}->{'type'};
        if ($enabled && is_backend_locked($id)) {    # backend is enabled and locked: https://st.yandex-team.ru/MDS-2557
            my $enabled  = 1;
            my $readonly = 1;
            $back->{'enable'}    = bless(do {\($enabled)},  'JSON::XS::Boolean');
            $back->{'read_only'} = bless(do {\($readonly)}, 'JSON::XS::Boolean');
        } else {
            $back->{'enable'} = bless(do {\($enabled)}, 'JSON::XS::Boolean');
        }
        $back->{'blob_flags'} = $blob_flags;
        $back->{'defrag_percentage'} = $c->{'backend'}->{'blob'}->{'defrag_percentage'};
        $back->{'defrag_timeout'}    = $c->{'backend'}->{'blob'}->{'defrag_timeout'};

        my $index_block_bloom_length = undef;
        $index_block_bloom_length = get_index_block_bloom_length($c, $path, $enabled)
            if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');
        $back->{'index_block_bloom_length'} = "$index_block_bloom_length" if (defined $index_block_bloom_length);

        my $index_block_size = undef;
        $index_block_size = $c->{'backend'}->{'blob'}->{'index_block_size'}
        if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'mds27');
        $back->{'index_block_size'} = "$index_block_size" if (defined $index_block_size);

        $back->{'bg_ioprio_class'}          = int($c->{'backend'}->{'bg_ioprio_class'});
        $back->{'bg_ioprio_data'}           = int($c->{'backend'}->{'bg_ioprio_data'});

        push(@{$jconf->{'backends'}}, $back);
    }

    my $autoflush = 1;
    if ($c->{'global'}->{'config_format'} eq 'mds26' || $c->{'global'}->{'config_format'} eq 'json26') {
        $jconf->{'logger'}->{'level'} = $c->{'global'}->{'log_level'};
        $jconf->{'logger'}->{'frontends'}[0]->{'formatter'}->{'type'} = "string";
        $jconf->{'logger'}->{'frontends'}[0]->{'formatter'}->{'pattern'} =
            "%(timestamp)s %(request_id)s/%(lwp)s/%(pid)s %(severity)s: %(message)s, attrs: [%(...L)s]";
        $jconf->{'logger'}->{'frontends'}[0]->{'sink'}->{'type'}               = "files";
        $jconf->{'logger'}->{'frontends'}[0]->{'sink'}->{'path'}               = $log_source;
        $jconf->{'logger'}->{'frontends'}[0]->{'sink'}->{'autoflush'}          = bless(do {\($autoflush)}, 'JSON::XS::Boolean');
        $jconf->{'logger'}->{'frontends'}[0]->{'sink'}->{'rotation'}->{'move'} = 0;
    } elsif ($c->{'global'}->{'config_format'} eq 'mds27' && $c->{'tskv'}->{'enable'} == 1) {
        $jconf->{'logger'}->{'level'} = $c->{'global'}->{'log_level'};
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'type'} = "string";
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'sevmap'} = ["DEBUG", "NOTICE", "INFO", "WARNING", "ERROR"];
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'pattern'} =
            "{timestamp:l} {trace_id:{0:default}0>16}/{thread:d}/{process} {severity}: {message}, attrs: [{...}]";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'type'}            = "asynchronous";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'factor'}          = 20;
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'overflow'}        = "wait";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'path'}  = $log_source;
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'type'}  = 'file';
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'flush'} = $c->{'global'}->{'log_flush'};

        # tskv settings
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'type'}                    = "tskv";
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'create'}->{'tskv_format'} = $c->{'tskv'}->{'tskv_format'};
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'create'}->{'type'}        = $c->{'tskv'}->{'type'};
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'mutate'}->{'unixtime_microsec_utc'}->{'strftime'} = "%s.%f";
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'mutate'}->{'unixtime_microsec_utc'}->{'gmtime'} = bless( do{\(my $daemon = 0)}, 'JSON::XS::Boolean' );
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'mutate'}->{'timestamp'}->{'strftime'} = "%Y-%m-%dT%H:%M:%S";
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'mutate'}->{'timestamp'}->{'gmtime'} = bless( do{\(my $daemon = 0)}, 'JSON::XS::Boolean' );
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'mutate'}->{'timezone'}->{'strftime'} = "%z";
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'remove'}[0] = "severity";
        $jconf->{'logger'}->{'access'}[0]->{'formatter'}->{'remove'}[1] = "message";
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'type'}     = "asynchronous";
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'factor'}   = 20;
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'overflow'} = "wait";
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'sink'}->{'path'}  = $c->{'global'}->{'log_dir'} . "/tskv.log";
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'sink'}->{'type'}  = 'file';
        $jconf->{'logger'}->{'access'}[0]->{'sinks'}[0]->{'sink'}->{'flush'} = $c->{'global'}->{'log_flush'};
    } elsif ($c->{'global'}->{'config_format'} eq 'mds27') {
        $jconf->{'logger'}->{'level'} = $c->{'global'}->{'log_level'};
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'type'} = "string";
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'sevmap'} = ["DEBUG", "NOTICE", "INFO", "WARNING", "ERROR"];
        $jconf->{'logger'}->{'core'}[0]->{'formatter'}->{'pattern'} =
            "{timestamp:l} {trace_id:{0:default}0>16}/{thread:d}/{process} {severity}: {message}, attrs: [{...}]";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'type'}           = "asynchronous";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'factor'}         = 20;
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'overflow'}       = "wait";
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'path'} = $log_source;
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'type'} = 'file';

        if (defined $c->{'global'}->{'log_rotate_type'}) {
            $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'rotate'}->{'type'} = $c->{'global'}->{'log_rotate_type'};
        }
        $jconf->{'logger'}->{'core'}[0]->{'sinks'}[0]->{'sink'}->{'flush'} = $c->{'global'}->{'log_flush'};
    }

    # https://st.yandex-team.ru/ELL-838
    use List::MoreUtils qw(uniq);
    @fifo_pools = uniq @fifo_pools;
    foreach my $pool (@fifo_pools) {
        $jconf->{'options'}->{'io_pools'}->{$pool}->{'lifo'} = bless(do {\(0 != 0)}, 'JSON::XS::Boolean');
        if (defined $c->{'backend'}->{'queue_limit'}) {
            $jconf->{'options'}->{'io_pools'}->{$pool}->{'queue_limit'} = int($c->{'backend'}->{'queue_limit'});
        }
    }

    @lifo_pools = uniq @lifo_pools;
    foreach my $pool (@lifo_pools) {
        $jconf->{'options'}->{'io_pools'}->{$pool}->{'lifo'} = bless(do {\(0 != 1)}, 'JSON::XS::Boolean');
        if (defined $c->{'backend'}->{'queue_limit'}) {
            $jconf->{'options'}->{'io_pools'}->{$pool}->{'queue_limit'} = int($c->{'backend'}->{'queue_limit'});
        }
    }

    foreach my $k (keys %{ $c->{'features'} }) {
        $jconf->{'options'}->{'features'}->{$k} = bless(do {\(int($c->{'features'}->{$k}) == 1)}, 'JSON::XS::Boolean');;
    }

    $jconf->{'options'}->{'tls'}->{'support'} = int($c->{'tls'}->{'support'});
    $jconf->{'options'}->{'tls'}->{'cert_path'} = $c->{'tls'}->{'cert_path'};
    $jconf->{'options'}->{'tls'}->{'key_path'} = $c->{'tls'}->{'key_path'};
    $jconf->{'options'}->{'tls'}->{'ca_path'} = $c->{'tls'}->{'ca_path'};
    $jconf->{'options'}->{'tls'}->{'debug_mode'} = int($c->{'tls'}->{'debug_mode'});

    $jconf->{'options'}->{'monitor'}->{'port'}              = int($port + 9000);
    $jconf->{'options'}->{'monitor'}->{'history_length'}    = 20000;
    $jconf->{'options'}->{'monitor'}->{'call_tree_timeout'} = 10;

    if ($c->{'top'}->{'enable'} == 1) {
        $jconf->{'options'}->{'monitor'}->{'top'}->{'top_length'}        = int($c->{'top'}->{'top_length'});
        $jconf->{'options'}->{'monitor'}->{'top'}->{'events_size'}       = int($c->{'top'}->{'events_size'});
        $jconf->{'options'}->{'monitor'}->{'top'}->{'period_in_seconds'} = int($c->{'top'}->{'period_in_seconds'});
    }

    #https://st.yandex-team.ru/MDS-1562
    if ($c->{'srw'}->{'enable'} == 1) {
        if (!-d $cocaine_run_path) {mkdir $cocaine_run_path;}
        $jconf->{'options'}->{'srw_config'} = $c->{'srw'}->{'srw_config'};
    }

    #https://st.yandex-team.ru/MDS-1763
    if ($c->{'handystats'}->{'enable'} == 1) {
        $jconf->{'options'}->{'handystats_config'} = $c->{'handystats'}->{'handystats_config'};
    }

    $jconf->{'options'}->{'parallel'} = bless(do {\($c->{'global'}->{'parallel'})}, 'JSON::XS::Boolean');

    if (   defined $c->{'global'}->{'cache_size'}
        && $c->{'global'}->{'cache_size'} ne "none"
        && $c->{'global'}->{'cache_size'} ne "per_backend")
    {
        $jconf->{'options'}->{'cache'}->{'size'} = int($c->{'global'}->{'cache_size'});
    }
    $jconf->{'options'}->{'check_timeout'} = int($c->{'backend'}->{'check_timeout'});
    $jconf->{'options'}->{'queue_timeout'} = $c->{'backend'}->{'queue_timeout'} if (defined $c->{'backend'}->{'queue_timeout'});
    $jconf->{'options'}->{'wait_timeout'}  = int($c->{'backend'}->{'wait_timeout'});
    my $join = int($c->{'global'}->{'join'});
    $jconf->{'options'}->{'join'}                = bless(do {\($join)}, 'JSON::XS::Boolean');
    $jconf->{'options'}->{'send_limit'}          = int($c->{'backend'}->{'send_limit'});
    $jconf->{'options'}->{'flags'}               = int($flags);
    $jconf->{'options'}->{'indexes_shard_count'} = int($c->{'backend'}->{'indexes_shard_count'});
    $jconf->{'options'}->{'daemon'}              = bless(do {\(my $daemon = 0)}, 'JSON::XS::Boolean');

    $i = 0;
    foreach my $family (@{$c->{'int'}->{'lfamily'}}) {
        push(@{$jconf->{'options'}->{'address'}}, $c->{'int'}->{'node_name'} . ":$port:" . $family . "-" . $i);
        $i++;
        if ($c->{'global'}->{'enable_fastbone'} == 1 and $family == 10) {

            # Add fastbone address if it exists (IPv6 only)
            my $fb_name = $c->{'int'}->{'node_name'};
            $fb_name =~ s/\.storage/-fb\.storage/;
            my @fb_info = gethostbyname2($fb_name, AF_INET6);
            if (@fb_info) {
                print "Found fastbone hostname $fb_name (" . inet_ntop(AF_INET6, $fb_info[4]) . ")\n";
                push(@{$jconf->{'options'}->{'address'}}, $fb_name . ":$port:" . $family . "-" . $i);
                $i++;
            }
        }
    }

    $jconf->{'options'}->{'auth_cookie'}               = $c->{'global'}->{'auth_cookie'};
    $jconf->{'options'}->{'io_thread_num'}             = int($c->{'backend'}->{'io_thread_num'});
    $jconf->{'options'}->{'nonblocking_io_thread_num'} = int($c->{'backend'}->{'nonblocking_io_thread_num'});
    $jconf->{'options'}->{'net_thread_num'}            = int($c->{'backend'}->{'net_thread_num'});

    # ELL-933
    if (defined $c->{'backend'}->{'io_pools'}->{'defaults'}->{'io_thread_num'}) {
        $jconf->{'options'}->{'io_pools'}->{'defaults'}->{'io_thread_num'} = int($c->{'backend'}->{'io_pools'}->{'defaults'}->{'io_thread_num'});
    }

    if (defined $c->{'backend'}->{'io_pools'}->{'defaults'}->{'nonblocking_io_thread_num'}) {
        $jconf->{'options'}->{'io_pools'}->{'defaults'}->{'nonblocking_io_thread_num'} = int($c->{'backend'}->{'io_pools'}->{'defaults'}->{'nonblocking_io_thread_num'});
    }

    $jconf->{'options'}->{'server_net_prio'} = int($c->{'global'}->{'server_net_prio'});
    $jconf->{'options'}->{'client_net_prio'} = int($c->{'global'}->{'client_net_prio'});

    $jconf->{'options'}->{'remote'} = $c->{"int"}->{"master_nodes"};

    return $jconf;
}

sub is_backend_locked {
    my $id = shift;

    if (-f "/etc/elliptics/parsed/$id.lock") {
        return 1;
    }
    return 0;
}

sub is_backend_stopped {
    my $id = shift;

    if (-f "/etc/elliptics/parsed/$id.stop") {
        return 1;
    }
    return 0;
}

sub create_config {

    my ($name, $c) = @_;

    my $jconf = ();
    $jconf = create_json($name, $c);

    my $enable = 1;
    my $json   = new JSON::XS;
    $json = $json->pretty([$enable]);

    my $json_text = $json->encode($jconf);

    open OUT, ">" . $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.tmp";
    print OUT $json_text;
    close OUT;
    if (-s $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.tmp") {
        move(
            $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.tmp",
            $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.parsed"
        );
    } else {
        die "parsed file " . $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.conf.tmp not exist or zero size\n";
    }

    return;
}

sub master_nodes {

    my ($c, $name) = @_;

    my $mn_file           = $c->{'global'}->{'master_nodes'};
    my $master_nodes      = ();
    my $master_nodes_full = ();

    #my $port = $c->{'global'}->{'base_port'} + $name;
    my $port = $c->{'global'}->{'base_port'} + 1;

    if ($mn_file !~ /^https?:\/\/.*/) {    # remote nodes defined in text file
        if (!-f $mn_file) {die "can't open file $mn_file, see http://elliptics.ru/node/5";}
        open IN, "<$mn_file" || die "can't open file $mn_file, see http://elliptics.ru/node/5";
        while (<IN>) {
            next if m/^ *#/;
            chomp;
            next if m/^\s*$/;
            push(@{$master_nodes}, $_);
        }
        close IN;
    } else {

        my $timeout   = 15;
        my $USERAGENT = "config-elliptics-node";
        my $request   = new URI($mn_file . "?format=json");
        my $ua        = LWP::UserAgent->new(agent => $USERAGENT);
        $ua->timeout($timeout);
        my $resp = $ua->get($request);

        my $group = $mn_file;
        $group =~ s/.*\/(.+)/$1/;
        my $cache_file = "/var/tmp/cache-config-elliptics-node-$group";
        if ($resp->status_line() =~ /^200 .*/) {

            my $json        = new JSON::XS;
            my $perl_scalar = $json->decode($resp->content());

            open OUT, ">$cache_file.tmp";
            foreach (@$perl_scalar) {
                push(@{$master_nodes}, $_->{"fqdn"});
                print OUT $_->{"fqdn"} . "\n";
            }
            close OUT;
            if (-s "$cache_file.tmp") {move("$cache_file.tmp", "$cache_file");}
        } elsif (-s $cache_file) {

            #            print "using cache $cache_file\n";
            open IN, "<$cache_file";
            foreach (<IN>) {
                chomp;
                next if m/^ *#/;
                next if m/^\s*$/;
                push(@{$master_nodes}, $_);
            }
            close IN;
        } else {
            die "can't request valid data from $mn_file and not cached data availeble in $cache_file\n";
        }
    }

    foreach my $family (@{$c->{'int'}->{'lfamily'}}) {
        foreach (@{$master_nodes}) {
            push(@{$master_nodes_full}, $_ . ":" . $port . ":" . $family);
        }
    }

    $c->{"int"}->{"master_nodes"} = $master_nodes_full;

    return;
}

sub get_group {

    my ($c, $path, $enabled) = @_;

    if (!$enabled) {return int(0);}    #for json26/mds26/mds27 always return group=0 for disabled backends

    my $group_id           = "";
    my $groupid_file_local = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "group.id";
    my $groupid_file_global = $c->{'int'}->{'conf_dir'} . "/" . "group.id";

    my $groupid_file = "";

    #
    # by default, use /etc/elliptics/group.id file
    #
    if (-e $groupid_file_global && -s $groupid_file_global > 0) {
        $groupid_file = $groupid_file_global;
    } elsif (-e $groupid_file_global && -s $groupid_file_global == 0) {
        warn "Warning: group_id file $groupid_file_global has zero size\n";
    }

    #
    # if exist local group.id in /srv/NUM/kdb/group.id, use it (override /etc/elliptics/group.id for this service NUM)
    #
    if (-e $groupid_file_local && -s $groupid_file_local > 0) {
        $groupid_file = $groupid_file_local;
    } elsif (-e $groupid_file_local && -s $groupid_file_local == 0) {
        warn "Warning: group_id file $groupid_file_local has zero size\n";
        print "Warning: group_id file $groupid_file_local has zero size\n";
    }
    if (!-f $groupid_file) {
        if ($enabled == 1) {    # elliptics 2.26, json format
                                # disk mounted (kdb dir exist), but no groupID file in kdb/group.id
                                # disable backend

#           warn "group.id file ($groupid_file) didn't found (or unreadable) at $groupid_file_local AND /etc/elliptics/group.id, disable backend, set group=-1";
            return 0;
        }
    }
    open IN, "<$groupid_file" || die "can't open file $groupid_file, see http://elliptics.ru/node/5";
    while (<IN>) {
        next if m/^ *#/;
        next if !m/[0-9]+/;
        chomp;
        $group_id = $_;
    }
    close IN;

    return int($group_id);
}

sub get_blob_size_limit {

    my ($c, $path, $enabled) = @_;

    my $size_limit = 0;
    if (!$enabled) {return 0;}    #for json26/mds26 always return size=0 for permanent disabled backends

    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if (!-e $config || -s $config == 0) {
        warn "Warning: file $config has zero size or doesn't exist\n";
        return 0;
    }

    open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-296";
    while (<IN>) {
        next if !m/^ *blob_size_limit *= *([0-9]+[bkmgtp]*)$/i;
        $size_limit = $1;
    }
    close IN;

    return $size_limit;
}

sub get_single_pass_file_size_threshold {

    my ($c, $path, $enabled) = @_;

    my $single_pass_file_size_threshold = undef;
    if (!$enabled) {return 0;}    #for json26/mds26 always return size=0 for permanent disabled backends

    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if (!-e $config || -s $config == 0) {
        warn "Warning: file $config has zero size or doesn't exist\n";
        return 0;
    }

    open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-296";
    while (<IN>) {
        next if !m/^ *single_pass_file_size_threshold *= *([0-9]+[bkmgtp]*)$/i;
        $single_pass_file_size_threshold = $1;
    }
    close IN;

    if ( ! defined $single_pass_file_size_threshold) {
        if (defined $c->{'backend'}->{'blob'}->{'single_pass_file_size_threshold'}) {
            $single_pass_file_size_threshold = $c->{'backend'}->{'blob'}->{'single_pass_file_size_threshold'};
        }
    }

    return $single_pass_file_size_threshold;
}


sub get_blob_flags {

    my ($c, $path, $enabled) = @_;

    my $blob_flags = undef;
    if (!$enabled) {return 0;}    #for json26/mds26 always return size=0 for permanent disabled backends

    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if (!-e $config || -s $config == 0) {
        warn "Warning: file $config has zero size or doesn't exist\n";
        return 0;
    }

    open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-296";
    while (<IN>) {
        next if !m/^ *blob_flags *= *([0-9]+)$/i;
        $blob_flags = $1;
    }
    close IN;

    if ( ! defined $blob_flags) {
        $blob_flags = $c->{'backend'}->{'blob'}->{'flags'};
    }

    return $blob_flags;
}

sub get_pool_id {

    my ($c, $path, $enabled) = @_;

    my $pool_id = 1;
    my $config  = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    my @pool_path = split('/', $path);
    if ($pool_path[0] =~ m/[0-9]+/) {
        $pool_id = $pool_path[0];
    }

    if ($enabled) {

        if (!-e $config || -s $config == 0) {
            warn "Warning: file $config has zero size or doesn't exist\n";
            return 0;
        }

        my $lifo = 1;
        if (defined $c->{'backend'}->{'lifo'}) {
           $lifo = $c->{'backend'}->{'lifo'};
        }

        open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-296";
        while (<IN>) {
            next if !m/^ *lifo *= *((0|1))$/i;
            $lifo = $1;
        }
        close IN;

        if ($lifo == 0) {
            $pool_id = "${pool_id}_fi"
        }
    }

    return $pool_id;
}

sub get_cache_size {

    my ($c, $path, $enabled) = @_;

    my $cache_size = 100;
    if (!$enabled) {return 100;}    #for json26/mds26 always return size=0 for disabled backends

    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if (!-e $config || -s $config == 0) {
        warn "Warning: file $config has zero size or doesn't exist\n";
        return 0;
    }

    open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-1295";
    while (<IN>) {
        next if !m/^ *cache_size *= *([0-9]+[bkmgtp]*)$/i;
        $cache_size = $1;
    }
    close IN;

    return $cache_size;
}

sub get_cache_size_per_backend {

    my ($c, $path, $enabled) = @_;
    if (!$enabled) {return 100;}    #for json26/mds26 always return size=0 for disabled backends

    my $config     = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";
    my $cache_size = undef;

    if (!-e $config || -s $config == 0) {
        warn "Warning: file $config has zero size or doesn't exist\n";
        return 0;
    }

    open IN, "<$config" || die "can't open file $config, see https://st.yandex-team.ru/MDS-1295";
    while (<IN>) {
        next if !m/^ *cache_size *= *([0-9]+[bkmgtp]*)$/i;
        $cache_size = $1;
    }
    close IN;

    return $cache_size;
}

sub get_queue_timeout {

    my ($c, $path, $enabled) = @_;
    my $queue_timeout = $c->{'backend'}->{'queue_timeout'} if (defined $c->{'backend'}->{'queue_timeout'});
    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if ($enabled) {

        if (!-e $config || -s $config == 0) {
            warn "Warning: file $config has zero size or doesn't exist\n";
            return 0;
        }

        open IN, "<$config" || die "can't open file $config";
        while (<IN>) {
            next if !m/^ *queue_timeout *= *([0-9]+(s|ms|))$/i;
            $queue_timeout = $1;
        }
        close IN;
    }
    return $queue_timeout;
}

sub get_index_block_bloom_length {

    my ($c, $path, $enabled) = @_;
    my $index_block_bloom_length = undef;
    $index_block_bloom_length = $c->{'backend'}->{'blob'}->{'index_block_bloom_length'}
        if (defined $c->{'backend'}->{'blob'}->{'index_block_bloom_length'});
    my $config = $c->{'global'}->{'root_dir'} . "/" . $path . "/" . $c->{'global'}->{'mount_flag'} . "/" . "device.conf";

    if ($enabled) {

        if (!-e $config || -s $config == 0) {
            warn "Warning: file $config has zero size or doesn't exist\n";
            return 0;
        }

        open IN, "<$config" || die "can't open file $config";
        while (<IN>) {
            next if !m/^ *index_block_bloom_length *= *([0-9]+)$/i;
            $index_block_bloom_length = $1;
        }
        close IN;
    }
    return $index_block_bloom_length;
}

1;
