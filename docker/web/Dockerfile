FROM node:20-alpine
RUN corepack enable && corepack prepare pnpm@9.15.4 --activate
WORKDIR /app
COPY package.json pnpm-workspace.yaml pnpm-lock.yaml ./
COPY packages/types/package.json ./packages/types/
COPY apps/web/package.json ./apps/web/
RUN pnpm install --frozen-lockfile
COPY packages/types/ ./packages/types/
COPY apps/web/ ./apps/web/
EXPOSE 5173
WORKDIR /app/apps/web
CMD ["pnpm", "dev"]
