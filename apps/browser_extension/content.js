(function () {
  'use strict';

  // Default configuration
  let config = {
    enabled: true,
    blurRadius: 28,
    blurVideos: true,
    blurImages: true,
    strictMode: false,
    allowPeek: false,
    keywords: [
      'nsfw', 'adult', 'xxx', 'porn', 'erotic', 'naked', 'nude', 'sex',
      'bikini', 'swimwear', 'swimsuit', 'lingerie', 'underwear', 'thong',
      'onlyfans', 'leaks', 'boobs', 'butt', 'ass', 'hentai', 'uncensored'
    ]
  };

  let blockedCount = 0;
  const processedElements = new WeakSet();

  // Load configuration from browser.storage
  const storage = (typeof browser !== 'undefined') ? browser.storage.local : chrome.storage.local;
  if (storage) {
    storage.get(config, (items) => {
      if (items) {
        config = { ...config, ...items };
        document.documentElement.style.setProperty('--degoon-blur-radius', `${config.blurRadius}px`);
        scanDOM();
      }
    });

    // Listen for setting changes from popup
    if (typeof browser !== 'undefined' && browser.storage.onChanged) {
      browser.storage.onChanged.addListener((changes) => {
        for (const [key, change] of Object.entries(changes)) {
          config[key] = change.newValue;
        }
        document.documentElement.style.setProperty('--degoon-blur-radius', `${config.blurRadius}px`);
        scanDOM();
      });
    }
  }

  // Check if string contains any NSFW triggers
  function containsTrigger(text) {
    if (!text || typeof text !== 'string') return false;
    const lower = text.toLowerCase();
    return config.keywords.some(kw => lower.includes(kw));
  }

  // Inspect element attributes and parent context
  function isSuspicious(el) {
    if (config.strictMode) return true;

    // Check direct attributes
    const src = el.src || el.currentSrc || el.getAttribute('data-src') || '';
    const alt = el.alt || '';
    const title = el.title || '';
    const className = (typeof el.className === 'string') ? el.className : '';

    if (containsTrigger(src) || containsTrigger(alt) || containsTrigger(title) || containsTrigger(className)) {
      return true;
    }

    // Check parent context (links, post containers, article tags)
    let parent = el.parentElement;
    let depth = 0;
    while (parent && depth < 4) {
      const pClass = (typeof parent.className === 'string') ? parent.className : '';
      const pId = parent.id || '';
      const pData = parent.getAttribute('data-testid') || parent.getAttribute('data-nsfw') || '';

      if (containsTrigger(pClass) || containsTrigger(pId) || containsTrigger(pData)) {
        return true;
      }
      parent = parent.parentElement;
      depth++;
    }

    return false;
  }

  // Apply GPU blur shield to an element
  function shieldElement(el, isVideo = false) {
    if (processedElements.has(el)) return;
    processedElements.add(el);

    el.classList.add('degoon-blurred-media');
    if (config.allowPeek) {
      el.classList.add('degoon-allow-peek');
    }

    // Wrap videos in relative container with shield badge
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

  // Process a single media node
  function processNode(node) {
    if (!config.enabled || !node || node.nodeType !== 1) return;

    // 1. Process <video> tags
    if (config.blurVideos && node.tagName === 'VIDEO') {
      shieldElement(node, true);
      return;
    }

    // 2. Process <img> tags
    if (config.blurImages && node.tagName === 'IMG') {
      if (isSuspicious(node)) {
        shieldElement(node, false);
      }
      return;
    }

    // 3. Process container elements with CSS background images
    const bg = window.getComputedStyle ? window.getComputedStyle(node).backgroundImage : '';
    if (bg && bg !== 'none' && isSuspicious(node)) {
      shieldElement(node, false);
    }
  }

  // Scan current document DOM for media
  function scanDOM() {
    if (!config.enabled) return;

    if (config.blurVideos) {
      document.querySelectorAll('video').forEach(v => shieldElement(v, true));
    }

    if (config.blurImages) {
      document.querySelectorAll('img').forEach(img => {
        if (isSuspicious(img)) shieldElement(img, false);
      });
    }
  }

  // Observe dynamically loaded elements (infinite scroll / AJAX)
  const observer = new MutationObserver((mutations) => {
    if (!config.enabled) return;
    for (const mutation of mutations) {
      for (const node of mutation.addedNodes) {
        if (node.nodeType === 1) {
          processNode(node);
          // Check children
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
  });

  // Start observing
  if (document.documentElement) {
    observer.observe(document.documentElement, {
      childList: true,
      subtree: true
    });
  }

  // Initial scan on load
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', scanDOM);
  } else {
    scanDOM();
  }
})();
