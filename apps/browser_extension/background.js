// Degoonification Background Service
let stats = {
  totalBlockedMedia: 0,
  totalBlockedSites: 0,
  sessionsProtected: 1
};

let config = {
  enabled: true,
  blockAdultSites: true,
  enforceSafeSearch: true
};

// Load saved config & stats
browser.storage.local.get({ config, stats }, (res) => {
  if (res.config) config = { ...config, ...res.config };
  if (res.stats) stats = { ...stats, ...res.stats };
});

// Update config on changes
browser.storage.onChanged.addListener((changes) => {
  for (const [key, change] of Object.entries(changes)) {
    if (key === 'blockAdultSites' || key === 'enforceSafeSearch' || key === 'enabled') {
      config[key] = change.newValue;
    }
  }
});

// Network Interceptor: Block Adult Websites & Enforce SafeSearch
browser.webRequest.onBeforeRequest.addListener(
  (details) => {
    if (!config.enabled) return {};

    try {
      const url = new URL(details.url);

      // 1. Adult Website & NSFW Subreddit Interceptor
      if (config.blockAdultSites) {
        const isBlockedDomain = (typeof isDomainBlocked === 'function') && isDomainBlocked(url.hostname);
        const isRedditNsfw = (typeof isRedditNsfwPath === 'function') && url.hostname.includes('reddit.com') && isRedditNsfwPath(url.pathname);

        if (isBlockedDomain || isRedditNsfw) {
          stats.totalBlockedSites++;
          browser.storage.local.set({ stats });

          const redirectUrl = browser.runtime.getURL("blocked.html") + "?url=" + encodeURIComponent(details.url);
          return { redirectUrl };
        }
      }

      // 2. SafeSearch Enforcement
      if (config.enforceSafeSearch && details.type === 'main_frame') {
        // Google SafeSearch
        if (url.hostname.includes('google.') && url.pathname.includes('/search')) {
          if (url.searchParams.get('safe') !== 'active') {
            url.searchParams.set('safe', 'active');
            return { redirectUrl: url.toString() };
          }
        }

        // DuckDuckGo SafeSearch
        if (url.hostname.includes('duckduckgo.com')) {
          if (url.searchParams.get('kp') !== '1') {
            url.searchParams.set('kp', '1');
            return { redirectUrl: url.toString() };
          }
        }

        // Bing SafeSearch
        if (url.hostname.includes('bing.com') && url.pathname.includes('/search')) {
          if (url.searchParams.get('adlt') !== 'strict') {
            url.searchParams.set('adlt', 'strict');
            return { redirectUrl: url.toString() };
          }
        }
      }
    } catch (e) {
      console.error('[DegoonBackground] Error processing request:', e);
    }

    return {};
  },
  {
    urls: ["<all_urls>"],
    types: ["main_frame", "sub_frame"]
  },
  ["blocking"]
);

// Message Handler
browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === 'BLOCKED_MEDIA') {
    stats.totalBlockedMedia++;
    browser.storage.local.set({ stats });
  } else if (message.type === 'GET_STATS') {
    sendResponse(stats);
  }
});

console.log('🛡️ Degoonification Adult Website Filter & Visual Defense active.');
