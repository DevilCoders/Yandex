PY23_LIBRARY()

OWNER(borman)

SRCDIR(contrib/python/luigi/luigi/static)

RESOURCE(
    visualiser/css/font-awesome.min.css /luigi/static/visualiser/css/font-awesome.min.css
    visualiser/css/luigi.css /luigi/static/visualiser/css/luigi.css
    visualiser/css/tipsy.css /luigi/static/visualiser/css/tipsy.css
    visualiser/fonts/FontAwesome.otf /luigi/static/visualiser/fonts/FontAwesome.otf
    visualiser/fonts/fontawesome-webfont.eot /luigi/static/visualiser/fonts/fontawesome-webfont.eot
    visualiser/fonts/fontawesome-webfont.svg /luigi/static/visualiser/fonts/fontawesome-webfont.svg
    visualiser/fonts/fontawesome-webfont.ttf /luigi/static/visualiser/fonts/fontawesome-webfont.ttf
    visualiser/fonts/fontawesome-webfont.woff /luigi/static/visualiser/fonts/fontawesome-webfont.woff
    visualiser/fonts/fontawesome-webfont.woff2 /luigi/static/visualiser/fonts/fontawesome-webfont.woff2
    visualiser/fonts/glyphicons-halflings-regular.eot /luigi/static/visualiser/fonts/glyphicons-halflings-regular.eot
    visualiser/fonts/glyphicons-halflings-regular.svg /luigi/static/visualiser/fonts/glyphicons-halflings-regular.svg
    visualiser/fonts/glyphicons-halflings-regular.ttf /luigi/static/visualiser/fonts/glyphicons-halflings-regular.ttf
    visualiser/fonts/glyphicons-halflings-regular.woff /luigi/static/visualiser/fonts/glyphicons-halflings-regular.woff
    visualiser/index.html /luigi/static/visualiser/index.html
    visualiser/js/graph.js /luigi/static/visualiser/js/graph.js
    visualiser/js/luigi.js /luigi/static/visualiser/js/luigi.js
    visualiser/js/test/graph_test.js /luigi/static/visualiser/js/test/graph_test.js
    visualiser/js/tipsy.js /luigi/static/visualiser/js/tipsy.js
    visualiser/js/visualiserApp.js /luigi/static/visualiser/js/visualiserApp.js
    visualiser/lib/AdminLTE/css/AdminLTE.min.css /luigi/static/visualiser/lib/AdminLTE/css/AdminLTE.min.css
    visualiser/lib/AdminLTE/css/skin-green-light.min.css /luigi/static/visualiser/lib/AdminLTE/css/skin-green-light.min.css
    visualiser/lib/AdminLTE/css/skin-green.min.css /luigi/static/visualiser/lib/AdminLTE/css/skin-green.min.css
    visualiser/lib/AdminLTE/js/app.min.js /luigi/static/visualiser/lib/AdminLTE/js/app.min.js
    visualiser/lib/URI/1.18.2/URI.js /luigi/static/visualiser/lib/URI/1.18.2/URI.js
    visualiser/lib/bootstrap-toggle/css/bootstrap-toggle.min.css /luigi/static/visualiser/lib/bootstrap-toggle/css/bootstrap-toggle.min.css
    visualiser/lib/bootstrap-toggle/js/bootstrap-toggle.min.js /luigi/static/visualiser/lib/bootstrap-toggle/js/bootstrap-toggle.min.js
    visualiser/lib/bootstrap3/css/bootstrap-theme.min.css /luigi/static/visualiser/lib/bootstrap3/css/bootstrap-theme.min.css
    visualiser/lib/bootstrap3/css/bootstrap.min.css /luigi/static/visualiser/lib/bootstrap3/css/bootstrap.min.css
    visualiser/lib/bootstrap3/js/bootstrap.min.js /luigi/static/visualiser/lib/bootstrap3/js/bootstrap.min.js
    visualiser/lib/d3/d3.min.js /luigi/static/visualiser/lib/d3/d3.min.js
    visualiser/lib/d3/dagre-d3.min.js /luigi/static/visualiser/lib/d3/dagre-d3.min.js
    visualiser/lib/datatables/css/jquery.dataTables.min.css /luigi/static/visualiser/lib/datatables/css/jquery.dataTables.min.css
    visualiser/lib/datatables/images/favicon.ico /luigi/static/visualiser/lib/datatables/images/favicon.ico
    visualiser/lib/datatables/images/sort_asc.png /luigi/static/visualiser/lib/datatables/images/sort_asc.png
    visualiser/lib/datatables/images/sort_asc_disabled.png /luigi/static/visualiser/lib/datatables/images/sort_asc_disabled.png
    visualiser/lib/datatables/images/sort_both.png /luigi/static/visualiser/lib/datatables/images/sort_both.png
    visualiser/lib/datatables/images/sort_desc.png /luigi/static/visualiser/lib/datatables/images/sort_desc.png
    visualiser/lib/datatables/images/sort_desc_disabled.png /luigi/static/visualiser/lib/datatables/images/sort_desc_disabled.png
    visualiser/lib/datatables/js/jquery.dataTables.min.js /luigi/static/visualiser/lib/datatables/js/jquery.dataTables.min.js
    visualiser/lib/jquery-1.10.0.min.js /luigi/static/visualiser/lib/jquery-1.10.0.min.js
    visualiser/lib/jquery-ui/css/images/animated-overlay.gif /luigi/static/visualiser/lib/jquery-ui/css/images/animated-overlay.gif
    visualiser/lib/jquery-ui/css/images/ui-bg_flat_0_aaaaaa_40x100.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_flat_0_aaaaaa_40x100.png
    visualiser/lib/jquery-ui/css/images/ui-bg_flat_75_ffffff_40x100.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_flat_75_ffffff_40x100.png
    visualiser/lib/jquery-ui/css/images/ui-bg_glass_55_fbf9ee_1x400.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_glass_55_fbf9ee_1x400.png
    visualiser/lib/jquery-ui/css/images/ui-bg_glass_65_ffffff_1x400.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_glass_65_ffffff_1x400.png
    visualiser/lib/jquery-ui/css/images/ui-bg_glass_75_dadada_1x400.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_glass_75_dadada_1x400.png
    visualiser/lib/jquery-ui/css/images/ui-bg_glass_75_e6e6e6_1x400.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_glass_75_e6e6e6_1x400.png
    visualiser/lib/jquery-ui/css/images/ui-bg_glass_95_fef1ec_1x400.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_glass_95_fef1ec_1x400.png
    visualiser/lib/jquery-ui/css/images/ui-bg_highlight-soft_75_cccccc_1x100.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-bg_highlight-soft_75_cccccc_1x100.png
    visualiser/lib/jquery-ui/css/images/ui-icons_222222_256x240.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-icons_222222_256x240.png
    visualiser/lib/jquery-ui/css/images/ui-icons_2e83ff_256x240.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-icons_2e83ff_256x240.png
    visualiser/lib/jquery-ui/css/images/ui-icons_454545_256x240.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-icons_454545_256x240.png
    visualiser/lib/jquery-ui/css/images/ui-icons_888888_256x240.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-icons_888888_256x240.png
    visualiser/lib/jquery-ui/css/images/ui-icons_cd0a0a_256x240.png /luigi/static/visualiser/lib/jquery-ui/css/images/ui-icons_cd0a0a_256x240.png
    visualiser/lib/jquery-ui/css/jquery-ui-1.10.3.custom.min.css /luigi/static/visualiser/lib/jquery-ui/css/jquery-ui-1.10.3.custom.min.css
    visualiser/lib/jquery-ui/js/jquery-ui-1.10.3.custom.min.js /luigi/static/visualiser/lib/jquery-ui/js/jquery-ui-1.10.3.custom.min.js
    visualiser/lib/jquery.slimscroll.min.js /luigi/static/visualiser/lib/jquery.slimscroll.min.js
    visualiser/lib/mustache.js /luigi/static/visualiser/lib/mustache.js
    visualiser/mockdata/dep_graph /luigi/static/visualiser/mockdata/dep_graph
    visualiser/mockdata/fetch_error /luigi/static/visualiser/mockdata/fetch_error
    visualiser/mockdata/task_list /luigi/static/visualiser/mockdata/task_list
    visualiser/test.html /luigi/static/visualiser/test.html
)

END()
