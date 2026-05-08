document.addEventListener('DOMContentLoaded', () => {
    const functionLinks = document.querySelectorAll('.func');
    functionLinks.forEach(el => {
        el.style.cursor = 'pointer';
        el.addEventListener('click', () => {
            let name = el.innerText.toLowerCase().replace(/[()]/g, '').trim();
            
            window.location.href = `funcs/${name}.html`;
        });
    });
});