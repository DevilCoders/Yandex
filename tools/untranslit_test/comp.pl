#!/usr/bin/perl -w
use strict;

@ARGV == 3
    or die "comp.pl OriginalFile TranslitFile GeneratedFile\n";
    
open (INOR, "<".$ARGV[0])
    or die "couldn't open input file: $!";

open (INTR, "<".$ARGV[1])
    or die "couldn't open input file: $!";

open (INGN, "<".$ARGV[2])
    or die "couldn't open input file: $!";

LBL:
while (<INGN>) {
    <INGN>
        or die "Unexpected end of untransliterated file!\n";
    my $w = <INOR>;
    my $t = <INTR>;
    $w
      or  die "Unexpected end of original file!\n";      
    $t
      or  die "Unexpected end of transliterated file!\n";      
    if ($w =~ /[0-9]/) {
        next;
    }
    $w =~ tr/¨ÉÖÓÊÅÍÃØÙÇÕÚÔÛÂÀÏĞÎËÄÆİß×ÑÌÈÒÜÁŞ¸/åéöóêåíãøùçõúôûâàïğîëäæıÿ÷ñìèòüáşå/;
    chomp $w;
    chomp $t;

    tr/¨ÉÖÓÊÅÍÃØÙÇÕÚÔÛÂÀÏĞÎËÄÆİß×ÑÌÈÒÜÁŞ¸/åéöóêåíãøùçõúôûâàïğîëäæıÿ÷ñìèòüáşå/;
    my @arr = split /\s+/;
    for (@arr) {        
        $w eq $_
            and next LBL;
    }
    print "$w, $t: ", join (" ", @arr), "\n";    
}

my $w = <INOR>;
$w
    and die "Unexpected end of generated file!\n";

