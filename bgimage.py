from os import path
from requests import get
from glob import iglob
from pathlib import Path
from validators import url
import socket
import imghdr
import subprocess

class ImagePath(object):
    
    def __init__(self, parsers=[]):
        
        self.parsers = parsers
        self._parsers = [
            {"parser": self.ParseDirectoryPath, "description": "directory"},
            {"parser": self.ParseImagePath, "description": "image path"},
            {"parser": self.ParseImageUrl, "description": "image url"},
        ]
        self.fill={
            "background":"--bg",
            "center":"--bg-center",
            "fill":"--bg-fill",
            "scale":"--bg-scale",
            "seamless":"--bg-seamless",
            "tile":"--bg-tile",
            "max":"--bg-max"
        }


    def GetParsers(self):
        return self.parsers + self._parsers


    def ParseUri(self, uri):
        
        paths = []
        # loop user defined parsers first
        parsers = self.parsers + self._parsers
        for parser in parsers:
            paths = parser["parser"](uri)
            if paths != []:
                break

        return paths


    def ParseDirectoryPath(self, uri):
        paths = [] 
        if path.isdir(uri):
            pathobj = Path(uri)
            for p in iglob(str(pathobj) + "/*"):
                if path.isfile(p) and self.isimage(p):
                    paths.append(p)
        return paths


    def ParseImagePath(self, uri):
        if path.isfile(uri) and self.isimage(uri):
            return [uri]
        else:
            return []


    def ParseImageUrl(self, uri):
        if not url(uri) or not self.connected():
            return []
        tmp_path = "/home/spowell/.tmp"
        self.DownloadImage(uri, tmp_path)
        if self.isimage(tmp_path):
            return [uri]


    def DownloadImage(self, uri, fn):
        if path.isdir(path.dirname(path.abspath(fn))) and url(uri):
            with open(fn, "wb") as f:
                f.write(get(uri).content)


    def SetBackground(self, fn, fill):
        if fill not in self.fill:
            return False
        return subprocess.call(['feh', self.fill[fill], fn]) == 0


    def isimage(self, path):
        return bool(imghdr.what(path))


    def connected(self, host="8.8.8.8", port=53, timeout=2):
      try:
        socket.setdefaulttimeout(timeout)
        socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((host, port))
        return True
      except Exception as e:
        print e.message
        return False



    
        
