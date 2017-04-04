from os import path
from sys import stderr
from ConfigParser import ConfigParser

class BackgroundConfig(object):

    def __init__(self, cfg=None, callbacks={}, writeback=False): 
        # TODO:  add section support
        self.section = "Main"
        self.filename = cfg
        # make all keys lowercase
        self.callbacks = dict((k.lower(), v) for k,v in callbacks.iteritems())
        self.writeback = writeback
        if cfg:
            self.config = self.parse(cfg)
            self.config._sections.append = False

    
    def save(self, fn=None):
        if not fn:
            fn = self.filename
        if path.isdir(path.dirname(path.abspath(fn))):
            with open(fn, "wb") as cfg:
                self.config.write(cfg)
            return True
        else:
            return False


    def set(self, key, value):
        key = key.lower()
        self.config.set(self.section, key, value)

        if key in self.callbacks:
            self.callbacks[key]()
        
        if self.writeback:
            self.save()


    def get(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            return self.config.get(self.section, key)
        return ""


    def get_int(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            return self.config.getint(self.section, key)
        return False


    def get_float(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            return self.config.getfloat(self.section, key)
        return False


    def get_bool(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            return self.config.getboolean(self.section, key)
        return False


    def parse(self, cfg):
        if not path.exists(cfg):
            stderr.write("no such file or directory: {}".format(cfg))
            return False
        elif path.isdir(cfg):
            stderr.write("cannot parse {0}:\n\t{0} is a directory!".format(cfg))
            return False
        else:
            parser = ConfigParser()
            parser.read(cfg)

            return parser




