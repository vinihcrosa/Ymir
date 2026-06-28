# Stage 1: build WASM with Emscripten
FROM emscripten/emsdk:latest AS wasm-builder

COPY core/ /src/core/

# cmake requires the entry to be named CMakeLists.txt — stage it alongside sources
WORKDIR /wasm-src
RUN cp /src/core/CMakeLists.wasm.txt ./CMakeLists.txt && \
    cp -r /src/core/libs  ./libs  && \
    cp -r /src/core/cmake ./cmake && \
    cp -r /src/core/src   ./src

RUN emcmake cmake -B /wasm-build -S /wasm-src -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /wasm-build --parallel

# Stage 2: Vite dev server
FROM node:20-alpine
RUN corepack enable && corepack prepare pnpm@9.15.4 --activate
WORKDIR /app
COPY package.json pnpm-workspace.yaml pnpm-lock.yaml ./
COPY packages/types/package.json ./packages/types/
COPY apps/web/package.json ./apps/web/
RUN pnpm install --frozen-lockfile
COPY packages/types/ ./packages/types/
COPY apps/web/ ./apps/web/
COPY --from=wasm-builder /wasm-build/ymir.js   ./apps/web/public/wasm/
COPY --from=wasm-builder /wasm-build/ymir.wasm ./apps/web/public/wasm/
EXPOSE 5173
WORKDIR /app/apps/web
CMD ["pnpm", "dev"]
