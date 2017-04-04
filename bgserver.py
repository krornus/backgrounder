#!/usr/bin/env python2

from gi.repository import GLib
from pydbus import SessionBus

from os import path, remove, access, R_OK, W_OK
from bgconf import BackgroundConfig
from validators import url
from StringIO import StringIO
from threading import Timer

import random

# Image path parsers
from bgimage import ImagePath
from imgur import ImgurParser


class BackgrounderService(object):

    """
        <node>
            <interface name='com.krornus.dbus.Backgrounder'>
                <method name='Help'>
                    <arg type='s' name='usage' direction='out'/>
                </method>
                <method name='Next'>
                    <arg type='s' name='fn' direction='out'/>
                </method>
                <method name='Previous'>
                    <arg type='s' name='fn' direction='out'/>
                </method>
                <method name='Current'>
                    <arg type='s' name='fn' direction='out'/>
                </method>
                <method name='Pause'/>
                <method name='Resume'/>
                <method name='Reload'/>
                <method name='Refresh'/>
                <method name='Remove'/>
                <method name='Undo'>
                    <arg type='b' name='success' direction='out'/>
                </method>
                <method name='ListSettings'>
                    <arg type='s' name='usage' direction='out'/>
                </method>
                <method name='ListImages'>
                    <arg type='s' name='usage' direction='out'/>
                </method>
                <method name='AppendImagePath'>
                    <arg type='s' name='uri' direction='in'/>
                    <arg type='s' name='error' direction='out'/>
                </method>
                <method name='SetImagePath'>
                    <arg type='s' name='path' direction='in'/>
                    <arg type='s' name='error' direction='out'/>
                </method>
                <method name='Save'>
                    <arg type='s' name='path' direction='in'/>
                    <arg type='s' name='success' direction='out'/>
                </method>
                <method name='SaveImageList'>
                    <arg type='s' name='path' direction='in'/>
                    <arg type='s' name='success' direction='out'/>
                </method>
                <method name='LoadImageList'>
                    <arg type='s' name='path' direction='in'/>
                </method>
                <method name='SetSetting'>
                    <arg type='s' name='key' direction='in'/>
                    <arg type='s' name='value' direction='in'/>
                </method> 
                <method name='GetSetting'>
                    <arg type='s' name='key' direction='in'/>
                    <arg type='s' name='result' direction='out'/>
                </method> 
                <method name='Exit'/>
            </interface>
        </node>
    """


    def __init__(self, loop):

        self.loop = loop
        # callbacks on setting changed
        self.callbacks = {
            "shuffle": self.RefreshActivePaths,
            "writeback": self.OnWriteBackChanged,
            "cycle": self.OnCycleChanged,
            "interval": self.ResetTimer,
            "imagelist": self.LoadImageListHelper
        }

        imgur = ImgurParser()
        parsers = [
            {"parser": imgur.get_images, "description": "imgur gallery"}
        ]

        self.img_handler = ImagePath(parsers)

        self.cycle_timer = None

        self.Reload()
        self.OnCycleChanged()


    def Help(self):
        return "Usage: background <command> [ args ... ]"

    
    def RefreshActivePaths(self):
        shuffle = self.settings.get_bool("Shuffle")
        self.paths = list(set(self.paths))
        self.paths = reduce(lambda l, x: l.append(x) or l if (x not in l and x.strip()) else l, self.paths, [])


        if shuffle:
            self.active_paths = self.ShufflePaths(self.paths)
        else:
            if self.index > 0 and self.index < len(self.active_paths):
                current = self.active_paths[self.index]
                # get our current index
                if current in self.paths:
                    self.index = self.paths.index(current)
            else:
                self.index = 0 
            self.active_paths = self.paths


    def UpdateBackground(self):
        tmp_path = "/home/spowell/pictures/backgrounds/.tmp"
        rm = False

        if len(self.active_paths) == 0:
            return ""

        if self.index < 0:
            self.index = len(self.active_paths) - 1
        elif self.index >= len(self.active_paths):
            # reshuffle if set
            self.index = 0
            self.RefreshActivePaths()

        uri = self.active_paths[self.index] 
        if url(uri):
            self.img_handler.DownloadImage(uri, tmp_path)
            uri = tmp_path
            rm = True

        self.img_handler.SetBackground(uri, self.settings.get("fill"))

        if rm:
            remove(tmp_path)

        return self.active_paths[self.index] 

        
    def OnWriteBackChanged(self):
        wb = self.settings.get_bool("Writeback")
        self.settings.writeback = wb


    def OnCycleChanged(self):

        cycle = self.settings.get_bool("cycle")
        interval = self.settings.get_float("interval")
        alive = self.cycle_timer and self.cycle_timer.is_alive()

        if interval < 1:
            self.settings.set("interval", 1)
            return

        if cycle and not alive:
            self.cycle_timer = Timer(interval, self.Next)
            self.cycle_timer.start()
        if not cycle and alive:
            self.cycle_timer.cancel()
    
    
    def ShufflePaths(self, paths):
        shuffled = list(paths)
        random.shuffle(shuffled)
        return shuffled


    def Next(self):
        self.index += 1 
        self.ResetTimer()
        return self.UpdateBackground()


    def Previous(self):
        self.index -= 1 
        self.ResetTimer()
        return self.UpdateBackground()


    def Remove(self):
        item = None
        if self.index >= 0 and self.index < len(self.active_paths):
            item = self.active_paths[self.index]
            del self.active_paths[self.index]
        if self.index >= 0 and self.index < len(self.paths) \
                and item and item in self.paths:
            self.paths.remove(item)
        self.ResetTimer()
        self.UpdateBackground()


    def Current(self):
        if self.index > 0 and self.index < len(self.active_paths):
            return self.active_paths[self.index]
        return ""


    def Pause(self):
        self.settings.set("cycle", "false")


    def Resume(self):
        self.settings.set("cycle", "true")


    def Reload(self):
        # load settings from file
        self.paths = []
        self.active_paths = []
        self.index = 0

        self.settings = self.load_config()

        # append adds to our configs current paths, we want to parse
        # but dont want to append config path to itself
        paths = self.settings.get("Paths").split(",")
        self.settings.set("Paths", "")
        # this sets our active path and paths variables based on unexpaned uris
        self.AppendImagePath(paths)
        self.UpdateBackground()
        self.LoadImageList()
        self.ResetTimer()


    def Refresh(self):
        
        # reload from config, save paths 
        paths = self.settings.get("paths")
        paths = self.paths
        index = self.index
        active_paths = self.active_paths
        self.settings.set("paths", paths)
        self.settings = self.load_config()
        self.RefreshActivePaths()
        self.paths = paths
        self.active_paths = active_paths
        self.index = index
        self.UpdateBackground()

    
    def ResetTimer(self):
        if self.cycle_timer and self.cycle_timer.is_alive():
            self.cycle_timer.cancel()
        self.cycle_timer = None
        self.OnCycleChanged()


    def Undo(self):
        return False


    def ListSettings(self):
        io = StringIO()
        self.settings.config.write(io)
        
        settings = io.getvalue()

        io.close()

        return settings


    def ListImages(self):
        return str(self.active_paths)

    
    def AppendImagePath(self, uris, parsed=False):
        
        if not isinstance(uris, list):
            uris = [uris]

        conf_paths = self.settings.get("Paths")
        if conf_paths:
            conf_paths += ","
    
        message = ""
        acc = []

        # can't check every image in an imgur generated list
        if not parsed:
            for uri in uris:
                uri = uri.strip()
                paths = self.img_handler.ParseUri(uri)
                if not paths:
                    message += "invalid path: '{}'\n".format(uri)
                else:
                    acc += paths
        else:
            acc = uris

        self.paths += acc
        self.settings.set("Paths", conf_paths + ",".join(uris))
        self.RefreshActivePaths()

        if message:
            message += "path must be one of: {}".format([x['description'] for x in self.img_handler.GetParsers()])

        return message

    
    # TODO add restore functionality if append fails?
    def SetImagePath(self, arg):
        self.paths = []
        self.settings.set("Paths", "")
        return self.AppendImagePath(arg) 


    def Save(self, fn):
        if self.settings.save(fn):
            return ""
        else:
            return "no such directory '{}'".format(path.dirname(path.abspath(fn)))


    def SaveImageList(self, fn):
        
        apath = ""

        if not fn:
            fn = self.settings.get("imagelist")

        if fn:
            apath = path.abspath(path.expanduser(fn))

        clobber = self.settings.get_bool("overwrite")

        # checked in valid path, but i want a more verbose error
        if path.isfile(apath) and not clobber:
            return "invalid file: '{}',\n\toverwrite is set to false.".format(apath)
        if not self.valid_path(apath, clobber=clobber, create=True, 
                               directory=False, read=False, write=True):
            return "invalid file: '{}'".format(apath)

        #TODO: access write wasnt working, not returning this error atm
        if not access(apath, W_OK):
            print "cannot write to file '{}'".format(apath)

        with open(apath, "w") as f:
            for line in self.paths:
                f.write(line + "\n")

        return apath


    def LoadImageList(self, fn=None):
        
        if not fn:
            self.LoadImageListHelper()
        else:
            self.settings.set("imagelist", fn)


    def LoadImageListHelper(self, fn=None):
        apath = "" 

        if not fn:
            fn = self.settings.get("imagelist")

        if fn:
            apath = path.abspath(path.expanduser(fn))

        if not access(apath, R_OK):
            return "cannot read file '{}'".format(apath)
        
        paths = []
        with open(apath,"r") as f:
            paths = f.read().split("\n")
    
        # trusting a loaded file to be only image paths or urls
        # dont want to re parse a 12000 img imgur gallery and check every image
        self.AppendImagePath(paths, parsed=True)
        self.settings.set("paths", "")

        return apath


    def SetSetting(self, key, value):
        self.settings.set(key, value)


    def GetSetting(self, key):
        return str(self.settings.get(key))


    def Exit(self):
        #kill timer thread
        self.Pause()
        self.loop.quit()

    def valid_path(self, fn, create=True, clobber=False, directory=False, read=True, write=False):
        if not create and not clobber and not read:
            return False
        if not read and not write:
            return False

        if not fn:
            return False 
        if not path.isdir(path.dirname(fn)):
            return False 
        elif not directory and path.isdir(fn):
            return False
        elif path.isfile(fn) and not clobber and not read:
            return False
        elif not create and not path.isfile(fn):
            return False
        return True

    def load_config(self):
        return BackgroundConfig(
            [path.expanduser("~/.backgroundrc"), "/etc/backgroundrc"], 
            callbacks=self.callbacks)


def run(interface, service, loop):
    bus = SessionBus()
    bus.publish(interface, service)
    loop.run()

if __name__ == "__main__":
    DBUS_INTERFACE = "com.krornus.dbus.Backgrounder"
    loop = GLib.MainLoop()
    background = BackgrounderService(loop)

    try:
        run(DBUS_INTERFACE, background, loop)
    except KeyboardInterrupt:
        # won't exit properly due to threads from standard interrupt
        background.Exit()
    except Exception as e:
        print e.message
        background.Exit()
