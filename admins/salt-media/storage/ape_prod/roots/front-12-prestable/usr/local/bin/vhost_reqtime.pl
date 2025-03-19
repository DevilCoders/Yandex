#!/usr/bin/env perl

while (<STDIN>) {
  @fields = split(/^\[(?:[^\[\]]+?)\] (.+?) (?:.+?) "(?:[^\"]+?)" (?:[0-9]{3}) "(?:[^\"]+?)" "(?:[^\"]+?)" "(?:[^\"]+?)" ([^ ]+?) .*/,$_);
  if ($ARGV[0]) {
    if ($fields[1] eq $ARGV[0]) {
      print "$fields[2]\n";
    }
  } else {
    print "$fields[1] $fields[2]\n";
  }
}

