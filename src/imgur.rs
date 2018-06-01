use regex::Regex;
use url::Url;
use reqwest;
use kuchiki::traits::*;
use kuchiki;

use parser::{SiteParser,Error};
use parser::Error::*;
use parser::RouteError::*;


#[derive(Debug,Clone)]
pub struct ImgurParser {
    routes: Vec<Regex>,
}

#[derive(Debug, Deserialize)]
struct GalleryResponse {
    data: Gallery
}

#[derive(Debug, Deserialize)]
pub struct Gallery {
    count: usize,
    pub images: Vec<Image>,
}

#[derive(Debug, Deserialize)]
pub struct Image {
    pub hash:  String,
    pub title: String,
    pub description: Option<String>,
    pub has_sound: bool,
    pub width: usize,
    pub height: usize,
    pub size: usize,
    pub ext: String,
    pub animated: bool,
    pub prefer_video: bool,
    pub looping: bool,
    pub datetime: String,
}

impl Image {
    pub fn url(&self) -> String {
        format!("http://i.imgur.com/{}{}", self.hash, self.ext)
    }
}

impl SiteParser for ImgurParser {
    fn parse(&self, uri: &str) -> Vec<String> {

        let (i,re) = self.routes.iter().enumerate().find(|&(_,x)| {
                x.is_match(uri)
            }).unwrap();

        if i == 6 {
            vec![uri.to_string()]
        } else {
            let id = re.captures(uri).map(|m| m.get(1).unwrap())
                .unwrap();

            self.image_list(uri, id.as_str()).unwrap_or(vec![])
        }
    }

    fn is_valid(&self, uri: &str) -> bool {
        let routes = self.routes.iter().find(|x| {
                x.is_match(uri)
            }).map_or(false, |_| true);
        let url = Url::parse(uri).is_ok();

        routes && url
    }
}

impl ImgurParser {

    pub fn new() -> Self {
        let routes: Vec<_> = vec![
            Regex::new(r"^https?://imgur\.com/gallery/([^/]+)$").unwrap(),
            Regex::new(r"^https?://imgur\.com/r/[^/]+/([^/]+)$").unwrap(),
            Regex::new(r"^https?://imgur\.com/([^/]+)$").unwrap(),
            Regex::new(r"^https?://imgur\.com/t/[^/]+/([^/]+)$").unwrap(),
            Regex::new(r"^https?://imgur\.com/a/([^/]+)$").unwrap(),
            Regex::new(r"^https?://imgur\.com/([^/]+)$").unwrap(),
            Regex::new(r"^https?://i\.imgur\.com/([^/]+)$").unwrap(),
        ];

        ImgurParser {
            routes
        }
    }

    fn image_list(&self, uri: &str, id: &str) -> Result<Vec<String>, Error> {
        if let Ok(gallery) = self.gallery(uri, id) {
            Ok(gallery.images.iter().map(|x| x.url()).collect())
        } else if let Ok(images) = self.images(uri) {
            Ok(images)
        } else {
            Err(Route(InvalidGallery))
        }
    }

    fn images(&self, uri: &str) -> Result<Vec<String>, Error> {
        let html = reqwest::get(uri)?.text()?;
        let post_container = "div.post-image-container";

        let document = kuchiki::parse_html().one(html);

        let images = document.select(post_container).unwrap()
            .filter_map(|div| {
                div.as_node().select_first("img").ok()
            }).flat_map(|img| {
                self.source_from_element(&*img)
            }).collect();

        Ok(images)
    }

    fn source_from_element(&self, img: &kuchiki::ElementData) -> Vec<String> {
        if let Some(src) = img.attributes.borrow().get("src") {
            vec![format!("http:{}", src)]
        } else if let Some(id) = img.attributes.borrow().get("id") {
            self.parse(&format!("http://i.imgur.com/{}", id))
        } else {
            vec![]
        }
    }

    fn gallery(&self, uri: &str, id: &str) -> Result<Gallery, Error> {

        if let Err(_) = Url::parse(uri) {
            return Err(Route(InvalidUrl));
        }

        let ajax_request = &format!(
            "http://imgur.com/ajaxalbums/getimages/{}/hit.json",
            id,
        );

        let gallery: GalleryResponse = reqwest::get(ajax_request)?.json()?;
        Ok(gallery.data)
    }
}

