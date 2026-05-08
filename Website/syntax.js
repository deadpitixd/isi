const highlightCode = () => {
  const blocks = document.querySelectorAll('.syntax-highlight') + document.querySelectorAll('.container');

  blocks.forEach(block => {
    let text = block.textContent;

    const rules = [
      { type: 'comment',  regex: /(\/\/.*|\/\*[\s\S]*?\*\/)/g },
      { type: 'string',   regex: /("(?:\\"|[^"])*"|'(?:\\'|[^'])*')/g },
      { type: 'keyword',  regex: /\b(int|float|char|bool|while|if|else|lib)\b/g },
      { type: 'func',     regex: /\b([a-z_][a-z0-9_]*)(?=\()/gi }
    ];

    let highlighted = text;

    rules.forEach(rule => {
      highlighted = highlighted.replace(rule.regex, (match) => {
        const extraClass = rule.type === 'func' ? 'func' : '';
        return `<span class="code-${rule.type} ${extraClass}">${match}</span>`;
      });
    });

    block.innerHTML = highlighted;
  });

  setupFunctionLinks();
};

const setupFunctionLinks = () => {
  const functionLinks = document.querySelectorAll('.func');
  functionLinks.forEach(el => {
    el.style.cursor = 'pointer';
    el.addEventListener('click', () => {
      let name = el.innerText.toLowerCase().trim();
      window.location.href = `funcs/${name}.html`;
    });
  });
};

document.addEventListener('DOMContentLoaded', highlightCode);