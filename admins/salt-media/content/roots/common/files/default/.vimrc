setglobal fencs=ucs-bom,utf-8,cp1251
set encoding=utf8

function HasBOM()
    return &bomb ? ',bom' : ''
endfunction

" Editing behaviour
set autoindent
set hlsearch
set incsearch
map <C-H> :nohl<CR>
set smartcase
set showcmd
set shiftwidth=4
set expandtab
set smarttab
set softtabstop=4
set backspace=indent,eol,start
set updatetime=60000
set updatecount=1024
set timeoutlen=500
set smartindent
set completeopt=menuone,longest
set noswapfile
set modeline
autocmd FileType make :set noexpandtab
cmap w!! w !sudo tee % > /dev/null

" Display behaviour
set ruler
set laststatus=2
set statusline=%<%f%h%m%r%=%{&filetype}\ \ (%{&fileformat},%{&fileencoding}%{HasBOM()})\ \ %b(0x%B)\ \ %l,%c%V\ \ %P
set showtabline=1

nohl
set nolist
set listchars=eol:⁋,tab:↣⇢,trail:·,extends:⇉,precedes:⇇,nbsp:¤

" Helpers behaviour
set suffixes=.bak,~,.swp,.o,.info,.aux,.log,.dvi,.bbl,.blg,.brf,.cb,.ind,.idx,.ilg,.inx,.out,.toc,.pyc
set wildmode=list:longest
set wildignore=*.o,__init__.py
set scrolloff=5
set sidescroll=10

" Misc settings
filetype plugin indent on
syntax on
if &term == "xterm"
    set t_Co=256
    let xterm16bg_Normal = 'none'
endif

nmap <Up> gk
nmap <Down> gj
nmap <CR> O<Esc>
nmap <space> za
nmap <F5> :set invwrap wrap?<CR>
nmap <F6> :set invlist list?<CR>
