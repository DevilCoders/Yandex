#!/usr/bin/perl -w
# Скрипт ищет в директории корки по шаблону и шлет их разработчикам
# и на рассылку corba-cc
use strict;
use MIME::Lite;
use Sys::Hostname;

# Рисуем время и дату.
my @time = localtime(time);
$time[5] += 1900;
my $time_is = "Time is: $time[2]:$time[1]:$time[0], Date is: $time[3].$time[4].$time[5]";

# Определяем имя машинки, шаблон для поиска корки и директорию корок.
my $host = hostname;
my $core_pattern = "core.fastcgi";
my $core_path="/var/tmp/";

# Определяем кому слать
my $from = "root\@$host";
my $to = "dps-proxy-cores\@yandex-team.ru";
my $cc = "corba-cc\@yandex-team.ru";

# Читаем директорию и ищем в ней корки, собираем список в массиве @cores
opendir(my $core_dir, $core_path) or die "can't open $core_path: $!";
my @cores = grep { /^$core_pattern/ } readdir($core_dir);
closedir $core_dir;

# Отправляем почтой, удаляем в последствии.
foreach(@cores) {
    chomp($_);
    my $msg = MIME::Lite->new(
        From=>$from,
        To=>$to,
        Subject=>'Core files from dps-proxy',
        Type=>'multipart/mixed');
    $msg->attach(
        Type=>'text/plain',
        Data=>$time_is);
    $msg->attach(
        Path=>$core_path . $_,
        Filename=>"$_",
        Disposition=>'attachment');
    $msg->send();
    unlink("$core_path/$_");
};
