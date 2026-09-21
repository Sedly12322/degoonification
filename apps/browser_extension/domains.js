// Degoonification Comprehensive Adult Domain & Pattern Database

const BLOCKED_DOMAINS = new Set([
  // Top tier adult video networks & tubes
  "pornhub.com", "xvideos.com", "xnxx.com", "xhamster.com", "redtube.com",
  "youporn.com", "spankbang.com", "eporner.com", "hqporner.com", "tube8.com",
  "beeg.com", "daftsex.com", "porntrex.com", "thumbzilla.com", "xgroovy.com",
  "txxx.com", "tubehd.com", "vporn.com", "4tube.com", "fuq.com",
  "heavy-r.com", "motherless.com", "erome.com", "fapello.com", "faphero.com",
  "leakgirls.com", "thothub.to", "simpcity.su", "bunkr.is", "bunkr.ru",
  "coomer.party", "coomer.su", "kemono.party", "kemono.su",

  // Cam sites & live streaming
  "chaturbate.com", "stripchat.com", "bongacams.com", "cam4.com",
  "livejasmin.com", "camsoda.com", "myfreecams.com", "flirt4free.com",
  "streamate.com", "imlive.com", "jerkmate.com",

  // Creator platforms & paywalls
  "onlyfans.com", "fansly.com", "manyvids.com", "loyalfans.com", "admireme.vip",

  // Hentai, anime & rule34
  "rule34.xxx", "rule34video.com", "e621.net", "gelbooru.com", "danbooru.donmai.us",
  "hentaihaven.xxx", "hanime.tv", "nhentai.net", "tsumino.com", "hitomi.la",
  "pururin.io", "fakku.net", "e-hentai.org", "exhentai.org", "hentaivn.net",
  "asmhentai.com", "multporn.net", "3dbooru.com",

  // Premium studio brands
  "brazzers.com", "naughtyamerica.com", "realitykings.com", "bangbros.com",
  "mofos.com", "digitalplayground.com", "twistys.com", "fakehub.com",
  "evilangel.com", "blacked.com", "tushy.com", "vixen.com", "deeper.com",

  // Czech & European specific adult sites
  "amaterky.cz", "ceske-kurvicky.cz", "freevideo.cz", "sex.cz", "libimseti.cz",
  "sexcesky.cz", "porno-zdarma.cz", "erotik.cz", "sexbazar.cz"
]);

const BLOCKED_TLDS = [
  ".xxx", ".porn", ".adult", ".sex", ".cam"
];

const BLOCKED_PATTERNS = [
  "porn", "xxx", "hentai", "camgirl", "sexvid", "erovideo", "livecam",
  "nakedgirl", "fapvid", "nudeleak", "onlyfansleak", "erocams"
];

// Returns true if the given hostname should be blocked
function isDomainBlocked(hostname) {
  if (!hostname) return false;
  const host = hostname.toLowerCase().replace(/^www\./, '');

  // Exact domain or subdomain match
  for (const domain of BLOCKED_DOMAINS) {
    if (host === domain || host.endsWith('.' + domain)) {
      return true;
    }
  }

  // Adult TLD match
  for (const tld of BLOCKED_TLDS) {
    if (host.endsWith(tld)) {
      return true;
    }
  }

  // Hostname pattern match (e.g. free-porn-videos.net)
  for (const pat of BLOCKED_PATTERNS) {
    if (host.includes(pat)) {
      return true;
    }
  }

  return false;
}

// Check Reddit NSFW subreddits
function isRedditNsfwPath(pathname) {
  if (!pathname) return false;
  const path = pathname.toLowerCase();
  const nsfwSubs = [
    '/r/nsfw', '/r/gonewild', '/r/porn', '/r/hentai', '/r/realgirls',
    '/r/rule34', '/r/ecchi', '/r/boobs', '/r/ass', '/r/milf',
    '/r/camgirls', '/r/fetish', '/r/onlyfans', '/r/leaks'
  ];
  return nsfwSubs.some(sub => path.startsWith(sub));
}

// Export for background and content scripts
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { BLOCKED_DOMAINS, BLOCKED_TLDS, BLOCKED_PATTERNS, isDomainBlocked, isRedditNsfwPath };
}
