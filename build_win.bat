echo Preparing 'vulkan-learning' solution...

mkdir build
cd build

cmake -G "Visual Studio 16 2019" .. 

start "" "vulkan-learning.sln"

cd ..