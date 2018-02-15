
def bool(s):
    s = s.lower()
    return s == "true" or s == "false"


def int(s):
    try:
        int(s, 0)
        return True
    except:
        return False


def float(s):
    try:
        float(s)
        return True
    except:
        return False
        

def bgmode(s):
    modes = [
        "background",
        "center",
        "fill",
        "scale",
        "seamless",
        "tile",
        "max"
    ]

    return s in modes

def filedir(s):
    
    if not s:
        return False
    if not path.isdir(path.dirname(s)):
        return False 
    elif path.isdir(s):
        return False
    return True
