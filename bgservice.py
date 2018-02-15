class ImagePathParser(object):

    
    def __init__(self, parsers):
        
        self.parsers = parsers
        self._parsers = [
            {"parser": self.ParseDirectoryPath, "description": "directory"},
            {"parser": self.ParseImagePath, "description": "image path"},
            {"parser": self.ParseImageUrl, "description": "image url"},
        ]


    def AddParser(self, func, desc):
        self.parsers.append({"parser": func, "description": desc})
    
    
    def Parse


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
    
    def Reload(self):
    
