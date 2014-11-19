# .bash_profile
# Created By Landon Sutherland
# Last edited: 11/19/14

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi

# User specific environment and startup programs
PATH=$PATH:$HOME/bin
MYAPP=/data/web/landoman/app/hpcaccounts
STAPP=/data/web/staging/cake/hpcaccounts
PROAPP=/data/web/pro/
STAGE=$STAPP/../..

# alias's to make life easier
alias ls="ls --color"
alias sobash="source ~/.bash_profile"
alias vibash="vim ~/.bash_profile && sobash"
alias vivim="vim ~/.vimrc"

# -- lanl specific --
alias cdhpc="cd $MYAPP"
alias cdst="cd $STAPP"
alias cdstage="cd $STAGE"
alias cdpro="cd $PROAPP"
alias hpcldap='ldapsearch -x -LLL -h hpcldap.lanl.gov -b "dc=hpc,dc=lanl,dc=gov"'
alias eisldap='ldapsearch -x -LLL -h ldap-1.lanl.gov -b "dc=lanl,dc=gov"'
alias groupldap='ldapsearch -x -LLL -h dir5.lanl.gov -b "ou=group,dc=lanl,dc=gov"'
alias devldap='ldapsearch -x -LLL -h hpcvm8.lanl.gov -b "dc=hpc,dc=lanl,dc=gov"'
alias backuphpc="rm -rf ~/backuphpc && cp -r $MYAPP ~/backuphpc"
alias tag_staging="cd $STAGE && ./tag_n_switch && cdhpc"
alias updatest="$MYAPP/tests/migration/update_staging.sh && tag_staging"
alias mymysql="mysql -u landoman -h hpcdb.lanl.gov --password=landon224881"
alias mysqldumpdev2="mysqldump -u landoman -h hpcdb.lanl.gov --password=landon224881 hpcaccounts_dev2 > hpcaccounts_deve2.sql"
alias init_migrate="$MYAPP/tests/migration/migrate_script.sh && updatest"
alias gendocs="cd $STAPP && doxygen && cd $MYAPP"
# ------------------

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
cdhpc
set http_proxy=http://proxyout.lanl.gov:8080
export PATH  
