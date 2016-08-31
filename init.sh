echo "initializing your dev environment..."

cd ~
ln -s ~/mouse-hole/.bash_profile ~/.bash_profile
ln -s ~/mouse-hole/.vimrc ~/.vimrc
ln -s ~/mouse-hole/.vim ~/.vim

git config --global user.name "Landon Sutherland"
git config --global user.email "Sutherlandon@gmail.com"

source .bash_profile
echo "done"
