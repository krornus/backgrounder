import requests
from urllib import quote_plus as encode
from bs4 import BeautifulSoup
import re
from validators import url as check_url
import cStringIO
from PIL import Image


class GoogleParser(object):

    def get_images(self, url):
        
        levels = 1

        formats =  re.compile("jpg|jpeg|tif|gif|png|bmp|raw")

        if not check_url(url) or "google.com" not in url:
            return []

        try:
            response = requests.get(url)
        except requests.exceptions.ConnectionError:
            return []

        html = response.text

        soup = BeautifulSoup(html, "lxml")

        rel = ""
        links = []
        for res in soup.find_all("div", { "class" : "g" } ):
            for link in res.find_all("cite"):
                link = "".join([str(x) for x in link.contents])
                link = link.replace("<b>","").replace("</b>", "")
                links.append(link)

        for x in range(levels):
            links = set(self.expand_links(links))
        
        images = []

        for i in links:
            for img in self.images(i):
                try:
                    inp = requests.get(img, timeout=1, stream=True)
                    if inp.status_code == 200:
                        images.append(img)
                except:
                    continue

            if len(images) > 50:
                break
        
        return images 


    def images(self, url):

        ignore = re.compile("facebook|twitter|pinterest|irc|mailto")

        if not check_url(url) or ignore.search(url):
            return []

        try:
            response = requests.get(url, timeout=1)
        except: 
            return []

        html = response.text

        soup = BeautifulSoup(html, "lxml")

        out = []
        
        patt = re.compile("(https?://[^/]+)")

        for link in soup.find_all("img"):
            if ignore.search(str(link)):
                continue
            if "src" in str(link):
                if link['src'].startswith("/"):
                    m = patt.search(url)
                    out.append(m.group(1) + link['src'])
                else:
                    out.append(link['src'])
        return out

    def get_links(self, url):

        ignore = re.compile("facebook|twitter|pinterest|irc|mailto")

        if not check_url(url) or ignore.search(url):
            return []

        try:
            response = requests.get(url, timeout=1)
        except: 
            return []

        html = response.text

        soup = BeautifulSoup(html, "lxml")

        out = []
        
        patt = re.compile("(https?://[^/]+)")

        for link in soup.find_all("a"):
            if ignore.search(str(link)):
                continue
            if "href" in str(link):
                if link['href'].startswith("/"):
                    m = patt.search(url)
                    out.append(m.group(1) + link['href'])
                else:
                    out.append(link['href'])
        return out

    def expand_links(self, links):
        out = []
        for link in links:
            out.extend(self.get_links(link)) 
        return out

    def dimensions(self, raw):
        f = cStringIO.StringIO(raw)
        im = Image.open(f)

        return im.size 


if __name__ == "__main__":
    g = GoogleParser()

    q = encode(raw_input("enter query"))
    print g.get_images("https://www.google.com/search?q=" + q)
