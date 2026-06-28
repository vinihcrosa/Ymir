FROM node:20-alpine AS base
RUN corepack enable && corepack prepare pnpm@9.15.4 --activate

FROM base AS deps
WORKDIR /app
COPY package.json pnpm-workspace.yaml pnpm-lock.yaml ./
COPY packages/types/package.json ./packages/types/
COPY apps/api/package.json ./apps/api/
RUN pnpm install --frozen-lockfile

FROM deps AS builder
COPY packages/types/ ./packages/types/
COPY apps/api/ ./apps/api/
RUN pnpm --filter @ymir/types build
RUN pnpm --filter @ymir/api build

FROM node:20-alpine AS runner
RUN corepack enable && corepack prepare pnpm@9.15.4 --activate
WORKDIR /app
COPY package.json pnpm-workspace.yaml pnpm-lock.yaml ./
COPY packages/types/package.json ./packages/types/
COPY apps/api/package.json ./apps/api/
RUN pnpm install --frozen-lockfile --prod
COPY --from=builder /app/packages/types/dist/ ./packages/types/dist/
COPY --from=builder /app/apps/api/dist/ ./apps/api/dist/
EXPOSE 3000
CMD ["node", "apps/api/dist/index.js"]
