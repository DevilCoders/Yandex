require 'ipaddr'
(01..12).each do |i|
	host = "corba-dev%2dу.dev.yandex.net" % i
	ip6 = IPAddr.new "2a02:6b8:0:1801::%s" % i.to_s.hex
	ip6_arpa = ip6.ip6_arpa
	puts "#{host} = #{ip6}"
end
