from collections import OrderedDict

import src.modules as Modules
import helpers

CALLISTO_VIEWER_HOST = 'cv.clusterstate.yandex-team.ru(:\\\\d+)?'

JUPITER_VIEWER_HOST = 'jv.clusterstate.yandex-team.ru(:\\\\d+)?'

CTRL_VIEW_CONTROLLER_HOST = 'ctrl-view.clusterstate.yandex-team.ru(:\\\\d+)?'
BETA_CONTROLLER_HOST = 'ctrl.clusterstate.yandex-team.ru(:\\\\d+)?'

#

VIDEO_MULTIBETA_BACKENDS = helpers.endpointsets('video-betas-multi-base-controller', {'vla'})
VIDEO_MULTIMETA_BACKENDS = helpers.endpointsets('video-betas-multi-meta-controller', {'vla'})
VIDEO_MULTIQUICK_BACKENDS = helpers.endpointsets('video-betas-multi-quick-base-controller', {'vla'})

VIDEO_VIEWER_BACKENDS = helpers.endpointsets('video-viewer', {'sas', 'vla'})
VIDEO_PROD_BACKENDS = helpers.endpointsets('video-prod-main-controller', {'vla'})
VIDEO_PIP_BACKENDS = helpers.endpointsets('video-prod-acceptance-controller', {'vla'})
VIDEO_MAN_BACKENDS = helpers.endpointsets('video-prod-man-controller', {'man'})
VIDEO_SAS_BACKENDS = helpers.endpointsets('video-prod-sas-controller', {'sas'})
VIDEO_VLA_BACKENDS = helpers.endpointsets('video-prod-vla-controller', {'vla'})

IMG_BETA1_BACKENDS = helpers.endpointsets('images-betas-beta1-controller', {'vla'})
IMG_BETA2_BACKENDS = helpers.endpointsets('images-betas-beta2-controller', {'vla'})
IMG_THUMB_BACKENDS = helpers.endpointsets('images-prod-thumb-wide-controller', {'vla'})
IMG_MULTIBETA_BACKENDS = helpers.endpointsets('images-betas-multi-base-controller', {'vla'})
IMG_MULTIMETA_BACKENDS = helpers.endpointsets('images-betas-multi-meta-controller', {'vla'})
IMG_MULTIQUICK_BACKENDS = helpers.endpointsets('images-betas-multi-quick-base-controller', {'vla'})

IMG_VIEWER_BACKENDS = helpers.endpointsets('img-viewer', {'sas', 'vla'})
IMG_PROD_BACKENDS = helpers.endpointsets('images-prod-main-controller', {'vla'})
IMG_PIP_BACKENDS = helpers.endpointsets('images-prod-acceptance-controller', {'vla'})
IMG_MAN_BACKENDS = helpers.endpointsets('images-prod-man-controller', {'man'})
IMG_SAS_BACKENDS = helpers.endpointsets('images-prod-sas-controller', {'sas'})
IMG_VLA_BACKENDS = helpers.endpointsets('images-prod-vla-controller', {'vla'})
IMAGES_OLDPROD_BACKENDS = helpers.endpointsets('images-oldprod2-controller', {'vla'})

RIM_VIEWER_BACKENDS = helpers.endpointsets('rim-viewer', {'sas', 'vla'})
RIM_PROD_BACKENDS = helpers.endpointsets('images-prod-rim-controller', {'vla'})
RIM_PIP_BACKENDS = helpers.endpointsets('images-prod-rim-acceptance-controller', {'vla'})
RIM_MAN_BACKENDS = helpers.endpointsets('images-prod-rim-man-controller', {'man'})
RIM_SAS_BACKENDS = helpers.endpointsets('images-prod-rim-sas-controller', {'sas'})
RIM_VLA_BACKENDS = helpers.endpointsets('images-prod-rim-vla-controller', {'vla'})

