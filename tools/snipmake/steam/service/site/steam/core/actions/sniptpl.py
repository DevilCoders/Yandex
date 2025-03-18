# -*- coding: utf-8 -*-

from django import template

from core.actions import snipextraparser
from core.actions.common import jsonify
from core.actions.snipextraparser import JsonFields
from core.models import Snippet


class SnipTemplateAbstract(object):
    market_text_tpl = template.loader.get_template('core/snippet_templates/market_text.html')

    def __init__(self, snip, proto=False):
        self.snip = snip
        self.proto = proto
        if proto:
            extra_json = jsonify(getattr(snip, 'ExtraInfo', {}),
                                 'JSONIFY_SNIP_TPL_LOAD_ERROR')
            self.extra = snipextraparser.parse(extra_json)
        else:
            self.extra = snip.data.get('extra', {})
        self.extra = snipextraparser.prepare(self.extra)


#private function
#use a_content for divide links from tag <l> and from <a> in escape in safemarkup
    def render_content_links(self, fragments_old):
        fragments = []
        i = 0
        for fragment in fragments_old:
            while '<l>' in fragment:
                target = self.extra[JsonFields.LINK_ATTRS][i]
                fragment = fragment \
                    .replace('<l>', '<a_content href=\"%s\">' % target, 1) \
                    .replace('</l>', '</a_content>', 1)
                i += 1
            fragments.append(fragment)
        return fragments

    def render_text(self, **kwargs):
        if JsonFields.LINK_ATTRS in self.extra:
            kwargs['fragments'] = self.render_content_links(kwargs['fragments'])
        res = self.text_tpl.render(template.Context(kwargs))
        if JsonFields.MARKET in self.extra:
            res = self.market_text_tpl.render(template.Context({
                'description': res,
                'extra': self.extra[JsonFields.MARKET],
            }))
        return res

    def render_title(self, **kwargs):
        return self.title_tpl.render(template.Context(kwargs))

    def render_green_url(self, **kwargs):
        return self.green_url_tpl.render(template.Context(kwargs))

    def use_breadcrumbs(self):
        return False


class BaseSnipTemplate(SnipTemplateAbstract):
    tpl_type = Snippet.Template.BASE
    text_tpl = template.loader.get_template('core/snippet_templates/base_text.html')
    title_tpl = template.loader.get_template('core/snippet_templates/base_title.html')
    green_url_tpl = template.loader.get_template('core/snippet_templates/base_green_url.html')

    def __init__(self, snip, proto=False):
        super(BaseSnipTemplate, self).__init__(snip, proto)
        if proto:
            self.headline = getattr(snip, 'Headline', '')
            self.fragments = [fr.Text for fr in snip.Fragments]
            self.title = snip.TitleText
            self.url = snip.Url
            self.hilitedurl = getattr(snip, 'HilitedUrl', '')
            self.urlmenu = []
            self.link_attrs = []
        else:
            self.headline = snip.data.get(JsonFields.HEADLINE, '')
            self.fragments = snip.data['text']
            self.title = snip.data['title']
            self.url = snip.data['url']
            self.hilitedurl = snip.data.get(JsonFields.HILITEDURL, '')
            self.urlmenu = snip.data.get(JsonFields.URLMENU, [])
            self.link_attrs = snip.data.get(JsonFields.LINK_ATTRS, [])

    def render_text(self, **kwargs):
        kwargs['headline'] = self.headline
        kwargs['fragments'] = self.fragments
        kwargs['by_link'] = self.extra.get(JsonFields.BY_LINK)
        kwargs['link_attrs'] = self.extra.get(JsonFields.LINK_ATTRS)
        return super(BaseSnipTemplate, self).render_text(**kwargs)

    def render_title(self, **kwargs):
        kwargs['title'] = self.title
        return super(BaseSnipTemplate, self).render_title(**kwargs)

    def render_green_url(self, **kwargs):
        kwargs['url'] = self.url
        kwargs['hilitedurl'] = self.hilitedurl
        kwargs['urlmenu'] = self.urlmenu
        kwargs['use_breadcrumbs'] = self.use_breadcrumbs()
        return super(BaseSnipTemplate, self).render_green_url(**kwargs)

    def use_breadcrumbs(self):
        if not self.urlmenu:
            return False
        crumbs_occur = sum(title.count('<b>') for url, title in self.urlmenu)
        hilitedurl_occur = self.hilitedurl.count('<b>')
        return crumbs_occur >= hilitedurl_occur


class ListSnipTemplate(BaseSnipTemplate):
    tpl_type = Snippet.Template.LIST
    text_tpl = template.loader.get_template('core/snippet_templates/list_text.html')

    def render_text(self, **kwargs):
        kwargs['extra'] = self.extra.get(JsonFields.SPEC_ATTRS,
                                         {}).get(JsonFields.LISTDATA, {})
        return super(ListSnipTemplate, self).render_text(**kwargs)

class RCASnipTemplate(BaseSnipTemplate):
    tpl_type = Snippet.Template.RCA
    media_content_tpl = template.loader.get_template('core/snippet_templates/media_content.html')

    def render_media_content(self, **kwargs):
        kwargs['images'] = self.extra.get(JsonFields.IMG_AVATARS, [])
        return self.media_content_tpl.render(template.Context(kwargs))

class UnknownSnipTemplate(BaseSnipTemplate):
    tpl_type = None


HEADLINE_SRCS_TPLS = {
    None: BaseSnipTemplate,
    '': BaseSnipTemplate,
    'ruwiki': BaseSnipTemplate,
    'static_annotation': BaseSnipTemplate,
    'list_snip': ListSnipTemplate,
    'rca': RCASnipTemplate,
}


def get_tpl_class(headline_src):
    return HEADLINE_SRCS_TPLS.get(headline_src, UnknownSnipTemplate)


def get_snip_tpls(snippets, proto=False):
    return [
        get_tpl_class(
            getattr(snip, 'HeadlineSrc', None) if proto
            else snip.data.get(JsonFields.HEADLINE_SRC)
        )(snip, proto=proto)
        for snip in snippets
    ]


def get_snip_tpls_ext(task, ignore_shuffle=False):
    return zip(get_snip_tpls(task.snippets(ignore_shuffle)),
               task.cur_tpls(ignore_shuffle))
