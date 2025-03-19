#!/usr/bin/perl

#apt-get install libproc-daemon-perl

#use v5.10;
use strict;
use warnings;
use LWP::Simple;
use JSON qw( decode_json );
use Data::Dumper;
use Term::ANSIColor;
use Sys::Hostname;
use Config::General;
use Digest::MD5 qw(md5);

#use Data::Dumper;


sub read_config ( $ ) {

my $config_file = shift;
my $conf = Config::General->new( -ConfigFile => $config_file, );
my %config = $conf->getall;
return %config;

}

sub md5sum ( $ ) { #http://stackoverflow.com/questions/1037783/how-can-i-calculate-the-md5-hash-of-a-wav-file-in-perl
my $config_file = shift;
my $hash;

    local $/ = undef;
    open FILE, "$config_file";
    binmode FILE;
    my $data = <FILE>;
    close FILE;
    $hash = md5($data);

return $hash;
}

#print Dumper(%config);

#foreach ( sort keys %{$config{'slb'}} ) {
#print "slb $_ \n";
#close_or_not('cxfront05d7.yandex.ru',$_);
#        }
#Определим все slb, которые обслуживает этот хост.

sub get_slb_for_host ( $ ) {
    my $host = shift;
    my %slb_name;
    my $slb =
        "http://slbloghandler.yandex.net/jsrpc.php?mode=api&action=apply_filters&rs=$host";
    my $json = get($slb) or die "Could not get $slb - $! !";
    my $decoded_json = decode_json($json);
    foreach ( @{ $decoded_json->{'vsg'} } ) {

#        print LOG localtime(time) . $_->{'name'} . "\n";
        $slb_name{ $_->{'name'} } = 'true';
    }
    return %slb_name;
}

sub close_or_not ( $$$$ ) {

    my $host_long       = shift;
    my $slb             = shift;
    my $allow_close     = shift;
    my $allow_close_per = shift;

    #http://wiki.yandex-team.ru/noc/monitoring/slbloghandler#api
    my $slb_real_status =
        "http://slbloghandler.yandex.net/jsrpc.php?mode=api&action=rs_status&vsg=$slb";

    my $json = get($slb_real_status)
        or print LOG "Could not get $slb_real_status - $! !";
    my $decoded_json = decode_json($json);

    print DEBUG Dumper($decoded_json);

    my %status    = ();
    my %balancers = ();
    my $server_dc = '';
    my %servers   = ();
	my %banned_server = ();

=begin
            'dc' => "\x{418}\x{432}\x{430}\x{43d}\x{442}\x{435}\x{435}\x{432}\x{43a}\x{430}-\x{411}",
            'status' => [
                          {
                            'last_change' => '2013-10-10 17:48:39',
                            'alive' => '1',
                            'port' => '81'
                          },
                          {
                            'last_change' => '2013-10-10 17:48:43',
                            'alive' => '1',
                            'port' => '444'
                          }
                        ],
            'balancer' => 'ptah',
            'ip' => '84.201.156.198',
            'name' => 'cxfront06e6.yandex.ru',
            'local_ip' => '10.103.122.198'
          },
=cut

    foreach ( @{$decoded_json} ) {
        if ( $_->{'name'} eq $host_long ) {
            $server_dc = $_->{'dc'};
        }

        $balancers{ $_->{'dc'} }{'alive'} = 0
            if not $balancers{ $_->{'dc'} }{'alive'};
 
#определяем сколько портов слушает сервер
        my $ports = scalar @{ $_->{'status'} } . "\n";
        
#и запихиваем эти данные хеш
        for my $i ( 0 .. ( $ports - 1 ) ) {

            #			$server_dc{'name'}{$_->{'status'}->[$i]{'port'}} = 'port';
            my $port = $_->{'status'}->[$i]{'port'} . "\n";
            $servers{ $_->{'name'} }{$port} = $slb;
        }
#зачем нам выполнять код, если мы сразу знаем, что хост закрыт от балансера?
        for my $i ( 0 .. ( $ports - 1 ) ) {
	if ( $_->{'name'} eq $host_long ) {
	if ( $_->{'status'}->[$i]{'alive'} == 0 ) {
		print LOG localtime(time) . " $host_long already banned on balancer\n";
		print LOG localtime(time) . " $_->{'status'}->[$i]{'alive'}\n";
		print LOG localtime(time) . " $_->{'name'}\n";
		$banned_server{$host_long}{ $_->{'status'}->[$i]{'port'} . "\n" } = $slb;
		next;
#            $servers{ $_->{'name'} }{1} = 'already_banned';
#            return %servers;
	}
	}
	}
#Определена ли вообще переменная статус для хоста? Нет? Тогда не паримся.
        if ( not $_->{'status'}->[0]{'alive'} ) {
            $status{ $_->{'dc'} }{ $_->{'name'} } = 0;
            $balancers{ $_->{'dc'} }{'dead'}             += 1;
            $balancers{ $_->{'dc'} }{'status_not_found'} += 1;
            next;
        }

#Считаем сколько живых, сколько мертвых хостов.
#TODO - тут надо проверять не 1 порт, а все.
        if ( $_->{'status'}->[0]{'alive'} ) {
            $status{ $_->{'dc'} }{ $_->{'name'} } =
                $_->{'status'}->[0]{'alive'};
            $balancers{ $_->{'dc'} }{'alive'} += 1;
        }
        elsif ( $_->{'status'}->[0]{'alive'} eq 0 ) {
            $status{ $_->{'dc'} }{ $_->{'name'} } = 0;
            $balancers{ $_->{'dc'} }{'dead'} += 1;
        }
    }
