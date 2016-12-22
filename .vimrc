" ./vimrc created by Landon Sutherland
" last edit 9/11/2014

" This must be first, because it changes other options as a side effect
set nocompatible


"___ setting the look and feel of the editor ___________________________
" sets syntax highlighting
set syntax=on
set filetype=on
set hlsearch

" sets tabs and indenting
set ts=4
set shiftwidth=4
set autoindent
set list listchars=tab:¬\ ,trail:» " sets how to represenet whitespace chars like tab a trailing characters

" cursor and lines
set number " sets line numbers
set cursorline " shows the line the cursor is on
hi cursorline term=none cterm=none ctermbg=235 ctermfg=none " highlights the cursor line
set scroll=20 " make <C-d> and <C-y> move 20 lines instead of 40
set scrolloff=10 " makes it so there the cursor is alway 10 lines from the top or bottom when scrolling
set laststatus=2 " displays file names a the bottom
set showmode " show what mode we are in
set showcmd "show comman (partial) in the bottome right corner, on the last line
set pastetoggle=<F2> " when in insert mode, press <F2> to enter paste mode and whant you paste won't be autoindented

hi ColorColumn ctermbg=235
set colorcolumn=80


"___ abreviations for autocompleteing ____________________________________
" inserts a comment block after typing the first line
ab /** /**<CR> *<CR>*/<Up>


"___ remaping commands ___________________________________________________
" Change the mapleader from \ to .
let mapleader="."

" clears highlighting of search terms with ctrl+l
nnoremap <silent> <C-l> :nohl<CR><C-l>

" get out of insert mode and start scrolling, because that is probably what you meant to do
inoremap jj <ESC><Down>
inoremap kk <ESC><Up>

" writes a file even in insert mode
inoremap :w <ESC>:w<CR>

" Edit and source the vimrc file
nnoremap <silent> <leader>ev :e ~/.vimrc<CR>
nnoremap <silent> <leader>sv :so ~/.vimrc<CR>
nnoremap <silent> <leader>f5 :call Nofluff()<CR>
nnoremap <silent> <leader>f6 :call Yesfluff()<CR>
nnoremap <silent> <leader>f7 :call Tab2()<CR>

" turns on and off extra character for copy and pasting
map <F5> :call Nofluff()<CR>
map <F6> :call Yesfluff()<CR>
map <F7> :call Tab2()<CR>


"___ user defined functions _______________________________________________
" used for cleaning editor for clean copy and pasting
function! Nofluff()
	set nolist " turnes off all extra chars
	set nu! " turns off line numbers
endfunction
"
" turns line numbers and whitespace caracters back on
function! Yesfluff()
	set list " turns on all extra charis
	set nu " turns on line numbers
endfunction

" sets all tabing to 2 spaces instead of 4
function! Tab2()
	set ts=2
	set shiftwidth=2
	set expandtab
endfunction
