id=e4503b30fc78200f846c62cf8091b76ff5547662
mkdir tmp
tar -xvzf $id.tar.gz -C tmp
rm -r ~/.vscode-server/bin/$id
mv tmp/vscode-server-linux-x64 ~/.vscode-server/bin/$id
rm -r tmp