# Если у наш хост забанен - то выходим из функции и возвращаем его порты.
	return %{$banned_server{ $host_long }} if scalar %banned_server;

    my $all_real = 0;
    for my $key ( sort keys %status ) {
        if ( $balancers{$key}{'dead'} ) {
            $all_real = $balancers{$key}{'alive'} + $balancers{$key}{'dead'};
            if ( ( $all_real * $allow_close_per / 100 )
                > $balancers{$key}{'dead'} )
            {
#если укладываемся в заданные лимиты - то ок, закрываем, чо.
                $allow_close = 1;
            }
        }
        else {
            $all_real = $balancers{$key}{'alive'};

#если у нас нет ни одной закрытой машинки, то чего нам париться, закрываем конечно!
            $allow_close = 1;
        }
        print "In dc $key alive "
            . $balancers{$key}{'alive'}
            . " from $all_real servers\n"
            if our $DEBUG;
        if ($DEBUG) {
            for my $name ( sort keys %{ $status{$key} } ) {
                print colored( "$name is dead\n", 'bold red' )
                    if $status{$key}{$name} eq 0;
            }
        }
        if ( $key eq $server_dc and $allow_close ) {
            return %{$servers{$host_long}};
        }
        if ( not $allow_close and $key eq $server_dc ) {

        print LOG "DON'T TOUCH! ( dead servers = $balancers{$key}{'dead'} ) \n";
		%servers = ();
            return %servers;
        }
        $all_real    = 0;
        $allow_close = 0;
    }

}

sub daemonize {
    use POSIX;
    POSIX::setsid or die "setsid: $!";
    my $pid = fork();
    if ( $pid < 0 ) {
	print LOG "fork: $!\n";
        die "fork: $!";
    }
    elsif ($pid) {
        exit 0;
    }
    chdir "/";
    umask 0;
    foreach ( 0 .. ( POSIX::sysconf(&POSIX::_SC_OPEN_MAX) || 1024 ) ) {
        POSIX::close $_;
    }
    open( STDIN,  "</dev/null" );
    open( STDOUT, ">/dev/null" );
    open( STDERR, ">&STDOUT" );
    return $$;
}

my $pid = daemonize;
#my $host = 'mail-extract05g.mail.yandex.net';

#отрываем лог-файлы
open( LOG, "+>>", "/tmp/check_monitoring_and_close_host.log" );
select LOG;
$| = 1;

open( DEBUG, "+>>", "/tmp/debug.log" );
select DEBUG;
$| = 1;

my $continue = 1;

my $config_file = '/root/check_real.conf';

#$SIG{TERM} = sub { $continue = 0 };

#use Regexp::IPv6;

our $DEBUG = 0;

my $host_is_close = 0;

my @bad_checks;

#Считаем, что 75% машин в каждом ДЦ должны справиться.
our $allow_close_per = 25;
our $allow_close     = 0;
binmode( STDOUT, ":encoding(utf8)" );
our $host = hostname;

my %config = read_config($config_file);

my $md5_config_file = md5sum($config_file);