IMG_COMMERCIAL_VIEWER_BACKENDS = helpers.endpointsets('images_commercial_data_viewer', {'sas', 'vla'})
IMG_COMMERCIAL_MAIN_BACKENDS = helpers.endpointsets('images_commercial_data_controller', {'sas'})
IMG_COMMERCIAL_PIP_BACKENDS = helpers.endpointsets('images_commercial_data_controller_pip', {'sas'})
IMG_COMMERCIAL_MAN_BACKENDS = helpers.endpointsets('images_commercial_data_controller_man', {'man'})
IMG_COMMERCIAL_SAS_BACKENDS = helpers.endpointsets('images_commercial_data_controller_sas', {'sas'})
IMG_COMMERCIAL_VLA_BACKENDS = helpers.endpointsets('images_commercial_data_controller_vla', {'vla'})

WEB_MULTIBETA_BACKENDS = helpers.endpointsets('web-betas-multi-base-controller', {'vla'})
WEB_MULTIMETA_BACKENDS = helpers.endpointsets('web-betas-multi-meta-controller', {'vla'})
WEB_MULTIINT_BACKENDS = helpers.endpointsets('web-betas-multi-int-controller', {'vla'})
WEB_MULTIFULL_BACKENDS = helpers.endpointsets('web-betas-multi-full-controller', {'sas'})

WEB_VIEWER_BACKENDS = helpers.endpointsets('jupiter-viewer', {'sas', 'vla'})
WEB_PROD_BACKENDS = helpers.endpointsets('web-prod-main-controller', {'vla'})
WEB_PIP_BACKENDS = helpers.endpointsets('web-prod-acceptance-controller', {'vla'})
WEB_MAN_BACKENDS = helpers.endpointsets('web-prod-man-controller', {'man'})
WEB_SAS_BACKENDS = helpers.endpointsets('web-prod-sas-controller', {'sas'})
WEB_SAS_PLATINUM_BACKENDS = helpers.endpointsets('web-prod-sas-platinum-controller', {'sas'})
WEB_VLA_BACKENDS = helpers.endpointsets('web-prod-vla-controller', {'vla'})
WEB_OLDPROD_BACKENDS = helpers.endpointsets('web-oldprod2-controller', {'vla'})

CALLSITO_BETA_BACKENDS = helpers.endpointsets('web-callisto-beta-controller', {'vla'})
CALLSITO_PUMPKIN_BACKENDS = helpers.endpointsets('web-callisto-pumpkin-controller', {'sas'})
CALLISTO_VIEWER_BACKENDS = helpers.endpointsets('callisto-viewer', {'vla'})
CALLISTO_MAN_BACKENDS = helpers.endpointsets('web-callisto-prod-man-controller', {'man'})
CALLISTO_SAS_BACKENDS = helpers.endpointsets('web-callisto-prod-sas-controller', {'sas'})
CALLISTO_VLA_BACKENDS = helpers.endpointsets('web-callisto-prod-vla-controller', {'vla'})

SAAS_FROZEN_BETA = helpers.endpointsets('saas-betas-frozen-controller', {'vla'})
SAAS_NEWS_BETA = helpers.endpointsets('saas-betas-news-controller', {'vla'})
SAAS_NEWS_MMETA_BETA = helpers.endpointsets('saas-betas-news-mmeta-controller', {'vla'})


def _add_cgi(*cgi):
    if not cgi:
        return []
    return [(
        Modules.Rewrite, {
            'actions': [
                {
                    'split': 'cgi',
                    'regexp': r'^\\?(.*)',  # does start with `?`
                    'rewrite': '?%1' + '&' + '&'.join(cgi),
                },
                {
                    'split': 'url',
                    'regexp': r'^([^?]*)$',  # does not contain `?`
                    'rewrite': '%1?' + '&'.join(cgi),
                },
            ]
        }
    )]


def _default_section(section_name, host, backends):
    balancer_options = make_balancer_options(backends)
    balancer_options.update({
        'balancer_type': 'rr',
        'policies': OrderedDict([
            ('unique_policy', {})
        ]),
    })

    return (
        section_name,
        {'match_fsm': OrderedDict([('host', host)])},
        [
            (Modules.Regexp, [
                ('default', {}, [
                    (Modules.Balancer2, balancer_options),
                ]),
            ]),
        ]
    )


