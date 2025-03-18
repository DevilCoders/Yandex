import pytest
from ..util import is_arcadia


skip_modules = [
    # Do not require stub generation
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
    ])

# Modules that require "# type: ignore" during import
type_ignored_modules = [
    'jupyter_dash',
    'plotly',
    'kazoo',
    'dash',
]

# Modules that are known to generate incorrect stubs
broken_modules = {
    'toloka.util._managing_headers': pytest.mark.xfail(reason='builtins.ContextVar vs contextvars.ContextVar'),
}
