rem ./node_modules/.bin/electron-rebuild -f -w serialport --platform=win32 --arch=x64
npm run electron-build
electron-packager . --platform=win32 --arch=x64 --overwrite
start alert.mp3
camsys-win32-x64/camsys.exe