def _section_with_rewrite(section_name, backends, match_regexp, rewrite_regexp, rewrite, cgi):
    modules = [
        (Modules.Rewrite, {
            'actions': [
                {
                    'split': 'url',
                    'regexp': rewrite_regexp,
                    'rewrite': rewrite,
                },
            ]
        }),
    ] + _add_cgi(*cgi) + [
        (Modules.Balancer, {
            'backends': backends,
            'proxy_options': OrderedDict([
                ('backend_timeout', '60s'),
                ('fail_on_5xx', False),
            ]),
        }),
    ]
    return (
        section_name,
        {'match_fsm': OrderedDict([('URI', match_regexp)])},
        modules,
    )


def make_balancer_options(backends):
    return {
        'backends': backends
    }


def _sd_section_with_rewrite(section_name, match_regexp, rewrite_regexp, rewrite, cgi, backends):
    balancer_options = make_balancer_options(backends)

    balancer_options.update({
        'balancer_type': 'rr',
        'policies': OrderedDict([
            ('unique_policy', {})
        ]),
        'proxy_options': OrderedDict([
            ('backend_timeout', '60s'),
            ('fail_on_5xx', False),
        ]),
    })

    modules = [
        (Modules.Rewrite, {
            'actions': [
                {
                    'split': 'url',
                    'regexp': rewrite_regexp,
                    'rewrite': rewrite,
                },
            ]
        }),
    ] + _add_cgi(*cgi) + [
        (Modules.Balancer2, balancer_options),
    ]

    return (
        section_name,
        {'match_fsm': OrderedDict([('URI', match_regexp)])},
        modules,
    )


