#!/usr/bin/env sh
set -e

REPO="ak495867/dmc-redis"
INSTALL_DIR="${HOME}/.dmc-redis/bin"
VERSION="${1:-latest}"

if [ "$VERSION" = "latest" ]; then
    TAG="v1.0.0"
else
    TAG="$VERSION"
fi

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux*)
        TARGET="linux-x86_64"
        ;;
    Darwin*)
        if [ "$ARCH" = "arm64" ]; then
            TARGET="macos-arm64"
        else
            TARGET="macos-x86_64"
        fi
        ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

ASSET_URL="https://github.com/${REPO}/releases/download/${TAG}/dmc-redis-${TARGET}.tar.gz"

mkdir -p "$INSTALL_DIR"
TMP_DIR="$(mktemp -d)"

if command -v curl >/dev/null 2>&1; then
    curl -fsSL "$ASSET_URL" -o "${TMP_DIR}/dmc-redis.tar.gz"
elif command -v wget >/dev/null 2>&1; then
    wget -qO "${TMP_DIR}/dmc-redis.tar.gz" "$ASSET_URL"
else
    echo "curl or wget is required"
    exit 1
fi

tar -xzf "${TMP_DIR}/dmc-redis.tar.gz" -C "$TMP_DIR"
find "$TMP_DIR" -type f -name "dmc-redis" -exec cp -f {} "${INSTALL_DIR}/dmc-redis" \;
chmod +x "${INSTALL_DIR}/dmc-redis"

rm -rf "$TMP_DIR"

echo "dmc-redis ${TAG} successfully installed to ${INSTALL_DIR}"
