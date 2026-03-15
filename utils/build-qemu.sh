pushd ..

export LIBRARY_PATH=$CX2_ROOT/build-qemu/lib
export LD_LIBRARY_PATH=$CX2_ROOT/build-qemu/lib

pushd qemu_cx

# Check if the directory exists
if [ -d "build" ]; then
    echo "Stale build directory found at: $(pwd)/build"
    # Prompt the user
    read -p "Delete existing build directory and start fresh? (y/n): " confirm
    if [[ "$confirm" == [yY] || "$confirm" == [yY][eE][sS] ]]; then
        echo "Wiping build directory..."
        rm -rf build
    else
        echo "Proceeding with existing build directory"
    fi
fi

# Create directory if it was deleted or didn't exist
mkdir -p build
pushd build

../configure --target-list=riscv32-softmmu

if [ $? -ne 0 ]; then
	echo "issue with configuring qemu - possibly due to not having a python venv active"
    exit 1
fi

ninja

if [ $? -ne 0 ]; then
	echo "issue building qemu"
    exit 1
fi

popd
