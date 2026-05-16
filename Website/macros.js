const replaceMacrosFromJSON = async () => {
  const contentArea = document.querySelector('.content-wrapper');
  if (!contentArea) return;

  try {
    const response = await fetch('macros.json');
    const macroVariables = await response.json();

    let html = contentArea.innerHTML;
    const macroRegex = /\[\[([a-zA-Z0-9_]+)\]\]/g;

    html = html.replace(macroRegex, (match, key) => {
      return macroVariables.hasOwnProperty(key) ? macroVariables[key] : match;
    });

    contentArea.innerHTML = html;
    
    if (typeof highlightCode === 'function') {
      highlightCode();
    }
  } catch (error) {
    console.error(error);
  }
};

document.addEventListener('DOMContentLoaded', replaceMacrosFromJSON);