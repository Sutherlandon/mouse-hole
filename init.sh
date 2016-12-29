echo "initializing your dev environment..."

cd ~
rm -f ~/.bashrc ~/.vimrc
ln -s ~/mouse-hole/.bashrc ~/.bashrc
ln -s ~/mouse-hole/.vimrc ~/.vimrc
ln -s ~/mouse-hole/.vim ~/.vim

echo "Setting git globals"
git config --global user.name "Landon Sutherland"
git config --global user.email "Sutherlandon@gmail.com"
# may not need this anymore
#git config --global push.default simple

echo "Source: ~/.bashrc"
source ~/.bashrc

echo "done"
