#!/bin/bash
set -e

# Определение ОС
OS="$(uname -s)"
echo "=> Detected OS: $OS"

# Функции установки зависимостей
install_deps_macos() {
    if ! command -v brew &>/dev/null; then
        echo "Homebrew not found. Installing..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    echo "=> Installing dependencies via Homebrew..."
    brew install cmake make 2>/dev/null || true

    if ! command -v clang++ &>/dev/null && ! command -v g++ &>/dev/null; then
        echo "=> No C++ compiler found. Installing Xcode Command Line Tools..."
        xcode-select --install 2>/dev/null || true
        echo "   Please re-run build.sh after the installation completes."
        exit 1
    fi
}

install_deps_linux() {
    if command -v apt-get &>/dev/null; then
        echo "=> Detected package manager: apt"
        sudo apt-get update -qq
        sudo apt-get install -y cmake g++ make

    elif command -v pacman &>/dev/null; then
        echo "=> Detected package manager: pacman"
        sudo pacman -Sy --noconfirm cmake gcc make

    elif command -v dnf &>/dev/null; then
        echo "=> Detected package manager: dnf"
        sudo dnf install -y cmake gcc-c++ make

    elif command -v yum &>/dev/null; then
        echo "=> Detected package manager: yum"
        sudo yum install -y cmake gcc-c++ make

    elif command -v zypper &>/dev/null; then
        echo "=> Detected package manager: zypper"
        sudo zypper install -y cmake gcc-c++ make

    else
        echo "ERROR: No supported package manager found (apt, pacman, dnf, yum, zypper)."
        echo "Please install cmake, g++, and make manually."
        exit 1
    fi
}

# Установка зависимостей
case "$OS" in
    Darwin)
        install_deps_macos
        ;;
    Linux)
        install_deps_linux
        ;;
    *)
        echo "ERROR: Unsupported OS: $OS"
        exit 1
        ;;
esac

# Проверка успешности установки
echo "=> Verifying tools..."

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found after installation."
    exit 1
fi

CXX_COMPILER=""
if command -v clang++ &>/dev/null; then
    CXX_COMPILER="clang++"
elif command -v g++ &>/dev/null; then
    CXX_COMPILER="g++"
else
    echo "ERROR: No C++ compiler (clang++ or g++) found."
    exit 1
fi

echo "   cmake:   $(cmake --version | head -1)"
echo "   c++:     $($CXX_COMPILER --version | head -1)"

# Сборка
echo "=> Building project..."
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER="$CXX_COMPILER"
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
cd ..

# Проверка результата
echo ""
echo "=> Build artifacts in build/:"
MISSING=0
for artifact in build/db_server build/db_client; do
    if [ -f "$artifact" ]; then
        echo "   [OK] $artifact"
    else
        echo "   [MISSING] $artifact"
        MISSING=1
    fi
done

for lib in build/libdb_core.a build/libdb_client_lib.a; do
    if [ -f "$lib" ]; then
        echo "   [OK] $lib"
    else
        echo "   [MISSING] $lib"
        MISSING=1
    fi
done

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "ERROR: Some artifacts are missing. Build may have failed."
    exit 1
fi

echo ""
echo "=> Build successful!"
