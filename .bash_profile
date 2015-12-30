# .bash_profile
# Created By Landon Sutherland
# Last edited: 06/02/2015

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi

# User specific environment and startup programs
PATH=$PATH:$HOME/bin

# alias's to make life easier
alias ls="ls --color"
alias sobash="source ~/.bash_profile"
alias vibash="vim ~/.bash_profile && sobash"
alias vivim="vim ~/.vimrc"
alias sshprom="ssh prometj6@66.147.244.148"

# Regular Colors
# https://wiki.archlinux.org/index.php/Color_Bash_Prompt
off='\[\e[0m\]'       # Text Reset
black='\[\e[0;30m\]'        # Black
red='\[\e[0;31m\]'          # Red
green='\[\e[0;32m\]'        # Green
yellow='\[\e[0;33m\]'       # Yellow
blue='\[\e[0;34m\]'         # Blue
purple='\[\e[0;35m\]'       # Purple
cyan='\[\e[0;36m\]'         # Cyan
white='\[\e[0;37m\]'        # White

# prompt
alias get_site="pwd | awk -F '/' '{ print \$4 }'"
function set_prompt()
{
    # preserve the exit code of the last command
    code=$?
    site=$(get_site)
    working_dir=${PWD##*/}

    # get the site root if we are working on one and in it
    if [ -z "$site" ] || [ $site == $working_dir ];
    then
        # don't display the site root
        site=""
    else
        # color the site root
        site="[$purple$site$white]"
    fi

    #color the working directory
    if [ -z "$working_dir" ];
    then
        # color the root directory
        working_dir="[$yellow/$white]"
    else
        # color the working directory
        working_dir="[$yellow$working_dir$white]"
    fi

    # build a new prompt line
    PS1="$white[$green\h$white][$cyan\!$white]$site$working_dir$off \`if [ $code = 0 ];
            then echo \[\e[36m\]»\\ $off;
            else echo \[\e[31m\]»\\ $off;
        fi;\`"
}
PROMPT_COMMAND='set_prompt'

# Commands to run upon login
export PATH
