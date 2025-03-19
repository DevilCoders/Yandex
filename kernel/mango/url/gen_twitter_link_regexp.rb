# Этот скрипт генерирует строковый литерал для C++-программы, который является регулярным выражением для извлечения урлов
# из твитов. По факту это чуть-чуть переделанный Ruby-модуль https://github.com/mzsanford/twitter-text-rb/blob/master/lib/regex.rb,
# которым пользуется сам Твиттер.

REGEXEN = {}

REGEXEN[:valid_preceding_chars] = /(?:[^-\/"':!=a-z0-9_@\p{Cyrillic}]|^|\:)/
#WAS: REGEXEN[:valid_domain] = /(?:[^[:punct:]\s][\.-](?=[^[:punct:]\s])|[^[:punct:]\s]){1,}\.[a-z]{2,}(?::[0-9]+)?/
REGEXEN[:valid_domain] = /[^[:punct:]\s]+(?:[\.-][^[:punct:]\s]+)*\.[a-z\p{Cyrillic}]{2,}(?::[0-9]+)?/

REGEXEN[:valid_general_url_path_chars] = /[\p{Cyrillic}a-z0-9!\*';:=\+\,\$\/%#\[\]\-_~\(\)]/
# Allow URL paths to contain balanced parens
#  1. Used in Wikipedia URLs like /Primer_(film)
#  2. Used in IIS sessions like /S(dfd346)/
REGEXEN[:wikipedia_disambiguation] = /(?:\(#{REGEXEN[:valid_general_url_path_chars]}+\))/
# Allow @ in a url, but only in the middle. Catch things like http://example.com/@user
REGEXEN[:valid_url_path_chars] = /(?:#{REGEXEN[:wikipedia_disambiguation]}|@#{REGEXEN[:valid_general_url_path_chars]}+\/|[\.,]#{REGEXEN[:valid_general_url_path_chars]}+|#{REGEXEN[:valid_general_url_path_chars]}+)/
# Valid end-of-path chracters (so /foo. does not gobble the period).
#   1. Allow =&# for empty URL parameters and other URL-join artifacts
REGEXEN[:valid_url_path_ending_chars] = /[\p{Cyrillic}a-z0-9=_#\/\+\-]|#{REGEXEN[:wikipedia_disambiguation]}/
REGEXEN[:valid_url_query_chars] = /[\p{Cyrillic}a-z0-9!\*'\(\);:&=\+\$\/%#\[\]\-_\.,~]/
REGEXEN[:valid_url_query_ending_chars] = /[\p{Cyrillic}a-z0-9_&=#\/]/
REGEXEN[:valid_url] = %r{
    (?i)
    #{REGEXEN[:valid_preceding_chars]}
    (
      https?:\/\/
      #{REGEXEN[:valid_domain]}
      (?:/
        (?:
          #{REGEXEN[:valid_url_path_chars]}+#{REGEXEN[:valid_url_path_ending_chars]}|
          #{REGEXEN[:valid_url_path_chars]}+#{REGEXEN[:valid_url_path_ending_chars]}?|
          #{REGEXEN[:valid_url_path_ending_chars]}
        )?
      )?
      (?:\?#{REGEXEN[:valid_url_query_chars]}*#{REGEXEN[:valid_url_query_ending_chars]})?
    )
}x;

print REGEXEN[:valid_url].source.gsub('-mix', '').gsub('\\', '\\\\\\').gsub('"', '\\"').gsub(/\s/, '')

