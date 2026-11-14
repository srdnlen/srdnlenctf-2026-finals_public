import { h, render } from 'preact';
import { useEffect, useState } from 'preact/hooks';

const replies = [
  'Please check the tracking page for the latest update.',
  'That service is currently unavailable.',
  'Please contact the sender for more information.'
];

function Chat() {
  const [messages, setMessages] = useState([]);
  const [draft, setDraft] = useState('');

  function addMessage(content) {
    const reply = replies[Math.floor(Math.random() * replies.length)];
    setMessages(current => [...current,
      { sender: 'user', content },
      { sender: 'assistant', content: reply }
    ]);
  }

  useEffect(() => {
    function receive(event) {
      if (event.source !== parent) return;
      if (event.data?.type === 'chat-message') addMessage(event.data.content);
    }
    addEventListener('message', receive);
    parent.postMessage({ type: 'widget-ready' }, '*');
    return () => removeEventListener('message', receive);
  }, []);

  function submit(event) {
    event.preventDefault();
    if (!draft.trim()) return;
    addMessage(draft.trim());
    setDraft('');
  }

  return h('div', { class: 'widget' },
    h('div', { class: 'messages', role: 'log', 'aria-live': 'polite' },
      messages.length ? messages.map((message, index) =>
        h('p', { key: index, class: `message ${message.sender}` }, message.content)
      ) : h('p', null, 'Ask a question about your shipment.')
    ),
    h('form', { onSubmit: submit },
      h('label', { class: 'sr-only', for: 'chat-input' }, 'Your message'),
      h('input', { id: 'chat-input', value: draft, onInput: event => setDraft(event.currentTarget.value), placeholder: 'Type a message' }),
      h('button', { type: 'submit', disabled: !draft.trim(), 'aria-label': 'Send message' }, 'Send')
    )
  );
}

render(h(Chat), document.getElementById('root'));
