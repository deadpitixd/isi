// this should be included in every file that has code displayed

const highlightCode = () => {
  const blocks = [...document.querySelectorAll('.syntax-highlight'), ...document.querySelectorAll('.container')];

  blocks.forEach(block => {
    let text = block.textContent;

    const rules = [
      { type: 'comment',  regex: /(\/\/.*|\/\*[\s\S]*?\*\/)/g },
      { type: 'string',   regex: /("(?:\\"|[^"])*"|'(?:\\'|[^'])*')/g },
      { type: 'type',     regex: /\b(int|float|char|string|bool|while|if|else|lib)\b(?![^<>]*>)/g },
      { type: 'keyword',  regex: /\b(const|while|if|else|lib|return)\b(?![^<>]*>)/g },
      { type: 'func',     regex: /\b([a-z_][a-z0-9_]*)(?=\()(?![^<>]*>)/gi }
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