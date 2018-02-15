import requests
from bs4 import BeautifulSoup
import re
from validators import url as check_url
from imgur import ImgurParser


class BlueShotsParser(object):

    imgur = ImgurParser()
    def get_images(self, url):

        if not check_url(url):
            return []

        try:
            response = requests.get(url)
        except requests.exceptions.ConnectionError:
            return []

        html = response.text

        soup = BeautifulSoup(html, "lxml")

        rel = ""

        if not check_url(url):
            return []

        imgur_link = None
        patt = "^https?://imgur\.com.+$"
        for link in soup.find_all("a"):
            m = re.search(patt, link['href'])
            if m:
                imgur_link = link['href']

        images = []
        
        if imgur_link:
            images = self.imgur.get_images(imgur_link)

        return images 


if __name__ == "__main__":
    b = BlueShotsParser()
    print b.get_images("http://blushots.weebly.com/ex-machina.html")
