#! /usr/bin/perl

use strict;

$0 =~ m~/([^/]+?)\.[^/]+$~; #/
my $ME		= $1;
my $BASE	= "/";
my $CONF	= "$BASE/etc/$ME.conf";
my $PREV	= "$BASE/tmp/$ME.prev";

# defaults
our $socket	= '/mysqld/mysqld.sock';
our $user	= 'root';
our $password	= '';
our $behind_warn= 100;
our $behind_err = 10000;
our $no_io_pos_check = 1;
our $behind_lag_file = undef;

# read config
if (-f $CONF) {
	do ($CONF);
}

# check the server
my $prev = `cat $PREV 2>/dev/null`;
chomp $prev;

$password="-p'$password'" if ($password);
my @result = `mysql -u'$user' $password -S '$socket' -BEe 'SHOW SLAVE STATUS' 2>&1`;

if ((scalar @result == 0) or ((scalar @result == 1) and ($result[0] eq "Warning: Using a password on the command line interface can be insecure.\n"))) {
	print ("PASSIVE-CHECK:$ME;0;Not a MySQL replication slave\n");
	exit (0);
}

my $io_running	= 0;
my $sql_running	= 0;
my $behind	= 0;
my $read_pos	= 0;
my $error	= 0;

foreach (@result) {

	$io_running = 1 if (/Slave_IO_Running: Yes/);
	$sql_running = 1 if (/Slave_SQL_Running: Yes/);
	$behind = $1 if (/Seconds_Behind_Master: (\d+)/);
	$read_pos = $1 if (/Read_Master_Log_Pos: (\d+)/);
	$error = 1 if (/ERROR/);

}

open (F, ">$PREV");
print F "$read_pos\n";
close (F);

my @error 	= ();
my @warning	= ();

push (@error, 'Could not connect to MySQL') if ($error);

push (@error, 'IO down') unless ($io_running);
push (@error, 'SQL down') unless ($sql_running);

push (@warning, "$behind seconds behind") if ($behind > $behind_warn);
push (@error, "$behind seconds behind") if ($behind > $behind_err);

# Write behind lag to $behind_lag_file file
if (defined $behind_lag_file) {
  open (my $fh, ">$behind_lag_file");
  print $fh "$behind\n";
  close $fh;
}

if ($no_io_pos_check eq 0) {
  push (@error, 'IO position is stale') if ($prev == $read_pos);
}

if (scalar @error) {
	print ("PASSIVE-CHECK:$ME;2;" . join (',', @error) . "\n");
	exit (0);
}

if (scalar @warning) {
	print ("PASSIVE-CHECK:$ME;1;" . join (',', @warning) . "\n");
	exit (0);
}

print ("PASSIVE-CHECK:$ME;0;$behind seconds behind\n");
