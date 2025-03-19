#!/usr/bin/env perl
# MANAGED BY SALT
# CADMIN-6977
use strict;
use warnings;
# используем утилиту для прохода по строчка лога в обратном порядяке
if (!-x "/usr/bin/tac") { die "/usr/bin/tuc not found" }
# получаем список логов
my @logs = sort glob("/var/log/cron/*.log");
# задаем хеш для хранения ошибок
my %errors = ();
foreach my $log (@logs) {
    # KPDUTY-1526
    if ($log =~ /cron_parser_php/) { next; }
    # открываем десприктор для чтения файла в обратно порядке
    open REVERSED, "/usr/bin/tac ${log} | ";
    # получаем сведения о текущем файле
    my @log_stats = stat($log);
    my $log_size = $log_stats[7];
    # пустые файлы пропускаем
    if ($log_size < 1) { next; }
    # позже в файлах мы будем искать некий текст, но пока что считаем что лог не валидный
    my $valid_log = 0;
    # причина ошибки в логе
    my $mon_reason = '';
    # идём по файлу в обратном порядке
    while (<REVERSED>) {
        my $line = $_;
        # данная регуляка должна быть в каждом файле,
        # иначе позже мы будем считать что в файле не найден код выхода из крона
        if ($line =~ /[eE]xit\s+code:?\s+(\d+)/) {
            my $code = $1;
            # если мы сюда попали, значит в логе найден код выходп крона,
            # обновляем переменную, позже задействуем её
            $valid_log = 1;
            # если код выхода меньше 2 (0 - это ОК, 1 - это flock)
            if ($code lt 2) {
                #  пропускаем такой лог
                last;
            } else {
                # иначе - есть причина для добавления в список ошибок
                $mon_reason = "non-zero code";
                # дальше сканировать текущий лог нет смысла
                last;
            }
        }
    }
    # # если мы не нашли ни одной строчки про код выхода - это проблема
    # if(!$valid_log) {
    #     $mon_reason = "exit-code not found";
    # }
    # my $now = `date +%s`;
    # if ($now-$log_stats[9] gt 30) {
    #     $mon_reason = "exit-code not found";
    # }
    # если у нас есть причина для добавления в лог ошибок
    if ($mon_reason) {
        # делаем это
        $errors{$log} = $mon_reason;
    }
}
# считаем количество ключей в хеше с ошибками
my $count = scalar keys %errors;
# если количество ключей больше нуля, тогда monrun-статус приравниваем 2 (проблема), иначе 0
my $status = $count ? 2 : 0;
# получаем текстовый дескрипшен статуса
my $desc = $status ? "FAILED: " : "OK.";
# задаем пустую строчку с детализацией
my $data = '';
# заполняем детализацию
while (my ($k,$v) = each %errors) {
    $data = $data . sprintf("%s => %s; ", $k, $v);
}
# печатаем результат
printf("%s; %s%s\n", $status, $desc, $data);
# бесполезная инструкция =)
exit($status);
