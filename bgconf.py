from os import path
from sys import stderr
from ConfigParser import ConfigParser

class BackgroundConfig(object):


    def __init__(self, cfg=None, callbacks={}, defaults={}, types={}, writeback=False): 
        # TODO:  add section support
        self.section = "Main"
        # make all keys lowercase
        self.writeback = writeback
        self.types = types
        self.defaults = defaults

        self.callbacks = {}
        # make keys lowercase
        for k,v in callbacks.iteritems():
            self.callbacks[k.lower()] = v

        if cfg:
            # ConfigParser already supports this, but 
            # I only want the first item to be parsed
            if not isinstance(cfg,list):
                cfg = [cfg]

            success = False
            for fn in cfg:
                if path.isfile(fn):
                    self.config = self.parse(cfg, self.defaults)
                    self.filename = fn
                    success = True
                    break
            if not success:
                self.config = self.parse([], self.defaults)
        else:
             self.config = self.parse(cfg, self.defaults)

    
    def save(self, fn=None):
        if not fn and self.filename:
            fn = self.filename
        elif not fn and not self.filename:
            return False

        if path.isdir(path.dirname(path.abspath(fn))):
            with open(fn, "wb") as cfg:
                self.config.write(cfg)
            return True
        else:
            return False


    def set(self, key, value):

        if key in self.types and not self.types[key](value):
            return False

        key = key.lower()

        if not self.config.has_section(self.section):
            self.config.add_section(self.section)

        self.config.set(self.section, key, value)

        if key in self.callbacks:
            self.callbacks[key]()

        return True
        

    def get(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            return self.config.get(self.section, key, raw=True)
        return ""


    def get_int(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            try:
                return self.config.getint(self.section, key)
            except:
                return self.defaults.get(key, int())
        return int() 


    def get_float(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            try:
                return self.config.getfloat(self.section, key)
            except:
                return self.defaults.get(key, float())
        return float()


    def get_bool(self, key):
        key = key.lower()
        if self.config.has_section(self.section) \
           and self.config.has_option(self.section, key):
            try:
                return self.config.getboolean(self.section, key)
            except:
                return self.defaults.get(key, bool())
        return bool() 


    def parse(self, cfg, defaults):
        parser = ConfigParser(defaults)
        parser.read(cfg)

        return parser




