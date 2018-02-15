from os import path, access, W_OK

def validate_path(fn, mode, directory=False, create=True):

    fn = expandpath(fn)

    if mode != W_OK:
        create = False

    if not directory and create:
        return access(fn, mode) or access(path.dirname(fn), mode)
    elif directory and create:
        return access(path.dirname(fn), mode) and not path.exists(fn)
    elif not directory and not create:
        return access(fn, mode)
    elif directory and not create:
        return False


def expandpath(fn):
    return path.abspath(path.expanduser(fn))


