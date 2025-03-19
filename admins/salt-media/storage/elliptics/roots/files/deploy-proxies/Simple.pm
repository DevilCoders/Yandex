# vim:ft=perl
package Ubic::Service::Elliptics::Simple;

use strict;
use warnings;

use Ubic::Daemon qw(:all);
use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use parent qw(Ubic::Service::SimpleDaemon);

use Params::Validate qw(:all);
use vars qw($params);

use Config::General;
use Sys::Hostname;

use Ubic::Service::Shared::Ulimits;
use Ubic::Service::Shared::Dirs;

use Ubic::Service::Elliptics::Config;
use JSON::XS;
use LWP::UserAgent;
use LWP::Protocol::http;

use Data::Dumper;

# Fix 'ubic status' in case when previous service modifies implementor
LWP::Protocol::implementor( http => "LWP::Protocol::http");

my $conf_dir   = "/etc/elliptics";
my $parsed_dir = $conf_dir . "/parsed";

my $conf_file = $conf_dir . "/elliptics.conf";
if (!-f $conf_file) {$conf_file = $conf_dir . "/elliptics-default.conf";}

my $conf = new Config::General($conf_file);
my $c    = $conf->{'DefaultConfig'};

my $pidfile_base = $c->{'global'}->{'pidfile_dir'} . "/elliptics-";

$c->{'global'}->{'log_level'}     = "warning" unless defined $c->{'global'}->{'log_level'};
$c->{'global'}->{'access_tskv'}   = "0"       unless defined $c->{'global'}->{'access_tskv'};
$c->{'global'}->{'config_format'} = 'json26'  unless defined $c->{'global'}->{'config_format'};
$c->{'global'}->{'max_backends'}  = 10        unless defined $c->{'global'}->{'max_backends'};

# max backends per disk ( 1 backend ~= 930Gb --> 10 backends is ample for 10Tb disks )
$c->{'global'}->{'max_disks'}       = 60 unless defined $c->{'global'}->{'max_disks'};
$c->{'global'}->{'max_cache_disks'} = 5  unless defined $c->{'global'}->{'max_cache_disks'};

# max hardware disks/cache disks at server storage
$c->{'global'}->{'user'}          = 'root'                                       unless defined $c->{'global'}->{'user'};
$c->{'global'}->{'LD_PRELOAD'}    = '/usr/lib/x86_64-linux-gnu/libjemalloc.so.1' unless defined $c->{'global'}->{'LD_PRELOAD'};
$c->{'global'}->{'MALLOC_CONF'}   = 'stats_print:true'                           unless defined $c->{'global'}->{'MALLOC_CONF'};
$c->{'global'}->{'listen_family'} = '2'                                          unless defined $c->{'global'}->{'listen_family'};
$c->{'global'}->{'enable_fastbone'} = 0    unless defined $c->{'global'}->{'enable_fastbone'};
$c->{'global'}->{'base_port'}       = 1024 unless defined $c->{'global'}->{'base_port'};

$c->{'global'}->{'root_dir'}       = "/srv"                 unless defined $c->{'global'}->{'root_dir'};
$c->{'global'}->{'cache_subdir'}   = "cache"                unless defined $c->{'global'}->{'cache_subdir'};
$c->{'global'}->{'mount_flag'}     = "kdb"                  unless defined $c->{'global'}->{'mount_flag'};
$c->{'global'}->{'auth_cookie'}    = 'default'              unless defined $c->{'global'}->{'auth_cookie'};
$c->{'global'}->{'cache_size'}     = '102400'               unless defined $c->{'global'}->{'cache_size'};
$c->{'global'}->{'static_pool'}    = '0'                    unless defined $c->{'global'}->{'static_pool'};
$c->{'global'}->{'parallel'}       = 1                      unless defined $c->{'global'}->{'parallel'};
$c->{'global'}->{'join'}           = 1                      unless defined $c->{'global'}->{'join'};
$c->{'global'}->{'enable_noroute'} = 0                      unless defined $c->{'global'}->{'enable_noroute'};
$c->{'global'}->{'master_nodes'}   = "$conf_dir/masters.id" unless defined $c->{'global'}->{'master_nodes'};

