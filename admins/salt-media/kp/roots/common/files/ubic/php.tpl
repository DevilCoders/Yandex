use Ubic::Service::SimpleDaemon;

Ubic::Service::SimpleDaemon->new({
        bin => '/usr/sbin/{{ config.fpm_bin }} --fpm-config {{ config.fpm_conf }} --pid {{ config.fpm.global.pid }} --nodaemonize',
	stdout => '/var/log/php/stdout.log',
	stderr => '/var/log/php/stderr.log',
});
