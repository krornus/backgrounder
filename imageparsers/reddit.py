from reddit_api import RedditApi

class RedditParser(object):
    
    def get_images(self, uri):
        
        r = RedditApi()
        r.change_page(uri)
        print r.get_images()
        return []


if __name__ == "__main__":

    r = RedditParser()
    r.get_images("https://www.reddit.com/r/cats/search?q=maine%20coon%20self%3Ano&restrict_sr=on&sort=top&t=year&count=50&after=t3_4khl37")
