FROM ubuntu:22.04

# 1. Installation des dépendances système
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    git \
    cmake \
    g++ \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    openssl \
    libssl-dev \
    postgresql-server-dev-all \
    curl \
    && rm -rf /var/lib/apt/lists/*

# 2. Installation de Drogon depuis la source (version stable)
WORKDIR /tmp
RUN git clone https://github.com/drogonframework/drogon && \
    cd drogon && \
    git checkout v1.9.0 && \
    git submodule update --init && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install

# 3. Préparation de l'application
WORKDIR /app

# Copie des fichiers sources
COPY CMakeLists.txt .
COPY src/ ./src/
COPY config/ ./config/

# 4. Compilation de votre projet
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# 5. Commande de lancement
WORKDIR /app/build
EXPOSE 8082
CMD ["./antennes_5g"]