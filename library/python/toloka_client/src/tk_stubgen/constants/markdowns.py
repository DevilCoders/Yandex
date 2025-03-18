from ..util import is_arcadia


skip_modules = [
    # Do not require markdown generation
    'toloka.__version__',
    'crowdkit.__version__',

    # Generation fails on missing __all__
    'crowdkit.datasets._base',
    'crowdkit.datasets._loaders',
]

if is_arcadia():
    skip_modules.extend([
        'toloka.metrics.jupyter_dashboard',  # missing jupyter-dashboard
        'crowdkit.aggregation.texts.text_summarization',  # missing transformers
        # TextSummarization requires transformers so we cannot import it in ArcadiaBuild. So we
        # cannnot find TextSummarization in the module, even though it can be found in __all__
        # which raises a sanity check error
        'crowdkit.aggregation',
        'crowdkit.metrics.data._classification',  # no nltk
    ])

# Modules that are known to generate incorrect markdowns
broken_modules = {}
