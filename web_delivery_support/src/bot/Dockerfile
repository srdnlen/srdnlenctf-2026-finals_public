FROM mcr.microsoft.com/playwright:v1.54.1-noble
WORKDIR /home/bot
COPY package*.json ./
RUN npm install --omit=dev
COPY . ./
RUN useradd --create-home bot && chown -R bot:bot /home/bot
USER bot
CMD ["node", "index.js"]
