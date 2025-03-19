#!/usr/bin/perl
#
# check processes for overdrafts
#
# $Id: watchdog.pl,v 1.10 2007/10/31 08:43:07 andozer Exp $
#

use strict;
use warnings;

if (system("which detect_vms.sh > /dev/null 2>&1") == 0) {
    if (system("detect_vms.sh watchdog") == 0) {
        print("0;Check disabled by excluded_vms.conf\n");
        exit 0;
    }
}

my $new_cfg = [];

my $me = $0;
$me =~ s#^.*/([^.]+)\.[^/]*$#$1#o;

my $cf  = "/etc/monitoring/$me.conf";
my %def = (
    'time_warn'  => 120,
    'time_crit'  => 600,
    'cpu_warn'   => 60,
    'cpu_crit'   => 90,
    'state_crit' => 'R',
);
my %var = (
    'gzip'  => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
    'bzip2' => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
    'cpio'  => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
);

if ( -s $cf and open( CONF, "<$cf" ) ) {
    while ( my $l = <CONF> ) {
        my $orig_l = $l;

        next if $l =~ /^\s*(#.*)*$/o;    # skip comments and empty lines

        # remove trailing spaces and cr/lf
        $l =~ s/[\s\r\n]*$//goi;

        if ( $l =~ /^(\S+)\s*=\s*([^#]*)/o ) {
            my ( $key, $val ) = ( $1, $2 );

            if ( $val =~ /^'(.*)'$/o ) {
                $val = $2;
            }
            elsif ( $val =~ /^"(.*)"$/o ) {
                $val = $2;
            }
            elsif ( $val =~ /=>/o ) {
                $var{$key} = { $val =~ /(\S+)\s*=>\s*([^,\s]+)/go };
                next;
            }
            elsif ( $val =~ /,/o ) {
                $var{$key} = [ split( /,\s/, $val ) ];
                next;
            }

            $var{$key} = $val;
        }
        else {
            my $action = 'ignore';

#	FIXME: Remove Yandex::Tools::read_file_option
#     if ($l =~ /^pidfile:(.+)/) {
#       # by pidfile
#       my $pidfile = $1;
#
#        if ($l =~ /.*(\s+warning)/) {
#          $action = 'warning';
#        }
#        $l =~ s/\s+warning\s*$//goi;
#        $pidfile =~ s/\s+warning\s*$//goi;
#
#        if (-e $pidfile && -f $pidfile) {
#          my $pid = Yandex::Tools::read_file_option($pidfile);
#          if (defined($pid)) {
#            push (@{$new_cfg}, {'action' => $action, 'pid' => $pid, 'source_line' => $orig_l});
#          }
#        }
#      }
#      elsif ($l =~ /\/(.+)\//) {
# "/blah/ [ignore|warning]" regexp processing
            if ( $l =~ /^\/(.+)\/(\s+(.*))?$/ ) {
                my $rx = $1;
                $action = $3 if $3;

                my $rx_compiled = eval { qr/$rx/s };
                if ($@) {

                    # debug print: error compiling regexp: $@
                }
                else {
                    push(
                        @{$new_cfg},
                        {
                            'action'      => $action,
                            'rx_compiled' => $rx_compiled,
                            'source_line' => $orig_l
                        }
                    );
                }
            }
            else {

                # by exact name
                if ( $l =~ /(.*?)(\s+(.*))?$/ ) {
                    $action = $3 if ($3);
                }

                push(
                    @{$new_cfg},
                    {
                        'action'      => $action,
                        'name'        => $l,
                        'source_line' => $orig_l
                    }
                );
            }
        }
    }

    close(CONF);
}

#use Data::Dumper;
#print Data::Dumper::Dumper($new_cfg);

my ( @warn, @crit );

open( PS, "ps auwwx | tail -n +2 |" ) || die "cannot execute: $!\n";

PS_OUTPUT_LINE: while (<PS>) {
    my ( $owner, $pid, $cpu, $state, $start, $time, $rest, $cmd, $args ) =
/^(\S+)\s+(\d+)\s+(\d+)[,.]?\d?\s+\d+[,.]\d\s+\d+\s+\d+\s+\S+\s+(\S)\S*\s+(\S+)\s+(\d+):(\S+)\s+(\S+)\s*(.*)$/o;

    next PS_OUTPUT_LINE if ( $cmd =~ /idle/o || $cmd =~ /swi\d/o );

    my $force_warning;

  RULES: foreach my $a ( @{$new_cfg} ) {
        my $match;

        if ( $a->{'name'} ) {
            print "$cmd and $a->{'name'}\n";
            if ( $cmd eq $a->{'name'} ) {
                $match = 1;
            }
        }
        elsif ( $a->{'rx_compiled'} ) {
            my $rxc = $a->{'rx_compiled'};
            if ( $cmd =~ /$rxc/ ) {
                $match = 1;
            }
        }
        elsif ( defined( $a->{'pid'} ) ) {
            if ( $pid eq $a->{'pid'} ) {
                $match = 1;
            }
        }

        if ($match) {
            if ( $a->{'action'} eq 'ignore' ) {
                next PS_OUTPUT_LINE;
            }
            elsif ( $a->{'action'} eq 'warning' ) {
                $force_warning = 1;
                last RULES;
            }
        }
    }

    my ( $cpu_crit, $time_crit, $state_crit, $cpu_warn, $time_warn );

    if ( !defined $var{$cmd} && $cmd =~ m,^/.*/([^/]*)$, ) {
        $cmd = $1;
    }
    if ( defined $var{$cmd} ) {
        $cpu_crit =
          ( defined $var{$cmd}->{'cpu_crit'} )
          ? $var{$cmd}->{'cpu_crit'}
          : $def{"cpu_crit"};
        $cpu_warn =
          ( defined $var{$cmd}->{'cpu_warn'} )
          ? $var{$cmd}->{'cpu_warn'}
          : $def{"cpu_warn"};
        $state_crit =
          ( defined $var{$cmd}->{'state_crit'} )
          ? $var{$cmd}->{'state_crit'}
          : $def{"state_crit"};
        $time_crit =
          ( defined $var{$cmd}->{'time_crit'} )
          ? $var{$cmd}->{'time_crit'}
          : $def{"time_crit"};
        $time_warn =
          ( defined $var{$cmd}->{'time_warn'} )
          ? $var{$cmd}->{'time_warn'}
          : $def{"time_warn"};
    }
    else {
        ( $cpu_crit, $time_crit, $state_crit, $cpu_warn, $time_warn ) = (
            $def{"cpu_crit"}, $def{"time_crit"}, $def{"state_crit"},
            $def{"cpu_warn"}, $def{"time_warn"}
        );
    }

    if ( $cpu >= $cpu_crit && $time >= $time_crit && $state eq $state_crit ) {
        if ( !$force_warning ) {
            push( @crit,
                "$cmd\[$pid\]: $state, cpu $cpu, spent $time:$rest since $start"
            );
        }
        else {
            push( @warn,
                "$cmd\[$pid\]: $state, cpu $cpu, spent $time:$rest since $start"
            );
        }
        next;
    }

    if ( $cpu >= $cpu_warn && $time >= $time_warn && $state eq $state_crit ) {
        push( @warn,
            "$cmd\[$pid\]: $state, cpu $cpu, spent $time:$rest since $start" );
    }
}

my $out = "PASSIVE-CHECK:$me";

if ( $#crit != -1 ) {
    print "$out;2;", join( ", ", @crit ), "\n";
}
elsif ( $#warn != -1 ) {
    print "$out;1;", join( ", ", @warn ), "\n";
}
else {
    print "$out;0;OK\n";
}

