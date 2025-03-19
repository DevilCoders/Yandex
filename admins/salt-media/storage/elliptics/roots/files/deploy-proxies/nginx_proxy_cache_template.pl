#!/usr/bin/perl  

use strict;
use warnings;
use Filesys::Df;

use Template;

my $dir = "/var/cache/nginx";
my $ref = df $dir;

# calculate space for nginx cache in Gb
my $cache_size = (($ref->{used} + $ref->{bavail})/1024/1024) - ((($ref->{used}+$ref->{bavail})/1024/1024) * 0.15);
$cache_size = int($cache_size);
printf("Disk space on $dir = $cache_size G\n");

my $cache_dir = "/etc/nginx/conf.d";
my $template_cache_dir = "/etc/nginx/Template";

my $tpl_file = "template_proxycached.conf";
if ( ! -f "$template_cache_dir/$tpl_file" ){
        $tpl_file = "template_proxycached-default.conf";
}

my $tt2 = new Template({
   INCLUDE_PATH=>"$template_cache_dir",
   OUTPUT_PATH => "$cache_dir"
});

$tt2->process ("$tpl_file", {
		cachesize => $cache_size,
	},
	"02-proxycached.conf.tmp",
	) || die $tt2->error(), "\n";
        if ( -s "$cache_dir/02-proxycached.conf.tmp" ){
                `mv $cache_dir/02-proxycached.conf.tmp $cache_dir/02-proxycache.conf`;
        }
	 else {
                die "parsed file $cache_dir/02-proxycached.conf.tmp not exist or zero size\n";
}
