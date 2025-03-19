#!/usr/bin/perl

use strict;
use warnings;


=head1 NAME

pkgver.pl - compare local package versions with conductor

=cut

use Net::INET6Glue::INET_is_INET6;
use LWP::Simple;
use Getopt::Long 2.33;
use Sort::Versions;
use Sys::Hostname::Long;

my $host = `hostname -f`;
chomp $host;

my $release = 'ubuntu';
$release = 'rhel' if ( -e "/etc/redhat-release" );

my $actual;
my $install;
my $yes;
my $noyes;
my $forceyes;
my $checkonly;
my $skip;

my @skip_files = ('/etc/yandex-pkgver-ignore-file');
my $skip_dir = '/etc/yandex-pkgver-ignore.d';

GetOptions(
    'a|actual' => \$actual, # non-cached api (mnemonic: actual data)
    'h|host:s' => \$host,
    'i|install!' => \$install,
    'y|yes!' => \$yes,
    'n|noy!' => \$noyes,
    'forceyes|godmode!' => \$forceyes,
    'c|checkonly!' =>  \$checkonly,
    's|skip:s' => \$skip,
) or pod2usage(2);
pod2usage(2) if @ARGV;

sub conductor_tags_on_host {
    my $hostname = hostname_long;
    my $url = "http://c.yandex-team.ru/api-cached/get_host_tags/$hostname";

    if ( $install || $actual ) {
        my $url = "http://c.yandex-team.ru/api/get_host_tags/$hostname";
    }

    my $content = get $url; # or die "Can't get url for detect conductor tags on host: $!\n";
    if ( $content and $content =~ m/pkgver_new_is_ok/i ) {
        return 1;
    } else {
        return 0;
    }
}

sub conductor_pkg_list {
    my $url = "http://c.yandex-team.ru/api-cached/packages_on_host/$host";;

    if ( $install || $actual ) {
        $url = "http://c.yandex-team.ru/api/packages_on_host/$host";
    }

    my $content = `curl -s $url`;
    die "Couldn't get $url" unless defined $content;

    my $pkg_list = {};

    my @pkg = split /\n/, $content;
    for my $i (@pkg) {
        my ($pkg, $is_upgrade, $ver, $tags) = split /\s+/, $i;
        if ( $release eq 'rhel' ) {
            $pkg = rhel_replace($pkg);
            $ver = rhel_replace($ver);
        }
        if ( defined $pkg ) {
            $pkg_list->{$pkg} = { version => $ver, upgrade => $is_upgrade, tags => $tags };
        }
    }
    return $pkg_list;
}

sub local_pkg_list {
    my $dpkg_output = '';
    if ( $release eq 'rhel' ) {
        # Для некоторых пакетов ломается вывод, поэтому такой хак написан
        # (| grep -vP '^installed|^\@'|awk '{if ( NF == 1 ) print \$NF,getline,\$1; else print \$1,1,\$2}' | awk '{print \$1,\$3}')
        # yandex-environment-testing.x86_64 1.0-1.el6
        # yandex-lib-autodetect-environment.noarch
        # 1.0-22 @mail
        # yandex-mail-monrun-mstat.noarch 0.4-0
        # yandex-mail-monrun-zookeeper.noarch
        # 0.2-0 @mail
        # yandex-mail-shepherd.noarch 150422.messiah-123.f4f52a5

        $dpkg_output = `yum list installed | awk '{print \$1,\$2}' | grep -vP '^installed|^\@'|awk '{if ( NF == 1 ) print \$NF,getline,\$1; else print \$1,1,\$2}' | awk '{print \$1,\$3}'`;
    } else {
        $dpkg_output = `dpkg-query -W -f='\${Package} \${Version} \${Status}\n' 2> /dev/null`;
    }
    my $pkg_list = {};
    for my $line (split /\n/, $dpkg_output) {
        my ($package, $version, $status) = split /\s+/, $line, 3;
        if ( $release eq 'rhel' ) {
            $package = rhel_replace($package);
            $version = rhel_replace($version);
            $status = 'install ok installed';
        }
        $pkg_list->{$package} = { version => $version, status => $status };
    }
    return $pkg_list;
}

sub rhel_replace {
    my $a = $_[0];
    $a =~ s/(.noarch|.x86_64|-0$|-1$|.el6)//g;
    return $a
}

my $pkgver_new_is_ok = conductor_tags_on_host();

my $conductor_pkg_list = conductor_pkg_list();
my $local_pkg_list = local_pkg_list();
my $pkg_cmd = '';
my $pkg_repo_only_cmd = '';
my $found_bad_packages = 0;
my @skip_packages;
@skip_packages = split /,/, $skip if $skip;

if ( -d "$skip_dir" ) {
    opendir(SKIP_DIR, "$skip_dir");

    while ($_ = readdir(SKIP_DIR)) {
        if ( -f "$skip_dir/$_" ) {
            push(@skip_files, "$skip_dir/$_");
        }
    }

    close(SKIP_DIR);
}

for (@skip_files) {
    if ( -f "$_" && -r "$_" ) {
        open(SKIP_FILE, "$_");

        while (my $row = <SKIP_FILE>) {
            chomp $row;
            push(@skip_packages, $row);
        }

        close(SKIP_FILE);
    }
}

