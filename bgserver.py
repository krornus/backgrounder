#!/usr/bin/env python2

from gi.repository import GLib
from pydbus import SessionBus

from glob import iglob
from os import path, stat
from bgconf import BackgroundConfig
from PIL import Image
from pathlib import Path
from validators import url
from requests import get
from StringIO import StringIO
from sys import stdout

import imghdr
import random

# Image path parsers
from bgimage import ImagePath
from imgur import ImgurParser

DBUS_INTERFACE = "com.krornus.dbus.Backgrounder"



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
                <method name='Pause'>
                    <arg type='b' name='success' direction='out'/>
                </method>
                <method name='Resume'>
                    <arg type='b' name='success' direction='out'/>
                </method>
                <method name='Reload'>
                    <arg type='b' name='success' direction='out'/>
                </method>
                <method name='Refresh'>
                    <arg type='b' name='success' direction='out'/>
                </method>
                <method name='Remove'>
                    <arg type='b' name='success' direction='out'/>
                </method>
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
                <method name='AppendSetting'>
                    <arg type='s' name='key' direction='in'/>
                    <arg type='s' name='value' direction='in'/>
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

    def __init__(self):
        # load settings from file
        self.paths = []
        self.active_paths = []
        self.index = 0

        callbacks = {
            "shuffle": self.RefreshActivePaths,
            "writeback": self.OnWriteBackChanged
        }
        self.settings = BackgroundConfig("config.ini", callbacks=callbacks)

        imgur = ImgurParser()
        parsers = [
            {"parser": imgur.get_images, "description": "imgur gallery"}
        ]

        self.img_handler = ImagePath(parsers)

        # this sets our active path and paths variables based on unexpaned uris
        for path in self.settings.get("Paths").split(","):
            self.AppendImagePath(path)


    def Help(self):
        return "Usage: background <command> [ args ... ]"

    
    def RefreshActivePaths(self):
        shuffle = self.settings.get_bool("Shuffle")

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

        self.UpdateBackground()


    def UpdateBackground(self):
        tmp_path = "/home/spowell/pictures/backgrounds/.tmp"

        if len(self.active_paths) == 0:
            return ""

        if self.index < 0:
            self.index = len(self.active_paths) - 1
        elif self.index >= len(self.active_paths):
            self.index = 0

        uri = self.active_paths[self.index] 
        if url(uri):
            self.img_handler.DownloadImage(uri, tmp_path)
            uri = tmp_path

        self.img_handler.SetBackground(uri, "max")
        return self.active_paths[self.index] 

        
    def OnWriteBackChanged(self):
        wb = self.settings.get_bool("Writeback")
        self.settings.writeback = wb

    
    def ShufflePaths(self, paths):
        shuffled = list(paths)
        random.shuffle(shuffled)
        return shuffled


    def Next(self):
        self.index += 1 
        return self.UpdateBackground()


    def Previous(self):
        self.index -= 1 
        return self.UpdateBackground()


    def Pause(self):
        return False


    def Resume(self):
        return False


    def Reload(self):
        return False


    def Refresh(self):
        return False


    def Remove(self):
        return False


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

    
    def AppendImagePath(self, uri):
        
        conf_paths = self.settings.get("Paths")
        if conf_paths:
            conf_paths += ","
    
        success = False
        
        paths = self.img_handler.ParseUri(uri)

        if not paths:
            self.RefreshActivePaths()
            return "argument must be one of the following: " + \
                str([x["description"] for x in self.img_handler.GetParsers()]) 
        else:
            # local copy uses expanded paths
            self.paths += paths
            # rc file should have non expanded paths for readability
            self.settings.set("Paths", conf_paths + uri)

            self.RefreshActivePaths()
            return ""

    
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


    def SetSetting(self, key, value):
        self.settings.set(key, value)


    def GetSetting(self, key):
        return str(self.settings.get(key))


    def Exit(self):
        loop.quit()


if __name__ == "__main__":

    bus = SessionBus()
    bus.publish(DBUS_INTERFACE, BackgrounderService())

    loop = GLib.MainLoop()
    loop.run()