# Negative of this value will be written to oom_score_adj of the started process
# Default = 1000 disables OOM killer for this process
$c->{'global'}->{'oom_adj_value'} = 1000 unless defined $c->{'global'}->{'oom_adj_value'};

$c->{'global'}->{'server_net_prio'} = 1 unless defined $c->{'global'}->{'server_net_prio'};
$c->{'global'}->{'client_net_prio'} = 6 unless defined $c->{'global'}->{'client_net_prio'};

$c->{'global'}->{'log_type'}                   = 'file' unless defined $c->{'global'}->{'log_type'};
$c->{'global'}->{'log_flush'}                  = 1      unless defined $c->{'global'}->{'log_flush'};
$c->{'backend'}->{'io_thread_num'}             = 100    unless defined $c->{'backend'}->{'io_thread_num'};
$c->{'backend'}->{'net_thread_num'}            = 100    unless defined $c->{'backend'}->{'net_thread_num'};
$c->{'backend'}->{'nonblocking_io_thread_num'} = 25     unless defined $c->{'backend'}->{'nonblocking_io_thread_num'};
$c->{'backend'}->{'iterate_thread_num'}        = 2      unless defined $c->{'backend'}->{'iterate_thread_num'};
$c->{'backend'}->{'indexes_shard_count'}       = 16     unless defined $c->{'backend'}->{'indexes_shard_count'};
$c->{'backend'}->{'type'}                      = "blob" unless defined $c->{'backend'}->{'type'};

$c->{'backend'}->{'send_limit'} = 1500000 unless defined $c->{'backend'}->{'send_limit'};

$c->{'backend'}->{'bg_ioprio_class'} = 3 unless defined $c->{'backend'}->{'bg_ioprio_class'};
$c->{'backend'}->{'bg_ioprio_data'}  = 0 unless defined $c->{'backend'}->{'bg_ioprio_data'};

$c->{'extend_check'}->{'enabled'}        = 0  unless defined $c->{'extend_check'}->{'enabled'};
$c->{'extend_check'}->{'wait_timeout'}   = 40 unless defined $c->{'extend_check'}->{'wait_timeout'};
$c->{'extend_check'}->{'do_stack_trace'} = 0  unless defined $c->{'extend_check'}->{'do_stack_trace'};

$c->{'top'}->{'enable'}            = 0        unless defined $c->{'top'}->{'enable'};
$c->{'top'}->{'top_length'}        = 50       unless defined $c->{'top'}->{'top_length'};
$c->{'top'}->{'events_size'}       = 10485760 unless defined $c->{'top'}->{'events_size'};
$c->{'top'}->{'period_in_seconds'} = 120      unless defined $c->{'top'}->{'period_in_seconds'};

#https://st.yandex-team.ru/MDS-1562
$c->{'srw'}->{'enable'} = 0 unless defined $c->{'srw'}->{'enable'};

#https://st.yandex-team.ru/MDS-1763
$c->{'handystats'}->{'enable'} = 0 unless defined $c->{'handystats'}->{'enable'};

$c->{'tskv'}->{'enable'} = 0 unless defined $c->{'tskv'}->{'enable'};

$c->{'features'} = {} unless defined $c->{'features'};

$c->{'tls'}->{'support'} = 0 unless defined $c->{'tls'}->{'support'};
$c->{'tls'}->{'cert_path'} = "" unless defined $c->{'tls'}->{'cert_path'};
$c->{'tls'}->{'key_path'} = "" unless defined $c->{'tls'}->{'key_path'};
$c->{'tls'}->{'ca_path'} = "" unless defined $c->{'tls'}->{'ca_path'};
$c->{'tls'}->{'debug_mode'} = 0 unless defined $c->{'tls'}->{'debug_mode'};

if (   defined $c->{'global'}->{'log_type'}
    && ($c->{'global'}->{'log_type'} ne "file" && $c->{'global'}->{'log_type'} ne "syslog")
    && $c->{'global'}->{'log_type'} ne "none")
{
    die("log_type must be undef or 'file||syslog||none'");
}

