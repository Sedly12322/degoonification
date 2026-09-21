// Degoonification Background Service
let stats = {
  totalBlocked: 0,
  sessionsProtected: 1
};

// Enforce SafeSearch across all major search engines
browser.webRequest.onBeforeRequest.addListener(
  (details) => {
    try {
      const url = new URL(details.url);

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
    } catch (e) {
      console.error('[DegoonBackground] Error redirecting request:', e);
    }
    return {};
  },
  {
    urls: [
      "*://*.google.com/search*",
      "*://*.google.cz/search*",
      "*://duckduckgo.com/*",
      "*://*.bing.com/search*"
    ],
    types: ["main_frame"]
  },
  ["blocking"]
);

// Listen for messages from content scripts
browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.type === 'BLOCKED_MEDIA') {
    stats.totalBlocked++;
    browser.storage.local.set({ stats });
  } else if (message.type === 'GET_STATS') {
    sendResponse(stats);
  }
});

console.log('🛡️ Degoonification Zen Browser Extension background active.');
