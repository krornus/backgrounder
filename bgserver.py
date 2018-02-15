#!/usr/bin/env python2

from gi.repository import GLib
from pydbus import SessionBus

from os import path, remove, access, R_OK, W_OK
from bgconf import BackgroundConfig
from validators import url
from StringIO import StringIO
from threading import Timer
from time import sleep

import random

# Image path parsers
from bgimage import ImagePath
from imageparsers.imgur import ImgurParser
from imageparsers.reddit import RedditParser
from imageparsers.blueshots import BlueShotsParser
from imageparsers.google import GoogleParser

import bgtypechecker as typecheck


class BackgrounderService(object):

    """
        <node>
            <interface name='com.krornus.dbus.Backgrounder'>
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
                <method name='Remove'>
                    <arg type='s' name='success' direction='out'/>
                </method>
                <method name='Undo'>
                    <arg type='s' name='success' direction='out'/>
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
                <method name='AddToImageList'>
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


    def __init__(self, loop, cfgs=None):

        print "start init"
        self.history = []
        self.loop = loop

        self.cfgs = cfgs
        print "start reload"
        self.Reload()
        print "finish reload"


    def Reload(self):

        imgur = ImgurParser()
        blueshots = BlueShotsParser()
        google = GoogleParser()

        parsers = [
            {
                "parser": google.get_images,
                "description": "google images"
            },
            {
                "parser": imgur.get_images,
                "description": "imgur gallery"
            },
            {
                "parser": blueshots.get_images,
                "description": "blueshots gallery"
            },
        ]

        self.cycle_timer = None
        self.settings = None
        # load settings from file
        self.paths = []
        self.active_paths = []
        self.index = 0

        self.settings = self.LoadConfig()

        tmp = path.expanduser(self.settings.get("tmpfile"))
        self.img_handler = ImagePath(tmp, parsers)


        if not self.settings:
            return False

        have_list = bool(self.settings.get("imagelist"))
        paths = self.settings.get("Paths").split(",")

        if have_list:
            # we will be appending paths in paths option
            # this will append the path to itself in the option
            self.settings.set("paths", "")
            self.LoadImageListHelper()

        # append adds to our configs current paths, we want to parse
        # but dont want to append config path to itself
        self.UpdateImagePath(paths, append=have_list)

        self.RefreshActivePaths()

        self.OnCycleChanged()
        self.OnWriteBackChanged()
        self.UpdateBackground()


    def LoadConfig(self):
        # callbacks on setting changed
        callbacks = {
            "shuffle": self.OnShuffleChanged,
            "writeback": self.OnWriteBackChanged,
            "cycle": self.OnCycleChanged,
            "interval": self.ResetTimer,
            "imagelist": self.LoadImageListHelper
        }

        # conf defaults
        defaults = {
            "shuffle": True,
            "interval": 60.0,
            "cycle": True,
            "writeback": False,
            "overwrite": False,
            "mode": "max",
            "tmpfile": "~/.backgrounder-tmp"
        }

        # conf types
        types = {
            "shuffle": typecheck.bool,
            "interval": typecheck.float,
            "cycle": typecheck.bool,
            "writeback": typecheck.bool,
            "overwrite": typecheck.bool,
            "mode": typecheck.bgmode,
            "tmpfile": typecheck.filedir
        }

        if not self.cfgs:
            self.cfgs = [path.expanduser("~/.backgroundrc"), "/etc/backgroundrc"]

        return BackgroundConfig(self.cfgs, callbacks=callbacks, defaults=defaults, types=types)


    # sets our actively used path based on shuffle changes
    def RefreshActivePaths(self):
        shuffle = self.settings.get_bool("Shuffle")

        unique = []
        for uri in self.paths:
            if uri not in unique and bool(uri.strip()):
                unique.append(uri)
        self.paths = unique

        if shuffle:
            self.active_paths = self.ShufflePaths(self.paths)
        else:
            if self.index >= 0 and self.index < len(self.active_paths):
                current = self.active_paths[self.index]
                # get our current index
                if current in self.paths:
                    self.index = self.paths.index(current)
            else:
                self.index = 0
            self.active_paths = self.paths


    def UpdateBackground(self):
        tmp_path = path.expanduser(self.settings.get("tmpfile"))
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
            if not self.img_handler.DownloadImage(uri, tmp_path):
                return ""
            uri = tmp_path
            rm = True

        if path.exists(uri):
            self.img_handler.SetBackground(uri, self.settings.get("fill"))

        if rm:
            remove(tmp_path)

        return self.active_paths[self.index]


    def OnShuffleChanged(self):
        self.RefreshActivePaths()
        self.UpdateBackground()


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

        success = ""

        undo = {
            'function': self.UndoRemove,
            'args': self.SavePathState()
        }

        item = None
        if self.index >= 0 and self.index < len(self.active_paths):
            item = self.active_paths[self.index]
            del self.active_paths[self.index]
        if self.index >= 0 and self.index < len(self.paths) \
                and item and item in self.paths:
            self.paths.remove(item)

        if undo['args']['writeback']:
            success = "writing change to:\n\t" + self.SaveImageList()

        self.history.append(undo)
        self.ResetTimer()
        self.UpdateBackground()

        return success


    def UndoRemove(self, args):

        success = self.UndoPathChange(args)
        if args['writeback']:
            success += "reverting list:\n\t" + self.SaveImageList()

        return success


    def Current(self):
        if self.index >= 0 and self.index < len(self.active_paths):
            return self.active_paths[self.index]
        return ""


    def Pause(self):
        self.settings.set("cycle", "false")


    def Resume(self):
        self.settings.set("cycle", "true")


    def Refresh(self):

        # reload from config, save paths
        paths = self.settings.get("paths")
        paths = self.paths
        index = self.index
        active_paths = self.active_paths
        self.settings.set("paths", paths)
        self.settings = self.LoadConfig(self.cfgs)
        self.RefreshActivePaths()
        self.paths = paths
        self.active_paths = active_paths
        self.index = index


    def ResetTimer(self):
        if self.cycle_timer and self.cycle_timer.is_alive():
            self.cycle_timer.cancel()
        self.cycle_timer = None
        self.OnCycleChanged()


    def Undo(self):
        action = self.history.pop()
        if action:
            return action['function'](action['args'])
        else:
            return "nothing to undo"


    def ListSettings(self):
        io = StringIO()
        self.settings.config.write(io)

        settings = io.getvalue()

        io.close()

        return settings


    def ListImages(self):
        return str(self.active_paths)

    def SavePathState(self):
        return {
            "paths": self.paths,
            "active": self.active_paths,
            "index": self.index,
            "path_setting": self.settings.get("paths"),
            "imagelist": self.settings.get("imagelist"),
            "writeback": self.settings.writeback and bool(self.settings.get("imagelist"))
        }


    def UndoPathChange(self, args):
        self.paths = args['paths']
        self.active_paths = args['active']
        self.index = args['index']
        self.settings.set("paths", args['path_setting'])
        self.settings.set("imagelist", args['imagelist'])
        self.UpdateBackground()
        return "image path list reverted"


    def GetSanitizedPaths(self, uris):

        res = []
        if uris:
            for uri in uris:
                paths = self.img_handler.ParseUri(uri.strip())
                res += paths
        return res


    def UpdateImagePath(self, uris, parsed=False, append=True):

        message = ""

        undo = {
            "function": self.UndoPathChange,
            "args":self.SavePathState()
        }


        if not append:
            self.paths = []
            self.settings.set("Paths", "")

        if not isinstance(uris, list):
            uris = [uris]

        conf_paths = self.settings.get("Paths")

        if conf_paths:
            conf_paths += ","

        acc = uris if parsed else self.GetSanitizedPaths(uris)

        self.history.append(undo)
        self.paths += acc

        # if append is false, the paths settings is now empty
        if undo['args']['writeback']:
            message += "writing change to:\n\t" + self.SaveImageList()
        # if there is not an image list, or we cannot write
        else:
            self.settings.set("Paths", conf_paths + ",".join(uris))

        return message


    def AppendImagePath(self, uris):
        res = self.UpdateImagePath(uris)
        self.RefreshActivePaths()
        self.UpdateBackground()
        return res


    def SetImagePath(self, arg):
        self.settings.set("imagelist", "")
        res = self.UpdateImagePath(arg, append=False)
        self.RefreshActivePaths()
        self.UpdateBackground()
        return res


    def Save(self, fn):
        if self.settings.save(fn):
            return ""
        else:
            return "no such directory '{}'".format(path.dirname(path.abspath(fn)))


    def SaveImageList(self, fn=None):

        apath = ""

        if not fn:
            fn = self.settings.get("imagelist")

        if fn:
            apath = path.abspath(path.expanduser(fn))
        else:
            return "no save path given"

        clobber = self.settings.get_bool("overwrite")

        # checked in valid path, but i want a more verbose error
        if path.isfile(apath) and not clobber:
            return "invalid file: '{}',\n\toverwrite is set to false.".format(apath)
        #TODO: access write wasnt working, not returning this error atm
        if not access(apath, W_OK):
            print "cannot write to file '{}'".format(apath)

        with open(apath, "w") as f:
            for line in self.paths:
                f.write(line + "\n")

        return apath


    def LoadImageList(self, fn=None):

        if not fn:
            self.settings.set("imagelist",
                self.settings.get("imagelist"))
            self.RefreshActivePaths()
            self.UpdateBackground()
        else:
            self.settings.set("imagelist", path.abspath(path.expanduser(fn)))
            self.RefreshActivePaths()
            self.UpdateBackground()


    def LoadImageListHelper(self, fn=None):

        apath = ""

        if not fn:
            fn = self.settings.get("imagelist")

        if fn:
            apath = path.abspath(path.expanduser(fn))
        else:
            return "no load path given"

        print "image list load helper: " + apath

        if not access(apath, R_OK):
            return "cannot read file '{}'".format(apath)

        paths = []
        with open(apath,"r") as f:
            paths = f.read().split("\n")

        # trusting a loaded file to be only image paths or urls
        # dont want to re parse a 12000 img imgur gallery and check every image
        self.UpdateImagePath(paths, parsed=True, append=False)
        self.settings.set("paths", "")

        print len(self.paths)
        return apath


    def SetSetting(self, key, value):
        undo = {
            "function": self.UndoSetting,
            "args": {
                "option": key,
                "value": self.settings.get(key)
            }
        }

        # get bool before we set, in case writeback is being set to false
        save = self.settings.writeback

        self.history.append(undo)
        self.settings.set(key, value)

        if save:
            self.settings.save()

    def GetSetting(self, key):
        return str(self.settings.get(key))


    def UndoSetting(self, args):
        self.settings.set(args['option'], args['value'])
        return "setting '{}' to '{}'".format(args['option'], args['value'])


    def Exit(self):
        #kill timer thread
        self.Pause()
        self.loop.quit()



def run():

    interface = "com.krornus.dbus.Backgrounder"
    loop = GLib.MainLoop()
    background = BackgrounderService(loop)

    try:
        bus = SessionBus()
        bus.publish(interface, background)
        loop.run()
    except KeyboardInterrupt:
        # won't exit properly due to threads from standard interrupt
        background.Exit()
    except Exception as e:
        print e.message
        background.Exit()

if __name__ == "__main__":
    run()

