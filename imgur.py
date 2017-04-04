import requests
import argparse
import json
from bs4 import BeautifulSoup
import re
from validators import url as check_url



class ImgurParser(object):

    def __init__(self, min_height=0, min_width=0, 
                         max_height=-1, max_width=-1, 
                         height=-1, width=-1):
        self.min_height = min_height
        self.min_width = min_width
        self.max_height = max_height
        self.max_width = max_width
        self.height = height
        self.width = width

        self.routes = [
          "^https?://imgur.com/gallery/([^/]+)$",
          "^https?://imgur.com/r/[^/]+/([^/]+)$",
          "^https?://imgur.com/([^/]+)$",
          "^https?://imgur.com/t/[^/]+/([^/]+)$",
          "^https?://imgur.com/a/([^/]+)$",
          "^https?://imgur.com/([^/]+)$",
        ]
    

    def get_images(self, url):

        rel = ""

        if not check_url(url):
            return []

        for route in self.routes:
            m = re.search(route, url)
            if m:
                rel = m.group(1)

        if not rel:
            return []

        gallery_base="http://imgur.com/ajaxalbums/getimages/{}/hit.json"

        response = requests.get(url)
        html = response.text

        soup = BeautifulSoup(html, "lxml")
        images=[]
      

        gallery_url = gallery_base.format(rel)
        gallery_base.format(rel)
        json_res = requests.get(gallery_url)
        gallery = json.loads(json_res.text)
        
        if gallery['data']:
            if 'images' in gallery['data']:
                images=gallery['data']['images']
            images=[ 'http://i.imgur.com/'+x['hash']+x['ext'] for x in images ]
        else:
            images = []
            for img in soup.find_all("div", {"class":"post-image-container"}):
                soup=BeautifulSoup(str(img), "lxml")
                image=soup.find("img")
                if image:
                    images.append("http:"+image["src"])
                elif img:
                    #slow last resort
                    images+=get_images("http://i.imgur.com/" + img['id'])

        return images 