if (!defined $c->{'backend'}->{'blob'}->{'size'}) {
    if (defined $c->{'backend'}->{'blob_size'}) {    #compatibility with old format
        $c->{'backend'}->{'blob'}->{'size'} = $c->{'backend'}->{'blob_size'};
    } else {
        $c->{'backend'}->{'blob'}->{'size'} = "10G";
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'records_in_blob'}) {
    if (defined $c->{'backend'}->{'records_in_blob'}) {    #compatibility with old format
        $c->{'backend'}->{'blob'}->{'records_in_blob'} = $c->{'backend'}->{'records_in_blob'};
    } else {
        $c->{'backend'}->{'blob'}->{'records_in_blob'} = 5000000;
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'flags'}) {
    if (defined $c->{'backend'}->{'blob_flags'}) {         #compatibility with old format
        $c->{'backend'}->{'blob'}->{'flags'} = $c->{'backend'}->{'blob_flags'};
    } else {
        $c->{'backend'}->{'blob'}->{'flags'} = 2;
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'sync'}) {
    if (defined $c->{'backend'}->{'sync'}) {               #compatibility with old format
        $c->{'backend'}->{'blob'}->{'sync'} = $c->{'backend'}->{'sync'};
    } else {
        $c->{'backend'}->{'blob'}->{'sync'} = 30;
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'cache_size'}) {
    if (defined $c->{'backend'}->{'blob_cache_size'}) {    #compatibility with old format
        $c->{'backend'}->{'blob'}->{'cache_size'} = $c->{'backend'}->{'blob_cache_size'};
    } else {
        $c->{'backend'}->{'blob'}->{'cache_size'} = 0;
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'defrag_timeout'}) {
    if (defined $c->{'backend'}->{'defrag_timeout'}) {     #compatibility with old format
        $c->{'backend'}->{'blob'}->{'defrag_timeout'} = $c->{'backend'}->{'defrag_timeout'};
    } else {
        $c->{'backend'}->{'blob'}->{'defrag_timeout'} = 3600;
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'index_block_bloom_length'}) {
    if (defined $c->{'backend'}->{'index_block_bloom_length'}) {    #compatibility with old format
        $c->{'backend'}->{'blob'}->{'index_block_bloom_length'} = $c->{'backend'}->{'index_block_bloom_length'};
    }
}

if (!defined $c->{'backend'}->{'blob'}->{'defrag_percentage'}) {
    if (defined $c->{'backend'}->{'defrag_percentage'}) {           #compatibility with old format
        $c->{'backend'}->{'blob'}->{'defrag_percentage'} = $c->{'backend'}->{'defrag_percentage'};
    } else {
        $c->{'backend'}->{'blob'}->{'defrag_percentage'} = 25;
    }
}

$c->{'int'}->{'parsed_dir'}   = $parsed_dir;
$c->{'int'}->{'conf_dir'}     = $conf_dir;
$c->{'int'}->{'pidfile_base'} = $pidfile_base;
$c->{'int'}->{'node_name'}    = hostname();

$c->{'global'}->{'listen_family'} =~ s/ +//g;                       #remove spaces
@{$c->{'int'}->{'lfamily'}} = split(",", $c->{'global'}->{'listen_family'});

