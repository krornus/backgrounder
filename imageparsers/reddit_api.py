import re
from urllib import quote
from bs4 import BeautifulSoup
from fake_useragent import UserAgent
import requests

class RedditApi:
    
    def __init__(self, url):
        html = self.open_url(url)
        self.soup = BeautifulSoup(html, "lxml")

    def open_url(self, url):
        ua = UserAgent()

        headers = {'User-Agent': ua.chrome}
        cookies = {}
        response = requests.get(url, headers=headers, cookies=cookies)

        self.url = response.url

        return response.text

    def change_page(self, url):
        if url:
            html = self.open_url(url)
            self.soup = BeautifulSoup(html, "lxml")

    def get_next(self, url=None):
        if url:
            self.change_page(url)

        link = self.soup.find("a", {"rel":"next"})

        if link and "href" in str(link):
            href = link['href']
            match = re.search("https:\/\/www\.reddit\.com\/r\/.+(?:\?|&)after=(.+)&?", href)
            if match:
                return match.group(1)

    def get_prev(self, url=None):
        if url:
            self.change_page(url)

        link = self.soup.find("a", {"rel":"prev"})

        if link and "href" in str(link):
            href = link['href']
            match = re.search("https:\/\/www\.reddit\.com\/r\/.+(?:\?|&)before=(.+)&?", href)
            if match:
                return match.group(1)
                


    def get_images(self, url=None):
		
		if url:
			self.change_page(url)	
		    
		links = self.soup.find_all("div", {"class":"listing search-result-listing"})
		lsoup = BeautifulSoup(str(links), "lxml")
		results = [ l['href'] for l in lsoup.find_all("a", {"class":"search-link"})]

		images = []
		for img in results:
				images.append(img)

		return images

    
    def open_subreddit(sr, sort="top", time="month", after="", query=""):
         
        url = RedditUrl(sr, sort, time, after, query)

        change_page(url.url)


class RedditUrl(object):
    
    def __init__(self, sr, sort="top", time="month", after="", query="", flair="", self_post="no", count="50", before=""):
        self.sr = sr
        self.sort = sort
        self.time = time
        self._after = after
        self._before = before
        self.query = query
        self.self_post = self_post
        self.flair = flair
        self.count = count

    def construct_query(self):
        
        query = self.query + " self:" + self.self_post

        if self.flair:
            query = query + " flair:" + self.flair

        return quote(query)

    @property
    def url(self):
        query = self.construct_query()
        url = "https://www.reddit.com/r/{}/search?q={}&restrict_sr=on&sort={}&t={}&count={}"
        url = url.format(self.sr, query, self.sort, self.time, self.count)

        if self.after:
            url+="&after="+self.after 
        if self.before:
            url+="&before="+self.before
        
        return url

    @property
    def after(self):
        return self._after
    @after.setter
    def after(self, value):

        if value:
            self._before = None
            self._after = value 

    @property
    def before(self):
        return self._before
    @before.setter
    def before(self, value):

        if value:
            self._after = None
            self._before = value 

if __name__ == "__main__":
    
    r = RedditApi("https://www.reddit.com/r/wallpapers/")
    print r.get_images()
