echo "initializing your dev environment..."

cd ~
ln -s ~/mouse-hole/.bashrc ~/.bashrc
ln -s ~/mouse-hole/.vimrc ~/.vimrc
ln -s ~/mouse-hole/.vim ~/.vim

git config --global user.name "Landon Sutherland"
git config --global user.email "Sutherlandon@gmail.com"
git config --global push.default simple

source ~/.bashrc
echo "done"
