#!/usr/bin/perl

my $i,$j,$k;
my @abc;

if ($ARGV[0] eq '-c'){
    @abc = qw(а б в г д е ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я);
    $longword = "длинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинное".
    			"длинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинное".
    			"длинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинное".
    			"длинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинное".
    			"длинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинноедлинное".
    			"слово";
}elsif ($ARGV[0] eq '-l'){
    @abc = qw(a b c d e f g h i j k l m n o p q r s t u v w x y z);
    $longword = "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong".
    			"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong".
    			"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong".
    			"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong".
    			"longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong".
    			"word";
}else{
    die "Usage: gentrigramswords.pl -(l|c)\n";
}

print $longword."\n";
for ($i = 0; $i < scalar(@abc); $i++){
   for ($j = 0; $j < scalar(@abc); $j++){
       print $abc[$i].$abc[$j]."\n";
       for ($k = 0; $k < scalar(@abc); $k++){
           print $abc[$i].$abc[$j].$abc[$k].$abc[$i].$abc[$j]."\n";
       }
   }
}

