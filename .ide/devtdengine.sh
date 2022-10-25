echo "------------创建数据目录、日志目录"
if [ ! -d "./sim/" ];then
mkdir sim sim/cfg sim/data sim/log
fi

if [ ! -d "./.vscode/" ];then
mkdir .vscode
fi

sudo chmod -R 777 sim

echo "------------复制调试配置文件"
cp /home/project/.ide/taos.cfg /home/project/sim/cfg/
cp /home/project/.ide/launch.json /home/project/.vscode/

echo "------------下载插件"
cd .ide
wget https://github.com/SmartIDE/cppvsix/releases/download/v1.0.0/ms-vscode.cmake-tools-1.13.15.vsix
wget https://github.com/SmartIDE/cppvsix/releases/download/v1.0.0/ms-vscode.cpptools-1.13.2@linux-x64.vsix

echo "------------安装插件"
code --install-extension /home/project/.ide/ms-vscode.cmake-tools-1.13.15.vsix
code --install-extension /home/project/.ide/ms-vscode.cpptools-1.13.2@linux-x64.vsix

cd ../