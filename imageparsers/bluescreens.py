import requests
from bs4 import BeautifulSoup
import re
from validators import url as check_url
from imgur import ImgurParser


class BlueScreensParser(object):

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

        selector = "div a[href] img"
        for x in soup.select(selector):
            print x.parent['href']

        return [ x['href'] for x in soup.select(selector) if check_url(x.parent['href']) ]


if __name__ == "__main__":
    b = BlueScreensParser()
    for url in b.get_images("http://www.bluscreens.net/cul-de-sac.html"):
        print url
