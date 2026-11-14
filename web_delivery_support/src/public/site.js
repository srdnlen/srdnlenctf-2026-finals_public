const chat = document.getElementById('chat');
const frame = document.getElementById('chat-frame');

document.querySelector('[data-ask]').addEventListener('click', event => {
  chat.open = true;
  frame.contentWindow.postMessage({
    type: 'chat-message', content: event.currentTarget.dataset.ask
  }, window.origin);
});