def _ctrl():
    def _section1(user, name, backends, match_regexp_template='/{}/{}(/.*)?'):
        cgi = ['root-path=/{}/{}'.format(user, name)]
        match_regexp = match_regexp_template.format(user, name)
        rewrite_regexp = '/{}/{}(/.*)$'.format(user, name)
        section_name = '{}_{}'.format(user, name)
        return _sd_section_with_rewrite(
            section_name,
            match_regexp,
            rewrite_regexp,
            '%1',
            cgi,
            backends=backends,
        )

    def _section1_test(user, name, backends, match_regexp_template='/{}/{}(/.*)?'):
        cgi = ['root-path=/{}/{}'.format(user, name)]
        match_regexp = match_regexp_template.format(user, name)
        rewrite_regexp = '/{}/{}(/.*)?$'.format(user, name)
        section_name = '{}_{}'.format(user, name)
        return _section_with_rewrite(section_name, backends, match_regexp, rewrite_regexp, '%1', cgi)

    def _section2(user, name, backends, match_regexp_template='/{}/{}(/.*)?'):
        cgi = ['root-path=/{}'.format(user)]
        match_regexp = match_regexp_template.format(user, name)
        rewrite_regexp = '/{}(/.*)$'.format(user)
        section_name = '{}_{}'.format(user, name)
        return _section_with_rewrite(section_name, backends, match_regexp, rewrite_regexp, '%1', cgi)
        # return _section_with_rewrite(user, backends, match_regexp, rewrite_regexp, '%1', cgi)

    return (
        'ctrl',
        {'match_fsm': OrderedDict([('host', BETA_CONTROLLER_HOST)])},
        [
            (Modules.Regexp, [
                _section1('video', 'prod', VIDEO_PROD_BACKENDS),

                _section1('video', 'prod-man', VIDEO_MAN_BACKENDS),
                _section1('video', 'prod-sas', VIDEO_SAS_BACKENDS),
                _section1('video', 'prod-vla', VIDEO_VLA_BACKENDS),
                _section1('video', 'pip', VIDEO_PIP_BACKENDS),
                _section1('video', 'multi_meta', VIDEO_MULTIMETA_BACKENDS),
                _section1('video', 'multi_beta', VIDEO_MULTIBETA_BACKENDS),
                _section1('video', 'multi_quick', VIDEO_MULTIQUICK_BACKENDS),

                _section1('v', 'video', VIDEO_VIEWER_BACKENDS),
                _section1('v', 'img', IMG_VIEWER_BACKENDS),
                _section1('v', 'rim', RIM_VIEWER_BACKENDS),
                _section1('v', 'imgcommercial', IMG_COMMERCIAL_VIEWER_BACKENDS),

                _section1('img', 'beta1', IMG_BETA1_BACKENDS),
                _section1('img', 'beta2', IMG_BETA2_BACKENDS),
                _section1('img', 'thumb', IMG_THUMB_BACKENDS),
                _section1('img', 'multi_meta', IMG_MULTIMETA_BACKENDS),
                _section1('img', 'multi_beta', IMG_MULTIBETA_BACKENDS),
                _section1('img', 'multi_quick', IMG_MULTIQUICK_BACKENDS),

                _section1('img', 'prod', IMG_PROD_BACKENDS),
                _section1('img', 'pip', IMG_PIP_BACKENDS),
                _section1('img', 'prod-man', IMG_MAN_BACKENDS),
                _section1('img', 'prod-sas', IMG_SAS_BACKENDS),
                _section1('img', 'prod-vla', IMG_VLA_BACKENDS),
                _section1('img', 'oldprod', IMAGES_OLDPROD_BACKENDS),

                _section1('rim', 'prod', RIM_PROD_BACKENDS),
                _section1('rim', 'pip', RIM_PIP_BACKENDS),
                _section1('rim', 'man', RIM_MAN_BACKENDS),
                _section1('rim', 'sas', RIM_SAS_BACKENDS),
                _section1('rim', 'vla', RIM_VLA_BACKENDS),

                _section1('imgcommercial', 'prod', IMG_COMMERCIAL_MAIN_BACKENDS),
                _section1('imgcommercial', 'pip', IMG_COMMERCIAL_PIP_BACKENDS),
                _section1('imgcommercial', 'man', IMG_COMMERCIAL_MAN_BACKENDS),
                _section1('imgcommercial', 'sas', IMG_COMMERCIAL_SAS_BACKENDS),
                _section1('imgcommercial', 'vla', IMG_COMMERCIAL_VLA_BACKENDS),

                _section1('web', 'prod', WEB_PROD_BACKENDS),
                _section1('web', 'prod-man', WEB_MAN_BACKENDS),
                _section1('web', 'prod-sas', WEB_SAS_BACKENDS),
                _section1('web', 'prod-sas-platinum', WEB_SAS_PLATINUM_BACKENDS),
                _section1('web', 'prod-vla', WEB_VLA_BACKENDS),
                _section1('web', 'pip', WEB_PIP_BACKENDS),
                _section1('web', 'multi_beta', WEB_MULTIBETA_BACKENDS),
                _section1('web', 'multi_meta', WEB_MULTIMETA_BACKENDS),
                _section1('web', 'multiint', WEB_MULTIINT_BACKENDS),
                _section1('web', 'multi_full', WEB_MULTIFULL_BACKENDS),
                _section1('web', 'oldprod', WEB_OLDPROD_BACKENDS),

                _section1('callisto', 'beta', CALLSITO_BETA_BACKENDS),
                _section1('callisto', 'pumpkin', CALLSITO_PUMPKIN_BACKENDS),
                _section1('callisto', 'man', CALLISTO_MAN_BACKENDS),
                _section1('callisto', 'sas', CALLISTO_SAS_BACKENDS),
                _section1('callisto', 'vla', CALLISTO_VLA_BACKENDS),

                _section1('saas', 'SaasFrozenBeta', SAAS_FROZEN_BETA),
                _section1('saas', 'SaasNewsBeta', SAAS_NEWS_BETA),
                _section1('saas', 'news_meta', SAAS_NEWS_MMETA_BETA),
            ]),
        ]
    )


def get_modules():
    return [
        _default_section('callisto-viewer', CALLISTO_VIEWER_HOST, CALLISTO_VIEWER_BACKENDS),
        _default_section('jv-clusterstate', JUPITER_VIEWER_HOST, WEB_VIEWER_BACKENDS),

        _ctrl(),
    ]
