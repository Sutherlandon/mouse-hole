echo "initializing your dev environment..."

cd ~
ln -s ~/mouse-hole/.bash_profile ~/.bash_profile
ln -s ~/mouse-hole/.vimrc ~/.vimrc
ln -s ~/mouse-hole/.vim ~/.vim

git --global config user.name "Landon Sutherland"
git --gloabl config user.email "Sutherlandon@gmail.com"

source .bash_profile
echo "done"
