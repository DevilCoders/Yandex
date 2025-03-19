module Yandex

    module Passport
        require 'net/http'
        require 'rexml/document'
        require 'rexml/xpath'
    require 'uri'

    BLACKBOX_URL = 'http://blackbox.yandex-team.ru';
    PASSPORT_URL = 'https://passport.yandex-team.ru/passport';

        def Passport::check_session(session_id, host, remoteip, fields = ['accounts.login.uid'], blackbox_url = BLACKBOX_URL)
            url = '%s/blackbox?method=sessionid&sessionid=%s&host=%s&userip=%s&dbfields=%s' % [
        blackbox_url,
                URI.escape(session_id),
                host,
                remoteip,
                fields.join(',')
            ]

            url = URI.parse(url)
            http = Net::HTTP.new(url.host)
            http.read_timeout=3


            begin
                resp, data = http.request_get(url.request_uri)
            rescue
                return { 'status' => '255', 'error' => 'Error while fetching blackbox answer' }
            end

            doc = REXML::Document.new(data)
            results = {}

            status = REXML::XPath.first(doc, '/doc/status/@id')
            status_text = REXML::XPath.first(doc, '/doc/status')
            error = REXML::XPath.first(doc, '/doc/error')

            results['status'] = status.value unless status.nil?
            results['status_text'] = status_text.text unless status_text.nil?
            results['error'] = error.text unless error.nil?

            return results unless results['status'] == '0'

            fields.each do |f|
                results[f] = REXML::XPath.first(doc, '/doc/dbfield[@id="%s"]' % f).text
            end

            return results
        end
                
    end

end
