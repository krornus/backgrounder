#!/usr/bin/env python2

from pydbus import SessionBus

bus = SessionBus()
service = bus.get("com.krornus.dbus.Backgrounder")

def test():
    print "testing get test"
    reply = service.GetSetting("test")
    print reply

    print "testing interval = 20" 
    reply = service.SetSetting("interval", "20") 
    print reply

    print "testing interval = 30" 
    reply = service.SetSetting("interval", "30") 
    print reply

    print "testing get interval"
    reply = service.GetSetting("interval")
    print reply

    print "testing help"
    reply = service.Help()
    print reply

    print "testing set image on plaintext"
    reply = service.SetImagePath("/home/spowell/programming/python/backgrounder/bgserver.py")
    print reply

    print "testing set image on image"
    reply = service.SetImagePath("/home/spowell/pictures/backgrounds/aquatic_1.jpg")
    print reply

    print "testing get images"
    reply = service.ListImages()
    print reply

    print "testing append image on image"
    reply = service.AppendImagePath("/home/spowell/pictures/backgrounds/aquatic_1.jpg")
    print reply

    print "testing get images"
    reply = service.ListImages()
    print reply

    print "testing append image on directory"
    reply = service.AppendImagePath("/home/spowell/pictures/backgrounds/")
    print reply

    print "testing get images"
    reply = service.ListImages()
    print reply

    print "testing set image on image url"
    reply = service.SetImagePath("http://i.imgur.com/IjeQrKh.png")
    print reply

    print "testing get images"
    reply = service.ListImages()
    print reply

    print "testing save to ./save.out"
    reply = service.Save("/home/spowell/programming/python/backgrounder/save.out")
    print reply

    print "testing list settings"
    reply = service.ListSettings()
    print reply

    reply = service.SetImagePath("http://i.imgur.com/IjeQrKh.png")
    print reply

    reply = service.AppendImagePath("/home/spowell/pictures/backgrounds/aquatic_1.jpg")
    print reply

    reply = service.AppendImagePath("http://imgur.com/gallery/IiUWz")
    print reply

    reply = service.GetSetting("Paths").split(",")
    print reply

    service.SetSetting("Shuffle", "false")
    service.SetImagePath("http://imgur.com/gallery/IiUWz")
    print service.ListImages()
    print "\n"
    service.SetSetting("Shuffle", "true")
    print service.ListImages()
    print "\n"
    service.SetSetting("Shuffle", "false")
    print service.ListImages()
    print "\n"

    service.SetImagePath("http://imgur.com/gallery/IiUWz")
    print service.Next()
    print service.Previous()


    service.SetSetting("Writeback", "true")
    service.SetSetting("shuffle", "false")
    service.SetImagePath("http://imgur.com/gallery/IiUWz")
    service.SetSetting("Writeback", "false")
    service.SetSetting("shuffle", "true")


service.AppendImagePath("http://imgur.com/a/CrG6A")
service.SetSetting("shuffle", "true")
service.Next()
