#!/usr/bin/perl -w
use Time::Local;
use POSIX qw(strftime);

my $config="/home/monitor/agents/etc/postfix.conf";
my $postfix_init="/etc/init.d/postfix";
my $status=0;
my $msg="";

# set limits 
my %limits;
if ( -f $config and open(CONF,"<",$config)) {
	while (<CONF>) {
		next if ($_=~/^\s*#/);
		if ($_=~/\s*(\S+)\s*=\s*(\S+)\s*/) {
			$limits{$1}=$2;
		}
	}
	close(CONF);
} else {
	# set default limits
	%limits=( "queue_size_warn" => 10,
		  "queue_size_crit" => 20,
		  "msg_ttl_warn" => 600,
		  "msg_ttl_crit" => 7200,
	  );
}

# check if postfix is running
my $psgrep=`ps ax | grep postfix | grep master | grep -v grep`;
chomp($psgrep) if $psgrep;
unless ($psgrep) {
	mexit(2,"postfix is not running");
}

# get queue from mailq and check for old messages and queue size
if ( -x "/usr/bin/mailq" ) {
	my $q_size=0;
	my $old_msg_c=0;
	my $old_msg_w=0;
	my $mailq=`/usr/bin/mailq`;
	my $curr_time=time();
	foreach my $str (split(/\n/,$mailq)) {
		next unless ($str=~/^[A-Z0-9]+/);
		if ($str=~/^[A-Z0-9]+\s+\d+\s+(\w+\s+\w+\s+\d+\s+\d+:\d+:\d+)\s+.*/) {
			$q_size++;
			my $msg_time=convert_time($1);
			if (($curr_time - $msg_time) > $limits{"msg_ttl_crit"}) {
				$old_msg_c++;
			} elsif (($curr_time - $msg_time) > $limits{"msg_ttl_warn"}) {
				$old_msg_w++;
			}
		} 
	}
	if ($q_size > $limits{"queue_size_crit"}) {
		mexit(2,"$q_size messages in queue");
	} elsif ($q_size > $limits{"queue_size_warn"}) {
		$status=1 unless ($status==2);
		$msg.="$q_size messages in queue";
	}
	if ($old_msg_c) {
		mexit(2,"$old_msg_c old messages in queue,");
	} elsif ($old_msg_w) {
		$status=1 unless ($status==2);
		$msg.="$old_msg_w old messages in queue,";
	}
} else {
	mexit(2,"mailq not found or not runable");
}

# check if hostname in /etc/mailname is correct
my $hostname=`hostname -f`;
chomp($hostname);
if (-f "/etc/mailname") {
	my $mailname=`cat /etc/mailname`;
	chomp($mailname);
	unless ($mailname eq $hostname) {
		mexit(2,"/etc/mailname is not correct");
	}
}

$msg="OK" unless ($msg);
mexit($status,$msg);

sub mexit {
  my ($status,$msg) = @_;
  $msg=substr($msg,0,1024);
  print "PASSIVE-CHECK:postfix;$status;$msg\n";
  exit(0);
}

sub convert_time{
	my $time=shift;
	if ($time=~/(\w+\s+\w+\s+\d+\s+\d+:\d+:\d+)/) {
	    	my %monthes=("Jan" => 0, "Feb" => 1,"Mar" => 2, "Apr" => 3, "May" => 4, "Jun" => 5, "Jul" => 6, "Aug" => 7, "Sep" => 8, "Oct" => 9, "Nov" => 10, "Dec" => 11);
	    	my ($mon,$mday,$hour,$min,$sec) = $time =~/\w+\s+(\w+)\s+(\d+)\s+(\d+):(\d+):(\d+)/;
		my $year=strftime("%Y",localtime());
	    	if (exists($monthes{$mon})) {
	      		$mon=$monthes{$mon};
	    	} else {
	      		return 0;
	    	}
	    	$task_time=timelocal($sec,$min,$hour,$mday,$mon,$year) or return 0;
		return $task_time;
  	} else {
    		return 0;
  	}
}

