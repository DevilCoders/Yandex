#!/usr/bin/perl
package Test::Nginx::Sdch;

use strict;
use Test::Nginx::Socket;
use MIME::Base64 qw(encode_base64url);
use File::Temp qw(tempfile);

sub get_server_id($) {
  my $expected_dict = shift;
  my $dir = html_dir();
  open (my $SHA256, "-|", "sha256sum -b < $dir/$expected_dict") || die "sha256sum: $!";
  my $sha256 = <$SHA256>;
  die "sha256sum result" if !$sha256;
  die "sha256sum result" if $sha256 !~ m,^([a-fA-F0-9]{64})\s,;
  $sha256 = $1;
  my $sha256bin = pack('H*', $sha256);
  my $serveridbin = substr($sha256bin, 6, 6);
  my $serverid = encode_base64url($serveridbin);
  return $serverid;
}

sub get_vcdiff_dict_fn($) {
  my $expected_dict = shift;
  my $dir = html_dir();
  my ($fh, $filename) = tempfile(UNLINK => 1);
  open(my $edict, "<", "$dir/$expected_dict") || die "get_vcdiff_dict_fn: $dir/$expected_dict: $!";
  my $header = 1;
  while (<$edict>) {
    if ($header && ($_ eq "\n")) {
      $header = 0;
      next;
    }
    if ($header) {
      next;
    }
    print $fh $_;
  }
  close($fh);
  return $filename;
}

add_response_body_check(sub {
  my ($block, $body, $req_idx, $repeated_req_idx, $dry_run) = @_;
  my $name = $block->name;
  my $expected_dict = $block->resp_sdch_dict;
  
  if (!defined($expected_dict)) {
    return;
  }
  SKIP: {
    skip "name - sdch_dict - tests skipped due to dry_run", 1 if $dry_run;
    my $serverid = get_server_id($expected_dict);
    is(substr($body, 0, 9), "$serverid\0", "$name - server id");

    my $vcddict = get_vcdiff_dict_fn($expected_dict);
    my ($vcderrfh, $vcderr) = tempfile();
    close($vcderrfh);
    open(my $decode, "|-", "vcdiff decode -dictionary $vcddict > /dev/null 2>$vcderr") or die "vcdiff exec: $!";
    print $decode substr($body, 10, 0);
    ok(close($decode), "$name - vcdiff decode ($! $?)");
    open(my $vcderrfhi, "<", $vcderr) or die "vcdiff err read: $!";
    my $vcderrstr = <$vcderrfhi>;
    is ($vcderrstr, undef, "$name - vcdiff stderr empty");
    close($vcderrfhi);
    unlink($vcderr);
  }
});

1;
