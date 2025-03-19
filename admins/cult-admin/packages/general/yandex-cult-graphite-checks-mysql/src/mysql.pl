#!/usr/bin/perl -w

use strict;
use Storable;

my $tmp_file = '/tmp/mysql_graphite.tmp';
my %hash;
my %old_hash = eval { %{retrieve($tmp_file)} };
my $old_time = $old_hash{'timestamp'} || -1;
my $time = time();

if (open(MYSQL, "/usr/bin/mysql --defaults-file=/root/.my.cnf -e 'SHOW GLOBAL STATUS' |")) {
    foreach my $stat (<MYSQL>) {
	chomp($stat);
	next unless ($stat);
	my $key;
	my $param;
	my $value;
	#counters
	if (($param,$value) = $stat =~ m/^Com_(.*)\s(.*)$/si){
	    $key="mysql_commands.".$param;
	} elsif (($param,$value) = $stat =~ m/^Handler_(.*)\s(.*)$/si){
	    $key="mysql_handler.".$param;
	} elsif (($param,$value) = $stat =~ m/^Bytes_(.*)\s(.*)$/si){
	    $key="mysql_octets.".$param;
	} elsif (($param,$value) = $stat =~ m/^Table_locks_(.*)\s(.*)$/si){
	    $key="mysql_locks.".$param;
	} elsif (($value) = $stat =~ m/^Slow_queries(.*)$/si){
	    $key="slow_queries";
	} elsif (($param,$value) = $stat =~ m/^Select_(.*)\s(.*)$/si){
	    $key="select_types.".$param;
	} elsif (($value) = $stat =~ m/^Questions(.*)$/si){
	    $key="questions";
	}else {
	    #gauge here
	    if (($param,$value) = $stat =~ m/^Qcache_(.*)\s(.*)$/si){
		next unless $value>0;
		$key="cache_result.".$param;
		print "$key $value\n";
	    }
	    if (($param,$value) = $stat =~ m/^Threads_(.*)\s(.*)$/si){
		next unless $value>0;
		$key="mysql_threads.".$param;
		print "$key $value\n";
	    }
	    next;
	}
	next unless $value>0;
	
	
	$hash{$key} = $value;
	if ($old_hash{$key} && $old_time > 0) {
	    $value -= $old_hash{$key};
	    my $time_diff = $time - $old_time;
	    $time_diff = 1 if ($time_diff==0);
	    $value = int($value/$time_diff);
	    print "$key $value\n";
	} else {
	    print "$key 0\n";
	}
    }
    $hash{'timestamp'}=$time;
    store \%hash, $tmp_file;
    
} else {
    warn("Error (mysql) $?: \"$!\"");
}

