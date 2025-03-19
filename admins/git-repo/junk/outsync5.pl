#!/usr/bin/perl

use strict;
use Sort::Versions;
use Set::Scalar;

#versioncmp;

my $repo_root = '/repo';

my @branches_to_copy = ('stable', 'testing', 'unstable');
#my $dest_repo = 'tojetstyle';

my $jet_rsync = "rsync://repo.ya.jetstyle.ru/yandex";

#Paths to configs with packages. The first line should be with name of dest_repo, the second with src_repo.
my $config_dir = "/etc/tooutsource/repos";
opendir(my $dh, $config_dir) || die "can't opendir $config_dir: $!";
my @repos = grep { -f "$config_dir/$_" } readdir($dh);
closedir $dh;

sub grep_repo($$) {
	my ($repo_path, $wanted_set) = @_;
	open (REPO, "ls -1 $repo_path/*.changes 2>/dev/null |");
	my $res = new Set::Scalar;
	while (<REPO>) {
		if (/\/([^\/_]+)_([^\/_]+)_[a-z0-9]+\.changes/) {
			if (defined($wanted_set)) {
				$res->insert("$1 $2") if $wanted_set->has($1);
			} else {
				$res->insert("$1 $2");
			}
		} else {
			print STDERR "Failed to parse $_\n";
		}
	}
	close(REPO);
	return $res;
}

foreach my $branch (@branches_to_copy) {
	foreach my $repo (@repos) {
		open PACKS, '<', "$config_dir/$repo" or die $!;
		my @repo = <PACKS>;
		close PACKS;
		chomp (@repo);
		my $dest_repo = shift @repo;
		my $src_repo = shift @repo;

		my $dest_dir = "$repo_root/$dest_repo/$branch";
		my $dest_packs = grep_repo($dest_dir, undef);
		my $sync = 0;

		my $wanted_packs = new Set::Scalar(@repo);
		my $repo_dir = "$repo_root/$src_repo/$branch";
		my $repo_packs = grep_repo($repo_dir, $wanted_packs);
		
		my $diff = $repo_packs - $dest_packs;
		$sync = not $diff->is_empty;
		foreach my $pack ($diff->elements) {
			print STDERR "Found new package $pack in $src_repo $branch. Copying it to $dest_repo\n";
			system("/usr/sbin/bmove -c $branch $dest_repo $pack $src_repo");
		}
	}
#	chdir($dest_dir);
#	foreach my $arch ('qall', 'i386', 'amd64') {
#		system("/usr/bin/rsync --delete -qavz $arch/* $jet_rsync/$branch/$arch/");
#	}
}	
