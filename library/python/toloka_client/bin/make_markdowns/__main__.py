try:
    from library.python.toloka_client.src.tk_stubgen.makers.make_markdowns import main  # noqa
except ImportError:
    import os
    import sys
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src')))
    from tk_stubgen.makers.make_markdowns import main

if __name__ == '__main__':
    main()
