document.addEventListener('DOMContentLoaded', () => {
  const targetUrlEl = document.getElementById('targetUrl');
  const quoteTextEl = document.getElementById('quoteText');
  const quoteAuthorEl = document.getElementById('quoteAuthor');
  const btnBack = document.getElementById('btnBack');
  const btnBreathe = document.getElementById('btnBreathe');
  const breathingModal = document.getElementById('breathingModal');
  const btnCloseBreathe = document.getElementById('btnCloseBreathe');
  const breatheText = document.getElementById('breatheText');

  // 1. Parse target URL from query parameter
  const params = new URLSearchParams(window.location.search);
  const rawUrl = params.get('url');
  if (rawUrl) {
    try {
      const u = new URL(rawUrl);
      targetUrlEl.textContent = u.hostname + (u.pathname !== '/' ? u.pathname : '');
    } catch (e) {
      targetUrlEl.textContent = rawUrl;
    }
  } else {
    targetUrlEl.textContent = 'Neznámá adresa';
  }

  // 2. Curated Stoic & Dopamine Reset Quotes
  const QUOTES = [
    { text: "Máš moc nad svou myslí, ne nad vnějšími událostmi. Uvědom si to a najdeš svou pravou sílu.", author: "— Marcus Aurelius" },
    { text: "Vítězství nad sebou samým je to největší ze všech vítězství.", author: "— Platón" },
    { text: "Svoboda není dělat to, co chceš, ale mít vládu nad tím, co děláš.", author: "— Epiktétos" },
    { text: "Každé nutkání trvá v průměru jen 10 minut. Přečkej tento impuls a tvůj mozek se uklidní.", author: "— Neuroplasticita & Dopamin" },
    { text: "Nepřijímej krátkodobé potěšení, které krade tvůj dlouhodobý potenciál a sebeúctu.", author: "— Seneca" },
    { text: "Disciplína je most mezi tvými cíli a tvou skutečnou realitou.", author: "— Degoon Defense" }
  ];

  const randomQuote = QUOTES[Math.floor(Math.random() * QUOTES.length)];
  quoteTextEl.textContent = randomQuote.text;
  quoteAuthorEl.textContent = randomQuote.author;

  // 3. Return to Safety action
  btnBack.addEventListener('click', () => {
    if (window.history.length > 1) {
      window.history.back();
    } else {
      window.location.href = 'about:newtab';
    }
  });

  // 4. Guided Breathing Exercise (4s in, 4s out)
  let breatheInterval = null;
  btnBreathe.addEventListener('click', () => {
    breathingModal.classList.add('active');
    let isInhale = true;
    breatheText.textContent = 'Nádech... (4s)';

    breatheInterval = setInterval(() => {
      isInhale = !isInhale;
      breatheText.textContent = isInhale ? 'Nádech... (4s)' : 'Výdech... (4s)';
    }, 4000);
  });

  btnCloseBreathe.addEventListener('click', () => {
    breathingModal.classList.remove('active');
    if (breatheInterval) {
      clearInterval(breatheInterval);
      breatheInterval = null;
    }
  });
});
