#!/bin/sh

mkgroups=$0
attrconf=$1
indexpath=$2
if [ ! $indexpath ]
then
    echo "Usage:"
    echo "    ${mkgroups} grattr.cfg workdir [tempdir]"
    echo "Format of grattr.cfg file:"
    echo "    <src> <groupattr> [ <size> [<attrname> [<sortmode> [<use_ruscollate>]]]]"
    echo "        <src>: [arc|keys|attr] -- source for groupattr"
    echo "        <groupattr> -- grouping attribute name"
    echo "        <size> -- size of groupattr in bytes (4 by default)"
    echo "        <attrname> -- name of keys/arc attribute (==groupattr by default)"
    echo "        <sortmode>: [zeroes|nozeroes] -- adds leading zeroes to numbers (zeroes by default)"
    echo "        <use_ruscollate>: [yes|no] -- use pre- step to convert attr value by ruscollate::stringxfrm (no by default)"
    exit
fi

exec < ${attrconf}
temppath=$3

if [ ! $temppath ]
then
temppath=$TMP
fi

unpackattr=`grep -ic "^attr" ${attrconf}`
unpackarc=`grep -ic "^arc"  ${attrconf}`
curpath=$PWD
cd ${indexpath}
indexpath=$PWD
cd ${curpath}
cd ${temppath}

cp ${indexpath}/indexatr ./indexatr
cp ${indexpath}/indexaof ./indexaof
cp ${indexpath}/indexarc ./indexarc
rm -f ${indexpath}/*.c2n

if [ $unpackattr -gt 0 ]
then
    echo "unpacking indexatr/indexaof"
    atrview ./index
fi
if [ $unpackarc -gt 0 ]
then
    echo "unpacking indexarc"
    arcview -e ${indexpath}/index > ./arc
fi
rm -f clatrbase_config
while true
do
    read src gattr size sattr mode use_ruscollate
    if [ ! $src ]
    then
        break
    fi
    src0=`echo ${src} |sed 's/^\(.\).*/\1/'`
    if [ $src0 = "#" ]
    then
        continue
    fi
    if [ ! $gattr ]
    then
        echo "Error in ${attrconf}"
        break
    fi
    if [ ! $sattr ]
    then
        sattr=$gattr
    fi
    if [ ! $size ]
    then
        size=4
    fi
    if [ $src = "keys" ]
    then
        echo "printkeysing ${sattr}"
        printkeys -wk "#${sattr}" ${indexpath}/indexkey ${indexpath}/indexinv | perl -x ${mkgroups} keys ${indexpath} ${gattr} ${sattr} ${mode} ${use_ruscollate}
        mv ${gattr}.c2n ${indexpath}/
    fi
    if [ $src = "arc" ]
    then
        echo "extracting ${sattr}"
        cat ./arc | perl -x  ${mkgroups} arc ${indexpath} ${gattr} ${sattr} ${mode} ${use_ruscollate}
        mv ${gattr}.c2n ${indexpath}/
    fi
    echo  "${gattr}:${size}" >>clatrbase_config
done

clattrgen

mv ./attrbase ${indexpath}/indexatr
mv ./mapfile ${indexpath}/indexaof
rm -f ./arc
rm ./indexarc
rm ./indexatr
rm ./indexaof
rm -f *.d2c
rm clatrbase_config
exit


#!perl
use strict;
use ruscollate;

my ($frommode, $workindexdir, $groupattr, $attr, $mode, $use_ruscollate) = @ARGV;
$workindexdir =~ s/\/$//;
$attr = $groupattr  unless $attr;
my $mode = "zeroes" unless $mode;

my %d2n = undef;
my %n2rn = undef;
my @n = undef;
my $maxdoc = -1;

if($frommode eq "keys"){
    my $attrval = undef;
    my $realattrval = undef;
    $attr =~ tr/A-Z/a-z/;
    while(<STDIN>){
        if (m/^\#(.*?)=\"?(.*) ([0-9]+)$/ and $attr eq $1){
            $attrval = $2;
            $realattrval = $attrval;
            $attrval = zeroes($attrval)."\n".$attrval if $mode eq "zeroes";
            $attrval = stringxfrm( $attrval ) if( $use_ruscollate eq "yes" );
            $n2rn{$attrval} = $realattrval;
            push (@n, $attrval);
        }elsif (m/^\t\[([0-9]+).[0-9]+.[0-9]+.[0-9]+\]$/){
            $d2n{$1} = $attrval;
            $maxdoc = $1 if $1 > $maxdoc;
        }
    }
}elsif($frommode eq "arc"){
    my $doc = -1;
    $attr =~ tr/a-z/A-Z/;
    while(<STDIN>){
        if (m/^(.*?)=\"?(.*)$/ and $attr eq $1){
            my $attrval = $2;
            my $realattrval = $attrval;
            $attrval =~ s/^0+([0-9]+)$/$1/; #delete leading zeroes
            $attrval = zeroes($attrval)."\n".$attrval if $mode eq "zeroes";
            $attrval = stringxfrm( $attrval ) if( $use_ruscollate eq "yes" );
            $n2rn{$attrval} = $realattrval;
            push (@n, $attrval);
            $d2n{$doc} = $attrval;
        }elsif (m/^~~~~~~ Document ([0-9]+) ~~~~~~$/){
            $doc = $1;
            $maxdoc = $1 if $1 > $maxdoc;
        }
    }
}

my @c2n = sort @n;
my %n2c = undef;
open (C2N, ">$groupattr.c2n");
for (my $i = 0; $i < scalar(@c2n); $i++){
    printf C2N "$i\t$n2rn{$c2n[$i]}\n";
    $n2c{$c2n[$i]} = $i;
}
close (C2N);
open (D2C, ">$groupattr.d2c");
for (my $d = 0; $d <= $maxdoc; $d++){
    printf D2C "$d\t$n2c{$d2n{$d}}\n" if $d2n{$d};
}
close (D2C);

sub zeroes{
    my($attr) = @_;
    $attr =~ s/([0-9]+)/sprintf("%011d",$1)/ge;
    return $attr;
}
