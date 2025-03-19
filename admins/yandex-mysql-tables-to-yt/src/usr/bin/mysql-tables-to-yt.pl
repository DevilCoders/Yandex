#!/usr/bin/perl

use strict;
use warnings;
use feature 'say';
use File::Basename;
use JSON::PP;
use POSIX qw(strftime);
use IPC::Run qw(run);
no if $] >= 5.018, warnings => qw(experimental::smartmatch);
use Getopt::Long qw(GetOptions);
Getopt::Long::Configure qw(gnu_getopt);

my $test_config = 0;
GetOptions('test-config|t' => \$test_config,)
    or die "Usage: $0\n\t-t|--test-config\n";

my $status_filename = sprintf('/tmp/%s.log', fileparse($ARGV[0], qr/\.[^.]*/));

my $config;
while (<>) {$config .= $_;}
$config = decode_json($config);

my $global_error = 0;

sub current_date {
    return strftime("%Y.%m.%d %H:%M:%S", localtime);
}

sub check_error {
    my ($error, $msg, $path) = @_;
    my $ret_code = 0;
    if ($error) {
        printf "%s: %s\n\n", current_date(), $msg;
        $global_error = 1;
        if (-f $path) {unlink $path;}
        $ret_code = 1;
    }
    return $ret_code;
}