while ($continue) {
#$continue = 0;
sleep 4;
my $md5_config_file_new = md5sum($config_file);
if ( $md5_config_file_new ne $md5_config_file ) {
	print LOG localtime(time) . " config file was changed, reload him\n";
	%config = read_config($config_file);
	$md5_config_file = $md5_config_file_new;
}
#root@cxfront10y:~# monrun -w -f nagios
#Type: other
#        PASSIVE-CHECK:500-http-nginx;1;Ok,logfile /var/log/nginx/access.log doesn't exist
#                PASSIVE-CHECK:conductor_interfaces;2; NOT actual
#

    my @monrun_outputs = qx'monrun -w -f nagios | grep "PASSIVE-CHECK"';
	@bad_checks = ();
    if ( scalar @monrun_outputs ) {
        foreach my $monrun_check ( sort keys %{ $config{'monrun_checks'} } ) {
            foreach my $monrun_output (@monrun_outputs) {
                if ( $monrun_output =~ m/$monrun_check\;2/ ) {
			push @bad_checks,$monrun_output;
                    print LOG localtime(time) . "$monrun_output";
			my @check_iptable_rules = qx'/sbin/iptables -L -n | egrep "(REJECT|DROP).*10.0.0.0"';
			if ( scalar @check_iptable_rules ) {
			$host_is_close = 1;
			print LOG localtime(time) . " $host already closed from balancers\n";
			last;
			}
			else {
			$host_is_close = 0;
			}
			
                    if ( not $host_is_close ) {
			
                        print LOG localtime(time)
                            . " can i close host from balancer?\n";
                        my %slb  = get_slb_for_host($host);
                        my %port = ();
                        foreach ( sort keys %slb ) {
                            my %result =
                                close_or_not( $host, $_, $allow_close,
                                $allow_close_per );
#				print LOG Dumper(%result);
                            if ( scalar %result) {
				print DEBUG Dumper(%result);
                                foreach ( sort keys %result ) {
                                    $port{$_} = '1';
                                }
                            }
                            else
                            { #если машинку закрывать нельзя
                                $port{'0'} = '1';
                            }
                        }

                        if ( not $port{'0'} ) {
				print LOG localtime(time) . " I'm Commander Shepard and this is my favorite ports on balancers\n";
                            print LOG localtime(time) . " " . $_ foreach keys %port;
                            print LOG localtime(time) . " You can close $host\n";
                            print LOG localtime(time) . " Yes,commander!\n";
				my $ports = '';
				foreach my $x ( sort keys %port ) {
					chomp $x;
                            print LOG localtime(time) . " /sbin/iptables -I INPUT 1 -s 10.0.0.0/8 -d 10.0.0.0/8 -p tcp -m tcp --dport $x -j REJECT --reject-with icmp-port-unreachable\n";
                            qx|/sbin/iptables -I INPUT 1 -s 10.0.0.0/8 -d 10.0.0.0/8 -p tcp -m tcp --dport $x -j REJECT --reject-with icmp-port-unreachable|;
				}
                            $host_is_close = 1;
                        }
			else
				{
				print LOG localtime(time) . " can`t close $host\n";
			}
                    }
			last;
                }
#		else
#		{
#	print LOG localtime(time) . " All ok\n";
#        print LOG localtime(time) . "$monrun_output";
#        if ($host_is_close) {
#            print LOG localtime(time) . " open $host\n";
#            $host_is_close = 0;
#        }
#			
#		}
            }
        }
    }
    else {
	print LOG localtime(time) . " All ok\n";
        if ($host_is_close) {
            print LOG localtime(time) . " open $host\n";
	    while ( 1 ) {
		system qw|/sbin/iptables -D INPUT 1|;
		if ( $? != 0 ) {
			print LOG localtime(time) . " flushed all iptables rules with number = 1\n";
			last;
		}
	    }
            $host_is_close = 0;
        }
	}

	if ( not scalar @bad_checks ) {
		print LOG localtime(time) . " On $host all is ok\n";
        if ($host_is_close) {
            print LOG localtime(time) . " open $host\n";
	    while ( 1 ) {
		system qw|/sbin/iptables -D INPUT 1|;
		if ( $? != 0 ) {
			print LOG localtime(time) . " flushed all iptables rules with number = 1\n";
			last;
		}
	    }
            $host_is_close = 0;
        }
	}

    }

=todo
1. Проверять раздельно ipv4 && ipv6 балансер ( pdd.yandex.ru имеет на реалах ipv6, но не слушает их. Проверять что хост имеет ipv6 адрес ).
2. apt-get install libregexp-ipv6-perl
=cut