sub new {
    my $class = shift;

    $params = validate(
        @_,
        {
            #bin => { type => SCALAR, optional => 1 },
            #        user => { type => SCALAR, default => "root", optional => 1 },
            reload_signal => {type => SCALAR, default => "HUP", optional => 1},

            #        timeout => { type => SCALAR, default => 44, optional => 1 },
        }
    );

    my $name    = 1;
    my $pidfile = $c->{'int'}->{'pidfile_base'} . $name . ".pid";

    $c->{'int'}->{'backend_variables'} = "";

    if ($c->{'backend'}->{'type'} eq "blob") {
        $c->{'int'}->{'backend_variables'} = "
blob_size = $c->{'backend'}->{'blob'}->{'size'}
records_in_blob = $c->{'backend'}->{'blob'}->{'records_in_blob'}
blob_flags = $c->{'backend'}->{'blob'}->{'flags'}
blob_cache_size = $c->{'backend'}->{'blob'}->{'cache_size'}
defrag_timeout = $c->{'backend'}->{'blob'}->{'defrag_timeout'}
defrag_percentage = $c->{'backend'}->{'blob'}->{'defrag_percentage'}
sync = $c->{'backend'}->{'blob'}->{'sync'}
data = $c->{'global'}->{'root_dir'}/$name/data
";
    }

    my $ulimits = Ubic::Service::Shared::Ulimits::construct_ulimits($c->{'global'});

    #    my $kdb_dir = $c->{'global'}->{'root_dir'}."/".$name."/".$c->{'global'}->{'mount_flag'};
    #                #if (! -d $kdb_dir) {
    #                #       mkdir $kdb_dir,0755;
    #                #       chown($owner_info[2],-1,$kdb_dir);
    #                #} #do not try to create kdb dir (this is manual only flag)

    my $self = $class->SUPER::new(
        {
            bin           => "/usr/bin/dnet_ioserv -c " . $c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.parsed",
            user          => 'root',
            ulimit        => $ulimits || {},
            daemon_user   => $c->{'global'}->{'user'},
            ubic_log      => $c->{'global'}->{'log_dir'} . "/ubic-node-$name.log",
            stdout        => $c->{'global'}->{'log_dir'} . "/ubic-stdout-$name.log",
            stderr        => $c->{'global'}->{'log_dir'} . "/ubic-stderr-$name.log",
            auto_start    => 1,
            reload_signal => $params->{reload_signal},
            env           => {
                LD_PRELOAD => $c->{'global'}->{'LD_PRELOAD'},
                MALLOC_CONF => $c->{'global'}->{'MALLOC_CONF'}
            },
        },
    );

    $self->{'c'}       = $c;
    $self->{'name_id'} = $name;

    return $self;

}

sub start_impl {
    my $self = shift;

    my $c    = $self->{'c'};
    my $name = $self->{'name_id'};

    Ubic::Service::Shared::Dirs::directory_checker($c->{'global'}->{'root_dir'}, $c->{'global'}->{'user'});
    Ubic::Service::Shared::Dirs::directory_checker($c->{'int'}->{'parsed_dir'});
    Ubic::Service::Shared::Dirs::directory_checker($c->{'global'}->{'log_dir'}, $c->{'global'}->{'user'});

    my $res = Ubic::Service::Elliptics::Config::generate($name, $c);
    if (defined $res) {die $res;}

    #check permissions on root dir on specific dnet-node
    Ubic::Service::Shared::Dirs::directory_checker($c->{'global'}->{'root_dir'} . "/$name", $c->{'global'}->{'user'});

    $self->SUPER::start_impl(@_);
}

sub status_impl {
    my $self   = shift;
    my $status = $self->SUPER::status_impl();

    return $status if $status->status ne 'running';

    my $oom_adj_value = int($c->{'global'}->{'oom_adj_value'});
    if ($oom_adj_value > 0) {
        my $pid_state = Ubic::Daemon::PidState->new($self->pidfile);
        if (!$pid_state->is_empty) {
            my $piddata         = $pid_state->read;
            my $dnet_ioserv_pid = $piddata->{daemon};
            if (open(my $OOM_ADJ, ">", "/proc/$dnet_ioserv_pid/oom_score_adj")) {
                print $OOM_ADJ "-$oom_adj_value";
                close($OOM_ADJ);
            } else {
                print STDERR "Unable to adjust pid $dnet_ioserv_pid oom_score";
            }
        }
    }

    my $c    = $self->{'c'};
    my $name = $self->{'name_id'};
    my $message = "running";

    if ($c->{'extend_check'}->{'enabled'} == 1) {
        my $port = $c->{'global'}->{'base_port'} + $name + 9000;
        # It's better to use /config handle here, but it's not present in older versions of elliptics
        my $url = "http://localhost:$port/procfs";
        my $ua = LWP::UserAgent->new();
        $ua->timeout(${c}->{'extend_check'}->{'wait_timeout'});
        my $failure = 0;
        my $resp;

        for my $try ( 1..3 ){
            $resp = $ua->get($url);
            last if $resp->is_success;
        }
        if (! $resp->is_success) {
            $message="broken for instance $name (ext-check: URL=$url): status ".$resp->status_line."\n";
            $failure = 1;
            goto get_trace;
        }
        my $json = JSON::XS->new();
        my $stats = decode_json($resp->decoded_content);
        my $ts = $stats->{"timestamp"}{"tv_sec"};
        if (! defined $ts) {
            $message = "broken: timestamp undefined for instance $name (ext-checl: URL=$url\n";
            $failure = 1;
            goto get_trace;
        }
        my $cur_time = time();
        if ($cur_time - $ts >= 3600) {
            $message = "broken: timestamp differs too much: $cur_time (localtime) vs $ts (stats timestamp)\n";
            $failure = 1;
            goto get_trace;
        }
    get_trace:
        if ($failure && $c->{'extend_check'}->{'do_stack_trace'} == 1) {
            #
            # Writing trace file before restart
            # https://st.yandex-team.ru/MDS-8
            #
            my $confname  = "elliptics-node-" . $name . ".parsed";
            my $pid_state = Ubic::Daemon::PidState->new($self->pidfile);
            if (!$pid_state->is_empty) {
                my $piddata         = $pid_state->read;
                my $dnet_ioserv_pid = $piddata->{daemon};
                if (defined $dnet_ioserv_pid && -x "/usr/bin/gdb") {
                    my $trace_file = "/var/tmp/dnet_ioserv_node-" . $name . ".trace." . time();

                    #print "Writing trace to file".$trace_file;
                    system(   '(gdb -ex "set pagination 0" -ex "thread apply all bt" --batch -p '
                            . $dnet_ioserv_pid . ') &> '
                            . $trace_file);
                }
            }
        }
    }
    return $message;
}

