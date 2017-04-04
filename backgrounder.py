#!/usr/bin/env python2

from pydbus import SessionBus
import argparse
import daemon
from bgserver import run


def append_nargs_range(low,high):
    class RequiredLength(argparse.Action):
        def __call__(self, parser, args, values, option_string=None):
            n = len(values)
            if n >= low and n <= high:
                curr = getattr(args, self.dest)
                if curr:
                    curr.append(values)
                    setattr(args, self.dest, curr)
                else:
                    setattr(args, self.dest, [values])
            else:
                err='argument "{f}" requires [{nmin} - {nmax}] arguments (inclusive)'.format(
                    f=self.dest,nmin=nmin,nmax=nmax)
                raise argparse.ArgumentTypeError(err)
    return RequiredLength


def handle_settings(settings):
    for s in settings:
        if len(s) == 1:
            print "Setting '{}' is set to '{}'".format(s[0], service.GetSetting(s[0]))
        else:
            print "Setting {} to {}".format(s[0], s[1])
            service.SetSetting(s[0], s[1])


def handle_set_images(images):
    for img in images:
        print img[0]
        res = service.SetImagePath(img[0])
        if res:
            print "Error: {}:\n\t{}".format(img[0],res)

        
def handle_append_images(images):
    for img in images:
        res = service.AppendImagePath(img[0])
        if res:
            print "Error: {}:\n\t{}".format(img[0],res)


if __name__ == "__main__":
    

    parser = argparse.ArgumentParser()
    parser.add_argument("--server", action="store_true", default=False)
    
    parser.add_argument("--setting", "-s", nargs="+", action=append_nargs_range(1,2))
    parser.add_argument("--list-settings", action="store_true") 
    parser.add_argument("--save", nargs="?", const="", default=False) 

    parser.add_argument("--next", "-n", action="store_true") 
    parser.add_argument("--current", "-c", action="store_true") 
    parser.add_argument("--previous", "-p", action="store_true") 
    parser.add_argument("--pause", action="store_true") 
    parser.add_argument("--resume", action="store_true") 
    parser.add_argument("--remove", action="store_true") 

    parser.add_argument("--reload", action="store_true") 
    parser.add_argument("--refresh", "-r", action="store_true") 

    parser.add_argument("--save-images", nargs="?", const="", default=False) 
    parser.add_argument("--load-images", nargs="?", const="", default=False) 
    parser.add_argument("--set-images", "-i", nargs="+", action="append") 
    parser.add_argument("--get-images", "-l", action="store_true") 
    parser.add_argument("--append-images", "-a", nargs="+", action="append") 

    parser.add_argument("--verbose", "-v", action="store_true") 
    parser.add_argument("--exit", "-q", action="store_true") 

    args = parser.parse_args()

    if args.server:
        run()
    
    bus = SessionBus()
    service = None
    try:
        service = bus.get("com.krornus.dbus.Backgrounder")
    except Exception as e:
        print "service not running" 
    if not service:
        exit(1)

    # refresh then set settings
    if args.reload:
        service.Reload()
    if args.refresh:
        service.Refresh()

    # set settings then get settings then continue
    if args.setting:
        handle_settings(args.setting)

    if args.list_settings:
        print service.ListSettings()

    # get images then set/append
    if args.get_images:
        print "Image list: "
        if args.verbose:
            print service.ListImages()
        else:
            print service.ListImages()[:200]

    # set images/append images then next/prev, save/load
    if args.set_images:
        handle_set_images(args.set_images)

    if args.append_images:
        handle_append_images(args.append_images)

    # current then next/prev
    if args.current:
        print service.Current()

    # next/prev then pause/resume
    if args.remove:
        service.Remove()
    if args.next:
        service.Next()
    elif args.previous:
        service.Previous()

    if args.pause:
        service.Pause()
    elif args.resume:
        service.Resume()

    if args.save != False:
        service.Save(args.save)

    # save then load
    if args.save_images != False:
        print service.SaveImageList(args.save_images)
    if args.load_images != False:
        service.LoadImageList(args.load_images)

    if args.exit:
        service.Exit()
