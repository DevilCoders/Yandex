import os
import errno


def setup_bin_dir(flatten_all_data=False):
    """
    TODO fix tests which are using this module, for more info see DEVTOOLS-6940
    """
    import yatest.common

    if not hasattr(setup_bin_dir, 'path'):
        build_root = os.path.normpath(yatest.common.build_path())
        bin_dir = yatest.common.build_path('bin')
        if not os.path.exists(bin_dir):
            os.makedirs(bin_dir)

        for root, dirs, filenames in os.walk(build_root, followlinks=False):
            root = os.path.normpath(root)
            if root == build_root:
                for dirname in ['bin', 'sandbox-storage', 'canondata_storage', 'sandbox-storage']:
                    if dirname in dirs:
                        dirs.remove(dirname)
            for f in filenames:
                src = os.path.join(root, f)
                dst = os.path.join(bin_dir, f)
                if os.path.isfile(src) and (flatten_all_data or os.access(src, os.X_OK)):
                    try:
                        os.symlink(src, dst)
                    except OSError as e:
                        if getattr(e, 'errno', 0) != errno.EEXIST:
                            raise

        setattr(setup_bin_dir, 'path', bin_dir)

    return setup_bin_dir.path