sub stop_impl {
    my $self = shift;
    my $c    = $self->{'c'};
    my $name = $self->{'name_id'};

    Ubic::Service::Shared::Dirs::directory_checker($c->{'int'}->{'parsed_dir'});
    stop_daemon($self->pidfile, {timeout => $c->{'global'}->{'stop_timeout'}});
    my $js = Ubic::Service::Elliptics::Config::create_json($name, $c);
    foreach my $back (@{$js->{'backends'}}) {
        my $f = $back->{'data'} . ".lock";
        if (-f $f && !-s $f) {    # .lock file exist and it size is zero
                                  #print "removed $f \n";
            unlink($f);           # https://st.yandex-team.ru/MDS-537
        }
    }
    unlink($c->{'int'}->{'parsed_dir'} . "/elliptics-node-$name.parsed");
}

sub reload {
    my $self = shift;
    unless (defined $self->{reload_signal}) {
        return result('unknown', 'not implemented');
    }
    my $daemon = check_daemon($self->pidfile);
    unless ($daemon) {
        return result('not running');
    }

    my $c    = $self->{'c'};
    my $name = $self->{'name_id'};
    my $res  = Ubic::Service::Elliptics::Config::generate($name, $c);
    if (defined $res) {die $res;}

    my $pid = $daemon->pid;
    kill HUP => $pid;

    my $guardian_pid = $daemon->guardian_pid;
    kill HUP => $guardian_pid;

    #return result('reloaded', "sent $self->{reload_signal} to $pid, sent HUP to $guardian_pid");
    return result('reloaded', "sent HUP to $pid, sent HUP to $guardian_pid");
}

sub timeout_options {
    my $self = shift;

    my $c = $self->{'c'};

    {start => {step => $c->{'global'}->{'start_timeout_step'}, trials => $c->{'global'}->{'start_timeout_try'}}};
}

sub pid {
    my $self      = shift;
    my $pid_state = check_daemon($self->pidfile);
    unless ($pid_state) {
        die "not running";
    }
    return $pid_state->pid;
}

sub custom_commands {
    return qw/ info backend /;
}

sub do_custom_command {
    my ($self, $command) = @_;

    my $c    = $self->{'c'};
    my $name = $self->{'name_id'};

    if ($command eq 'info') {
        print "\n\n";
        foreach my $family (@{$c->{'int'}->{'lfamily'}}) {
            my $port = $c->{'global'}->{'base_port'} + $name;
            my $cmd =
                  "dnet_ioclient -w "
                . $c->{'extend_check'}->{'wait_timeout'} . " -r "
                . $c->{'int'}->{'node_name'}
                . ":$port:"
                . $family
                . " -z -l 2>/dev/null";
            $cmd = "/usr/bin/elliptics-node-info.py";
            my @res = exec($cmd);
            foreach (@res) {
                print;
            }
        }
    } elsif ($command eq 'backend') {

        #	print Dumper(@_);
    } else {
        $self->SUPER::do_custom_command($command);
    }
}

1;