for my $package (keys %$conductor_pkg_list) {
    my $skip_flag = 0;
    foreach my $skip_package ( @skip_packages ) {
        if ( $skip_package eq $package ) {
            $skip_flag = 1;
      }
    }
    if ( $skip_flag == 1 ) {
        next;
    }
    my $conductor = $conductor_pkg_list->{$package};
    my $local = $local_pkg_list->{$package};

    my $ver = $conductor->{version};
    my $tags = $conductor->{tags};
    my $cur_ver = '';

    if ( $local and $local->{status} eq 'install ok installed' ) {
        $cur_ver = $local->{version};
    }

    my $compare_result = versioncmp($cur_ver, $ver);

    if ( $compare_result != 0 ) {
        $found_bad_packages = 1;
        $found_bad_packages = 0 if ( $compare_result == 1 and $pkgver_new_is_ok == 1 );

        if ( $cur_ver eq '' ) {
            $cur_ver = "(none)";
        }

        if ( $install and $found_bad_packages == 1 ) {
            if (
                $conductor->{upgrade} eq 'false' #package MUST installed and currect version != conductor
                or $cur_ver ne '(none)' #package may be upgraded and now exist at host
            ) {
                $pkg_cmd .= "$package=$ver " if ( $release ne 'rhel' );
                $pkg_cmd .= "$package-$ver " if ( $release eq 'rhel' );
                if ( $tags =~ /(^|,)repo($|,)/ ) {
                    $pkg_repo_only_cmd .= "$package=$ver " if ( $release ne 'rhel' );
                    $pkg_repo_only_cmd .= "$package-$ver " if ( $release eq 'rhel' );
                }
            }
        }

        if ( $conductor->{upgrade} eq 'false' and $found_bad_packages == 1 ) {
            print "$package, Installed: $cur_ver, Must be: $ver\n";
        } elsif ( $cur_ver ne '(none)' and $found_bad_packages == 1 ) {
            print "$package, Installed: $cur_ver, Must be UPGRADED: $ver\n";
        }
    }
}

if ( $found_bad_packages == 1 and ! $install ) {
        exit 1;
}

if ( $install and $pkg_cmd ) {
    my $cmd_repo_only;
    my $cmd;

    if ( $release eq 'rhel' ) {
        $cmd_repo_only = "yum -y install $pkg_repo_only_cmd";
        $cmd_repo_only = "yum install $pkg_repo_only_cmd" if $noyes;
        $cmd_repo_only = "yum install -y $pkg_repo_only_cmd" if $forceyes;
        $cmd_repo_only = "yum  install $pkg_repo_only_cmd" if $checkonly;

        print "\n$cmd_repo_only \n" if ( $pkg_repo_only_cmd ne '' );
        $cmd = "yum -y install $pkg_cmd";
        $cmd = "yum install $pkg_cmd" if $noyes;
        $cmd = "yum install -y $pkg_cmd" if $forceyes;
        $cmd = "yum install $pkg_cmd" if $checkonly;

    } else {
        $cmd_repo_only = "apt-get update >/dev/null ; apt-get -y install $pkg_repo_only_cmd";
        $cmd_repo_only = "apt-get update >/dev/null ; apt-get install $pkg_repo_only_cmd" if $noyes;
        $cmd_repo_only = "apt-get update >/dev/null ; apt-get install -y --force-yes $pkg_repo_only_cmd" if $forceyes;
        $cmd_repo_only = "apt-get update >/dev/null ; apt-get -s install $pkg_repo_only_cmd" if $checkonly;

        print "\n$cmd_repo_only \n" if ( $pkg_repo_only_cmd ne '' );
        $cmd = "apt-get update >/dev/null && apt-get -y install $pkg_cmd";
        $cmd = "apt-get update >/dev/null && apt-get install $pkg_cmd" if $noyes;
        $cmd = "apt-get update >/dev/null && apt-get install -y --force-yes $pkg_cmd" if $forceyes;
        $cmd = "apt-get update >/dev/null && apt-get install -s $pkg_cmd" if $checkonly;
    }

    print "\n$cmd\n";

    if ( $yes ) {
        print "\n Option -y already defined, so go on...\n";
        print "\n Option -c defined, so check only...\n" if $checkonly;

        print "\n executing: $cmd_repo_only ...\n" if ( $pkg_repo_only_cmd ne '' );
        print "\n executing: $cmd ...\n\n";

        system($cmd_repo_only) if ( $pkg_repo_only_cmd ne '' );
        system($cmd) == 0 or die "apt-get returned an error, check manually";
    } else {
        print "\n Are you sure to install missing packages? [y/n]";
        $|++;                   # autoflush STDOUT

        my $answer = <STDIN>;
        chomp($answer);
        $answer=uc($answer);
        if ( $answer eq "Y") {
            print "\n executing: $cmd_repo_only ...\n" if ( $pkg_repo_only_cmd ne '' );
            print "\n executing: $cmd ...\n\n";
            system($cmd_repo_only) if ( $pkg_repo_only_cmd ne '' );
            system($cmd) == 0 or die "apt-get returned an error, check manually";
        }
    }
}
