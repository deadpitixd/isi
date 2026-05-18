const replaceMacrosFromJSON = async () => {
  const contentArea = document.querySelector('.content-wrapper');
  if (!contentArea) return;

  try {
    const response = await fetch('macros.json');
    const macroVariables = await response.json();

    let html = contentArea.innerHTML;
    const macroRegex = /\[\[([a-zA-Z0-9_-]+)(?::([a-zA-Z0-9_-]+))?\]\]/g;

    for (let pass = 0; pass < 2; pass++) {
      html = html.replace(macroRegex, (match, key, subkey) => {
        if (subkey) {
          if (macroVariables.hasOwnProperty(key) && macroVariables[key].hasOwnProperty(subkey)) {
            const commitHash = macroVariables[key][subkey];
            return `
              <details class="inline-commit">
                <summary class="g">${subkey}</summary>
                <span class="g">(COMMIT: ${commitHash})</span>
              </details>
            `.trim();
          }
          return match;
        }

        return macroVariables.hasOwnProperty(key) ? macroVariables[key] : match;
      });
    }

    contentArea.innerHTML = html;
    
    if (typeof highlightCode === 'function') {
      highlightCode();
    }
  } catch (error) {
    console.error(error);
  }
};

document.addEventListener('DOMContentLoaded', replaceMacrosFromJSON);