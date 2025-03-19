#!/usr/bin/perl

use strict;
use warnings;
use JSON qw(decode_json);
use POSIX qw(strftime);

my $mongodump = "/usr/bin/mongodump";
my $s3cmd = "/usr/bin/s3cmd";

my $config;
while (<>) { $config .= $_; }
$config = decode_json($config);

foreach my $job (keys %{$config}) {
    my @mdump_args;
    if (exists($config->{$job}->{"host"})) { push @mdump_args, sprintf("--host '%s'", $config->{$job}->{"host"}); }
    if (exists($config->{$job}->{"port"})) { push @mdump_args, sprintf("--port '%d'", $config->{$job}->{"port"}); }
    if (exists($config->{$job}->{"user"})) { push @mdump_args, sprintf("--username '%s'", $config->{$job}->{"user"}); }
    if (exists($config->{$job}->{"password"})) { push @mdump_args, sprintf("--password '%s'", $config->{$job}->{"password"}); }
    if (exists($config->{$job}->{"auth_db"})) { push @mdump_args, sprintf("--authenticationDatabase '%s'", $config->{$job}->{"auth_db"}); }
    if (exists($config->{$job}->{"db"})) {
        foreach (split(",", $config->{$job}->{"db"})) { push @mdump_args, sprintf("--db '%s'", $_); }
    }
    if (exists($config->{$job}->{"collection"})) {
        foreach (split(",", $config->{$job}->{"collection"})) { push @mdump_args, sprintf("--collection '%s'", $_); }
    }
    push @mdump_args, "--gzip";
    my $archive_name = sprintf("%s.%s.gz", $job, strftime("%Y-%m-%d_%H-%M-%S", localtime()));
    push @mdump_args, sprintf("--archive='%s'", $archive_name);

    my $mdump_args_str = join(" ", @mdump_args);

    chomp(my @result = `$mongodump $mdump_args_str 2>&1`);
    my $status = $? == 0 ? "OK" : "Fail";

    my (@result_s3, $status_s3);
    if (exists($config->{$job}->{"s3_bucket"})) {
        my @s3cmd_args;
        push @s3cmd_args, sprintf("put %s", $archive_name);
        push @s3cmd_args, sprintf("s3://%s/%s/backup/", $config->{$job}->{"s3_bucket"}, $job);
        my $s3cmd_args_str = join(" ", @s3cmd_args);
        chomp(my @result_s3 = `$s3cmd $s3cmd_args_str 2>&1`);
        $status_s3 = $? == 0 ? "OK" : "Fail";
        if (-f $archive_name) { unlink $archive_name; }
    }

    if (exists($config->{$job}->{"log"}) and $config->{$job}->{"log"}) {
        open my $logfile, ">>", sprintf("%s.log", $job) or die "Can't open file: $!";
        printf $logfile "mongodump %s:\n%s\n", $status, join("\n", @result);
        if (defined($status_s3)) { printf $logfile "s3cmd %s:\n%s\n", $status_s3, join("\n", @result_s3); }
        print $logfile "\n";
        close $logfile;
    }
    else {
        printf "mongodump %s:\n%s\n", $status, join("\n", @result);
        if (defined($status_s3)) { printf "s3cmd %s:\n%s\n", $status_s3, join("\n", @result_s3); }
    }
}