foreach my $job (keys %{$config->{"jobs"}}) {
    my %job_config;
    if (exists($config->{"jobs"}->{$job}->{"template"})) {
        my $template = $config->{"jobs"}->{$job}->{"template"};
        if (exists($config->{"templates"}->{$template})) {
            %job_config = %{$config->{"templates"}->{$template}};
        }
    }
    foreach my $param (keys %{$config->{"jobs"}->{$job}}) {
        $job_config{$param} = $config->{"jobs"}->{$job}->{$param};
    }

    if (exists($job_config{"yt_token"})) {
        $ENV{"YT_TOKEN"} = $job_config{"yt_token"};
    }
    my $retry = exists($job_config{"retry"}) ? $job_config{"retry"} : 1;

    foreach (qw(tables database yt_path yt_cluster)) {
        if (not exists($job_config{$_})) {
            printf "Job '%s' missing parameter '%s'\n", $job, $_;
            exit 1;
        }
    }
    if ($test_config) {next;}

    printf "%s: job '%s', DB '%s', YT cluster '%s'\n",
        current_date(), $job, $job_config{"database"}, $job_config{"yt_cluster"};

    foreach my $table (@{$job_config{"tables"}}) {
        printf "\ttable '%s'\n", $table;
        my @columns;
        my $mysql_dump_path = sprintf("/tmp/mysql_%s.sql_dump.zst", $table);
        my $error = 0;
        if (exists($job_config{"columns"})) {@columns = @{$job_config{"columns"}};}
        else {
            my ($cmd_out, $cmd_err);
            my @cmd = (
                '/usr/bin/mysql',
                '-NB',
                $job_config{"database"},
                '-e',
                sprintf(
                    "SELECT `COLUMN_NAME` FROM INFORMATION_SCHEMA.COLUMNS WHERE `TABLE_SCHEMA` = \"%s\" AND `TABLE_NAME` = \"%s\"",
                    $job_config{"database"}, $table
                )
            );
            printf "%s: get columns names from MySQL\n",
                current_date();
            my $status = run \@cmd, '>', \$cmd_out, '2>', \$cmd_err;
            if (defined($cmd_err) and ($cmd_err ne '')) {
                chomp $cmd_err;
                printf "\tCommand error: [%s]\n", $cmd_err;
            }
            if ($status) {
                $error = 0;
                foreach (split("\n", $cmd_out)) {
                    chomp;
                    push @columns, $_;
                }
            }
            else {$error = 1;}
            if (check_error(
                $error, "can't get columns names from MySQL",
                $mysql_dump_path
            )
            ) {
                next;
            }
        }
        my @sort_columns;
        if (exists($job_config{"sort_columns"})) {
            @sort_columns = @{$job_config{"sort_columns"}};
            my @columns_ordered = @sort_columns;
            foreach (@columns) {
                if ($_ ~~ @sort_columns) {next;}
                else {push @columns_ordered, $_;}
            }
            @columns = @columns_ordered;
        }
        my @columns_for_select;
        my @ifnull_columns = exists($job_config{"ifnull_columns"}) ? @{$job_config{"ifnull_columns"}} : ();
        my $ifnull_replace = exists($job_config{"ifnull_replace"}) ? $job_config{"ifnull_replace"} : "";
        foreach (@columns) {
            push(@columns_for_select,
                $_ ~~ @ifnull_columns
                    ? sprintf("IFNULL(`%s`, '%s') AS `%s`", $_, $ifnull_replace, $_)
                    : "`$_`"
            );
        }

        $error = 0;
        printf "\tcolumns '%s'\n", join(", ", @columns);
        if (@sort_columns) {printf "\tsort columns '%s'\n", join(", ", @sort_columns);}
        if (@ifnull_columns) {printf "\tIFNULL columns '%s'\n", join(", ", @ifnull_columns);}
        for (my $i = 1; $i <= $retry; $i++) {
            my $cmd_err;
            my @cmd = (
                '/usr/bin/mysql',
                '-NB',
                '--quick',
                $job_config{"database"},
                '-e',
                sprintf(
                    "SET NAMES utf8; SELECT %s FROM `%s`%s",
                    join(", ", @columns_for_select),
                    $table,
                    @sort_columns
                        ? sprintf(" ORDER BY %s",
                        join(", ", map {"`$_`"} @sort_columns))
                        : ""
                )
            );
            my $compress_err;
            my @compress_cmd = qw(/usr/bin/zstd -T0 -cq);
            printf "%s: export from MySQL [try %d/%d]\n",
                current_date(), $i, $retry;
            my $status = run \@cmd, '2>', \$cmd_err, '|', \@compress_cmd, '>', $mysql_dump_path, '2>', \$compress_err;
            if (defined($cmd_err) and ($cmd_err ne '')) {
                chomp $cmd_err;
                printf "\tCommand error: [%s]\n", $cmd_err;
            }
            if (defined($compress_err) and ($compress_err ne '')) {
                chomp $compress_err;
                printf "\tCompress command error: [%s]\n", $compress_err;
            }
            printf "%s: export from MySQL %s\n",
                current_date(),
                $status ? "done" : "failed";
            if ($status) {
                $error = 0;
                last;
            }
            else {$error = 1;}
        }
        if (check_error(
            $error, "can't export from MySQL", $mysql_dump_path
        )
        ) {
            next;
        }

        $error = 0;
        my $yt_full_path = sprintf("%s/%s",
            $job_config{"yt_path"},
            exists($job_config{"yt_table"})
                ? $job_config{"yt_table"}
                : $table);
        my $yt_full_path_tmp = $yt_full_path . "_tmp";
        printf "\tYT tmp path '%s'\n", $yt_full_path_tmp;
        my $schema;
        if (@sort_columns) {
            my @schema_arr;
            foreach (@columns) {
                if ($_ ~~ @sort_columns) {
                    push @schema_arr,
                        sprintf(
                            "{name = %s; type = int64; sort_order = \"ascending\"}",
                            $_);
                }
                else {
                    push @schema_arr,
                        sprintf("{name = %s; type = utf8}", $_);
                }
            }
            $schema = sprintf("<schema=[%s]>%s",
                join("; ", @schema_arr),
                $yt_full_path_tmp);
        }
        for (my $i = 1; $i <= $retry; $i++) {
            my ($cmd_out, $cmd_err);
            my @cmd = ('/usr/bin/yt', 'write');
            push @cmd, defined($schema) ? $schema : $yt_full_path_tmp;
            push @cmd,
                (
                    '--format',
                    sprintf(
                        "<columns=[%s];enable_type_conversion=%%true>schemaful_dsv",
                        join("; ", @columns)),
                    '--proxy',
                    $job_config{"yt_cluster"}
                );
            my $decompress_err;
            my @decompress_cmd = qw(/usr/bin/zstd -T0 -cdq);
            printf "%s: upload to YT [try %d/%d]\n",
                current_date(), $i, $retry;
            my $status = run(
                \@decompress_cmd, '<', $mysql_dump_path, '2>', \$decompress_err,
                '|',
                \@cmd, '>', \$cmd_out, '2>', \$cmd_err
            );
            if (defined($cmd_out) and ($cmd_out ne '')) {
                chomp $cmd_out;
                printf "\tCommand output: [%s]\n", $cmd_out;
            }
            if (defined($cmd_err) and ($cmd_err ne '')) {
                chomp $cmd_err;
                printf "\tCommand error: [%s]\n", $cmd_err;
            }
            if (defined($decompress_err) and ($decompress_err ne '')) {
                chomp $decompress_err;
                printf "\tDecompress command error: [%s]\n", $decompress_err;
            }
            printf "%s: upload to YT %s\n",
                current_date(),
                $status ? "done" : "failed";
            if ($status) {
                $error = 0;
                last;
            }
            else {$error = 1;}
        }
        if (check_error($error, "can't upload to YT", $mysql_dump_path)) {
            next;
        }

        {
            my ($cmd_out, $cmd_err);
            my @cmd = (
                '/usr/bin/yt', 'exists', $yt_full_path, '--proxy',
                $job_config{"yt_cluster"}
            );
            my $status = run \@cmd, '>', \$cmd_out, '2>', \$cmd_err;
            chomp $cmd_out;
            if ($status and ($cmd_out eq "true")) {
                @cmd = (
                    '/usr/bin/yt', 'remove', $yt_full_path, '--proxy',
                    $job_config{"yt_cluster"}
                );
                run \@cmd, '>', \$cmd_out, '2>', \$cmd_err;
            }
        }

        printf "\tYT path '%s'\n", $yt_full_path;
        {
            my ($cmd_out, $cmd_err);
            my @cmd = (
                '/usr/bin/yt', 'move', $yt_full_path_tmp, $yt_full_path,
                '--proxy', $job_config{"yt_cluster"}
            );
            printf "%s: move table on YT\n",
                current_date();
            my $status = run \@cmd, '>', \$cmd_out, '2>', \$cmd_err;
            if (defined($cmd_out) and ($cmd_out ne '')) {
                chomp $cmd_out;
                printf "\tCommand output: [%s]\n", $cmd_out;
            }
            if (defined($cmd_err) and ($cmd_err ne '')) {
                chomp $cmd_err;
                printf "\tCommand error: [%s]\n", $cmd_err;
            }
            printf "%s: move table on YT %s\n",
                current_date(),
                $status ? "done" : "failed";
            if ($status) {$error = 0;}
            else {$error = 1;}
            if (check_error(
                $error, "can't move table on YT",
                $mysql_dump_path
            )
            ) {
                next;
            }
        }

        print "\n";

        if (-f $mysql_dump_path) {unlink $mysql_dump_path;}
    }
}

if (open(my $fh, '>', $status_filename)) { say $fh $global_error }
else { die(sprintf('Could not open file "%s": %s', $status_filename, $!)) }
