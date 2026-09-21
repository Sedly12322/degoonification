document.addEventListener('DOMContentLoaded', () => {
  const masterToggle = document.getElementById('masterToggle');
  const blockSitesToggle = document.getElementById('blockSitesToggle');
  const videoToggle = document.getElementById('videoToggle');
  const imageToggle = document.getElementById('imageToggle');
  const strictToggle = document.getElementById('strictToggle');
  const peekToggle = document.getElementById('peekToggle');
  const blurSlider = document.getElementById('blurSlider');
  const blurValue = document.getElementById('blurValue');
  const statusBadge = document.getElementById('statusBadge');
  const blockedSitesCount = document.getElementById('blockedSitesCount');
  const blockedCount = document.getElementById('blockedCount');

  const storage = (typeof browser !== 'undefined') ? browser.storage.local : chrome.storage.local;

  function updateStatus(enabled) {
    if (enabled) {
      statusBadge.textContent = 'ACTIVE';
      statusBadge.className = 'status-badge';
    } else {
      statusBadge.textContent = 'PAUSED';
      statusBadge.className = 'status-badge disabled';
    }
  }

  // Load saved settings
  if (storage) {
    storage.get({
      enabled: true,
      blockAdultSites: true,
      blurVideos: true,
      blurImages: true,
      strictMode: false,
      allowPeek: false,
      blurRadius: 28,
      stats: { totalBlockedMedia: 0, totalBlockedSites: 0 }
    }, (items) => {
      masterToggle.checked = items.enabled;
      blockSitesToggle.checked = items.blockAdultSites !== false;
      videoToggle.checked = items.blurVideos;
      imageToggle.checked = items.blurImages;
      strictToggle.checked = items.strictMode;
      peekToggle.checked = items.allowPeek;
      blurSlider.value = items.blurRadius;
      blurValue.textContent = `${items.blurRadius}px`;

      const stats = items.stats || {};
      blockedSitesCount.textContent = stats.totalBlockedSites || 0;
      blockedCount.textContent = stats.totalBlockedMedia || 0;

      updateStatus(items.enabled);
    });
  }

  // Event handlers
  masterToggle.addEventListener('change', () => {
    const val = masterToggle.checked;
    updateStatus(val);
    storage.set({ enabled: val });
  });

  blockSitesToggle.addEventListener('change', () => {
    storage.set({ blockAdultSites: blockSitesToggle.checked });
  });

  videoToggle.addEventListener('change', () => {
    storage.set({ blurVideos: videoToggle.checked });
  });

  imageToggle.addEventListener('change', () => {
    storage.set({ blurImages: imageToggle.checked });
  });

  strictToggle.addEventListener('change', () => {
    storage.set({ strictMode: strictToggle.checked });
  });

  peekToggle.addEventListener('change', () => {
    storage.set({ allowPeek: peekToggle.checked });
  });

  blurSlider.addEventListener('input', () => {
    const val = blurSlider.value;
    blurValue.textContent = `${val}px`;
    storage.set({ blurRadius: parseInt(val, 10) });
  });
});
