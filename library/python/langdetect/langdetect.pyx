# coding: utf-8
from libc.stdint cimport int32_t
from libcpp.string cimport string
from libcpp.list cimport list as cpplist
from libcpp cimport bool


cdef extern from "contrib/libs/lang_detect/include/langdetect/find_domain_result.hpp" namespace "langdetect" nogil:
    cdef cppclass find_domain_result:
        find_domain_result();

        string domain
        int32_t content_region
        bool found


cdef extern from "contrib/libs/lang_detect/include/langdetect/domain_filter.hpp" namespace "langdetect" nogil:
    cdef cppclass domain_filter:
        domain_filter();

        bool empty() const
        void add(const cpplist[string]& domain)
        void add(const string& comma_separated_domains)


cdef extern from "contrib/libs/lang_detect/include/langdetect/domaininfo.hpp" namespace "langdetect" nogil:
    cdef cppclass domaininfo:
        domaininfo()

        bool parse_geo_regions(const string& str)
        bool parse_host(const string& str)
        bool parse_cookie_cr(const string& str)
        bool set_geo_regions(const cpplist[int32_t]& regions)


cdef extern from "contrib/libs/lang_detect/include/langdetect/userinfo.hpp" namespace "langdetect" nogil:
    cdef cppclass userinfo:
        userinfo();

        const string& accept_language() const
        const string& top_level_domain() const
        const cpplist[int32_t]& geo_regions() const
        int32_t cookie_value() const

        bool parse_accept_language(const string& str)
        bool parse_geo_regions(const string& str)
        bool parse_host(const string& str)

        bool parse_cookie(int32_t value)

        bool set_geo_regions(const cpplist[int32_t]& regions)
        bool set_pass_language(const string& str)


cdef extern from "contrib/libs/lang_detect/include/langdetect/langinfo.hpp" namespace "langdetect" nogil:
    cdef cppclass langinfo:
        langinfo()
        langinfo(int32_t value)

        string code
        string name
        int32_t cookie_value


cdef extern from "contrib/libs/lang_detect/include/langdetect/filter.hpp" namespace "langdetect" nogil:
    cdef cppclass filter:
        filter()

        bool empty() const
        bool allowed(const string& lang) const

        void add(const string& comma_separated_langs)

        void add_allowed_langs(const cpplist[string]& langs)
        void add_allowed_langs(const string& comma_separated_langs)


cdef extern from "contrib/libs/lang_detect/include/langdetect/lookup.hpp" namespace "langdetect::lookup" nogil:
    ctypedef cpplist[langinfo] lang_list_type;


cdef extern from "contrib/libs/lang_detect/include/langdetect/lookup.hpp" namespace "langdetect" nogil:
    cdef cppclass lookup:
        lookup(const string& path)

        bool need_swap() const
        const string& filepath() const

        bool get_info(const string& lang, langinfo& li) const

        bool find(const userinfo& ui, langinfo& li, const filter* f) const
        bool find(const userinfo& ui, langinfo& li, const filter* f, const string& def_lang) const
        bool find_without_domain(const userinfo &ui, langinfo& li, const filter* f, const string &def_lang) const

        bool list(const userinfo& ui, lang_list_type& langs, const filter* f) const
        bool list(const userinfo& ui, lang_list_type& langs, const filter* f, const string& def_lang) const

        string find_domain(const domaininfo& di, const domain_filter* f) const
        string find_domain(const domaininfo& di, const domain_filter* f, bool &found) const

        void find_domain_ex(const domaininfo& di, const domain_filter* f, find_domain_result& result) const


cdef class Langdetect:
    cdef lookup* _lookup

    def __cinit__(self, const char* path):
        self._lookup = new lookup(path)

    def __dealloc__(self):
        del self._lookup

    def find(self, data):
        cdef userinfo ui
        cdef langinfo li
        cdef filter f
        cdef string def_lang

        l = []

        if 'geo' in data:
            ui.parse_geo_regions(data['geo'])
        if 'domain' in data:
            ui.parse_host(data['domain'])

        if 'filter' in data:
            f.add(data['filter'])

        if 'language' in data:
            ui.parse_accept_language(data['language'])
        if 'pass-language' in data:
            ui.set_pass_language(data['pass-language'])
        if 'cookie' in data and data['cookie']:
            ui.parse_cookie(int(data['cookie']))

        if self._lookup.find(ui, li, &f, data.get('default')):
            l.append((li.code, li.name))
        return l

    def find_without_domain(self, data):
        cdef userinfo ui
        cdef langinfo li
        cdef filter f
        cdef string def_lang

        l = []

        if 'geo' in data:
            ui.parse_geo_regions(data['geo'])
        if 'domain' in data:
            ui.parse_host(data['domain'])

        if 'filter' in data:
            f.add(data['filter'])

        if 'language' in data:
            ui.parse_accept_language(data['language'])
        if 'pass-language' in data:
            ui.set_pass_language(data['pass-language'])
        if 'cookie' in data and data['cookie']:
            ui.parse_cookie(int(data['cookie']))

        if self._lookup.find_without_domain(ui, li, &f, data.get('default')):
            l.append((li.code, li.name))
        return l

    def language2cookie(self, lang):
        cdef langinfo li
        self._lookup.get_info(lang, li)
        return li.cookie_value
