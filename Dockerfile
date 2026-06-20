FROM mcr.microsoft.com/dotnet/sdk:9.0 AS api-build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src/c
COPY Makefile ./
COPY src/ ./src/
COPY tests/ ./tests/
RUN make gothic-lock

WORKDIR /src/api
COPY GothicCalculatorApi/GothicCalculatorApi.csproj ./
RUN dotnet restore GothicCalculatorApi.csproj
COPY GothicCalculatorApi/ ./
RUN dotnet publish GothicCalculatorApi.csproj -c Release -o /app/publish /p:UseAppHost=false

FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS api
WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/*
COPY --from=api-build /app/publish ./
COPY --from=api-build /src/c/gothic-lock ./gothic-lock
RUN chmod +x ./gothic-lock
ENV GOTHIC_LOCK_BIN=/app/gothic-lock
EXPOSE 8080
ENTRYPOINT ["dotnet", "GothicCalculatorApi.dll"]

FROM node:22-bookworm-slim AS frontend-build
WORKDIR /app
COPY gothic-calculator-frontend/package*.json ./
RUN npm ci
COPY gothic-calculator-frontend/ ./
RUN npm run build

FROM nginx:1.27-alpine AS frontend
COPY gothic-calculator-frontend/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=frontend-build /app/dist/gothic-calculator-frontend/browser /usr/share/nginx/html
EXPOSE 80
