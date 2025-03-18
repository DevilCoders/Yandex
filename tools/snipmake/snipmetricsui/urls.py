from snipmetricsview.views import *
from settings import ROOT_DIRECTORY
from django.conf.urls.defaults import *

# Uncomment the next two lines to enable the admin:
from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    (r'^$', index_html),
    (r'^index.html$', index_html),
    (r'^queries.html$', queries_html),
    (r'^add_queries.html$', add_queries_html),
    (r'^add_engine.html$', add_engine_html),
    (r'^engines.html$', engines_html),
    (r'^metrics.html$', metrics_html),
    (r'^add_metric.html$', add_metric_html),
    (r'^add_metric_category.html$', add_metric_category_html),
    (r'^view_metrics.html$', view_metrics_html),
    (r'^snips_dump.html$', snips_dump_html),
    (r'^calc_metrics.html$', calc_metrics_html),
    (r'^tasks.html$', tasks_html),
    (r'^snippets.html$', snippets_html),
    (r'^snippets_diff.html$', snippets_diff_html),
    (r'^help.html$', help_html),
    (r'^queries.txt$', queries_txt),
    (r'^snippets.txt$', snippets_txt),
    (r'^metrics.txt$', view_metrics_html),
    (r'^snips_upload.html$', snips_upload_html),
    (r'^wizard.html$', wizard_html),
    (r'^clear_cache.html$', clear_cache_view),
    (r'^su/(?P<shortUrl>.*)$', short_url_view),
    (r'^css/(?P<path>.*)$', 'django.views.static.serve', {'document_root': ROOT_DIRECTORY + 'snipmetricsview/templates/css/'}),
    (r'^img/(?P<path>.*)$', 'django.views.static.serve', {'document_root': ROOT_DIRECTORY + 'snipmetricsview/templates/img/'}),
    (r'^scripts/(?P<path>.*)$', 'django.views.static.serve', {'document_root': ROOT_DIRECTORY + 'snipmetricsview/templates/scripts/'}),

    # Uncomment the admin/doc line below and add 'django.contrib.admindocs'
    # to INSTALLED_APPS to enable admin documentation:
    # (r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
     (r'^admin/', include(admin.site.urls)),
)
