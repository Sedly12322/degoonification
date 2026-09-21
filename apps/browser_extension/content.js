(function () {
  'use strict';

  // Adult domain & keyword patterns
  const ADULT_DOMAINS = [
    'pornhub', 'xvideos', 'xnxx', 'xhamster', 'spankbang', 'redtube',
    'stripchat', 'chaturbate', 'onlyfans', 'erome', 'fapello', 'hqporner',
    'rule34', 'e621', 'nhentai', 'hanime', 'eporner', 'beeg', 'cam4',
    'bongacams', 'livejasmin', 'heavy-r', 'motherless', 'coomer', 'kemono',
    'hentaihaven', 'hitomi', 'tsumino', 'tube8', 'youporn', 'gelbooru', 'danbooru'
  ];

  const ADULT_KEYWORDS = [
    'porn', 'xxx', 'nsfw', 'adult', 'erotic', 'naked', 'nude', 'sex',
    'bikini', 'swimwear', 'swimsuit', 'lingerie', 'underwear', 'thong',
    'boobs', 'butt', 'ass', 'hentai', 'uncensored', 'camgirl'
  ];

  // Default configuration
  let config = {
    enabled: true,
    blurRadius: 28,
    blurVideos: true,
    blurImages: true,
    strictMode: false,
    allowPeek: false
  };

  let blockedCount = 0;
  const processedElements = new WeakSet();

  // 1. Check if current domain or URL is an adult site
  function isAdultSite() {
    try {
      const host = window.location.hostname.toLowerCase();
      const path = window.location.pathname.toLowerCase();

      // Check known adult domains
      if (ADULT_DOMAINS.some(d => host.includes(d))) return true;

      // Check generic adult keywords in hostname
      if (['porn', 'xxx', 'sex', 'hentai', 'erotic'].some(k => host.includes(k))) return true;

      // Check NSFW subreddits (e.g. reddit.com/r/nsfw...)
      if (host.includes('reddit.com') && (path.includes('/r/nsfw') || path.includes('/r/gonewild') || path.includes('/r/porn') || path.includes('/r/hentai'))) {
        return true;
      }
    } catch (e) {}
    return false;
  }

  // Tag adult sites immediately at document_start
  const onAdultSite = isAdultSite();
  if (onAdultSite && document.documentElement) {
    document.documentElement.classList.add('degoon-nsfw-site');
  }

  // Load configuration from storage
  const storage = (typeof browser !== 'undefined') ? browser.storage.local : chrome.storage.local;
  if (storage) {
    storage.get(config, (items) => {
      if (items) {
        config = { ...config, ...items };
        document.documentElement.style.setProperty('--degoon-blur-radius', `${config.blurRadius}px`);
        if (config.allowPeek) {
          document.documentElement.classList.add('degoon-allow-peek');
        } else {
          document.documentElement.classList.remove('degoon-allow-peek');
        }
        scanDOM();
      }
    });

    if (typeof browser !== 'undefined' && browser.storage.onChanged) {
      browser.storage.onChanged.addListener((changes) => {
        for (const [key, change] of Object.entries(changes)) {
          config[key] = change.newValue;
        }
        document.documentElement.style.setProperty('--degoon-blur-radius', `${config.blurRadius}px`);
        if (config.allowPeek) {
          document.documentElement.classList.add('degoon-allow-peek');
        } else {
          document.documentElement.classList.remove('degoon-allow-peek');
        }
        scanDOM();
      });
    }
  }

  function containsTrigger(text) {
    if (!text || typeof text !== 'string') return false;
    const lower = text.toLowerCase();
    return ADULT_KEYWORDS.some(kw => lower.includes(kw));
  }

  function isSuspicious(el) {
    if (onAdultSite || config.strictMode) return true;

    // Check Reddit specific attributes
    if (el.closest && (el.closest('[data-nsfw="true"]') || el.closest('.nsfw') || el.closest('shredder-media-item'))) {
      return true;
    }

    // Check Twitter / X sensitive media wrappers
    if (el.closest && el.closest('[data-testid="tweetPhoto"], [data-testid="videoComponent"]')) {
      const tweet = el.closest('article');
      if (tweet && tweet.innerText && containsTrigger(tweet.innerText)) {
        return true;
      }
    }

    const src = el.src || el.currentSrc || el.getAttribute('data-src') || '';
    const alt = el.alt || '';
    const title = el.title || '';
    const className = (typeof el.className === 'string') ? el.className : '';

    return containsTrigger(src) || containsTrigger(alt) || containsTrigger(title) || containsTrigger(className);
  }

  function shieldElement(el, isVideo = false) {
    if (processedElements.has(el)) return;
    processedElements.add(el);

    el.classList.add('degoon-blurred-media');

    if (isVideo && el.parentElement && !el.parentElement.classList.contains('degoon-video-wrapper')) {
      const wrapper = document.createElement('div');
      wrapper.className = 'degoon-video-wrapper';
      el.parentNode.insertBefore(wrapper, el);
      wrapper.appendChild(el);

      const badge = document.createElement('div');
      badge.className = 'degoon-shield-badge';
      badge.innerHTML = `<span>🛡️</span> <span>SHIELD ACTIVE</span>`;
      wrapper.appendChild(badge);
    }

    blockedCount++;
    if (typeof browser !== 'undefined' && browser.runtime && browser.runtime.sendMessage) {
      browser.runtime.sendMessage({ type: 'BLOCKED_MEDIA', count: blockedCount }).catch(() => {});
    }
  }

  function processNode(node) {
    if (!config.enabled || !node || node.nodeType !== 1) return;

    if (onAdultSite) {
      if (node.tagName === 'VIDEO' || node.tagName === 'IMG' || node.tagName === 'CANVAS') {
        shieldElement(node, node.tagName === 'VIDEO');
      }
      return;
    }

    if (config.blurVideos && node.tagName === 'VIDEO') {
      shieldElement(node, true);
      return;
    }

    if (config.blurImages && node.tagName === 'IMG') {
      if (isSuspicious(node)) {
        shieldElement(node, false);
      }
      return;
    }

    const bg = window.getComputedStyle ? window.getComputedStyle(node).backgroundImage : '';
    if (bg && bg !== 'none' && isSuspicious(node)) {
      shieldElement(node, false);
    }
  }

  function scanDOM() {
    if (!config.enabled) return;

    if (onAdultSite) {
      document.querySelectorAll('img, video, canvas').forEach(el => {
        shieldElement(el, el.tagName === 'VIDEO');
      });
      return;
    }

    if (config.blurVideos) {
      document.querySelectorAll('video').forEach(v => shieldElement(v, true));
    }

    if (config.blurImages) {
      document.querySelectorAll('img').forEach(img => {
        if (isSuspicious(img)) shieldElement(img, false);
      });
    }

    // Reddit & Twitter special selectors
    document.querySelectorAll('[data-nsfw="true"] img, .nsfw img, shredder-media-item img').forEach(img => {
      shieldElement(img, false);
    });
  }

  const observer = new MutationObserver((mutations) => {
    if (!config.enabled) return;
    for (const mutation of mutations) {
      for (const node of mutation.addedNodes) {
        if (node.nodeType === 1) {
          processNode(node);
          if (onAdultSite) {
            node.querySelectorAll?.('img, video, canvas').forEach(el => shieldElement(el, el.tagName === 'VIDEO'));
          } else {
            if (config.blurVideos) {
              node.querySelectorAll?.('video').forEach(v => shieldElement(v, true));
            }
            if (config.blurImages) {
              node.querySelectorAll?.('img').forEach(img => {
                if (isSuspicious(img)) shieldElement(img, false);
              });
            }
          }
        }
      }
    }
  });

  if (document.documentElement) {
    observer.observe(document.documentElement, {
      childList: true,
      subtree: true
    });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', scanDOM);
  } else {
    scanDOM();
  }
})();
