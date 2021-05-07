" settings to match coding style in vim
" add the following settings as last two lines in your .vimrc
"
"    set secure
"    set exrc
"

" Vim plugin to fit the Linux kernel coding style and help kernel development
" Maintainer:   Vivien Didelot <vivien.didelot@savoirfairelinux.com>
" License:      Distributed under the same terms as Vim itself.
"
" This script is inspired from an article written by Bart:
" http://www.jukie.net/bart/blog/vim-and-linux-coding-style
" and various user comments.

augroup linuxsty
    autocmd!

    autocmd FileType c,cpp call s:LinuxCodingStyle()
    autocmd FileType diff setlocal ts=8
    autocmd FileType rst setlocal ts=8 sw=8 sts=8 noet
augroup END

command! LinuxCodingStyle call s:LinuxCodingStyle()

function! s:LinuxCodingStyle()
    call s:LinuxFormatting()
    call s:LinuxKeywords()
    call s:LinuxHighlighting()
endfunction

function s:LinuxFormatting()
    setlocal colorcolumn=100
    setlocal tabstop=8
    setlocal shiftwidth=8
    setlocal softtabstop=8
    setlocal textwidth=100
    setlocal noexpandtab

    setlocal cindent
    setlocal cinoptions=:0,l1,t0,g0,(0
endfunction

function s:LinuxKeywords()
    syn keyword cStatement fallthrough
    syn keyword cOperator likely unlikely
    syn keyword cType u8 u16 u32 u64 s8 s16 s32 s64
    syn keyword cType u8 u16 u32 u64 s8 s16 s32 s64
    syn keyword cType leint16_t leint32_t leint64_t
    syn keyword cType le16 le32 le64
    syn keyword cType beint16_t beint32_t beint64_t
    syn keyword cType be16 be32 be64
endfunction

function s:LinuxHighlighting()
    highlight default link LinuxError ErrorMsg

    syn match LinuxError / \+\ze\t/     " spaces before tab

    " Highlight trailing whitespace, unless we're in insert mode and the
    " cursor's placed right after the whitespace. This prevents us from having
    " to put up with whitespace being highlighted in the middle of typing
    " something
    autocmd InsertEnter * match LinuxError /\s\+\%#\@<!$/
    autocmd InsertLeave * match LinuxError /\s\+$/
endfunction

" vim: ts=4 et sw=4
