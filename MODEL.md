# MADLAD's Text Editing Model

At it's core, MADLAD does not use a rope, piece table or gap buffer
to represent it's text, but a bespoke system inspired by classic `vi`.

This structure, referred to as a `buffer` in the code, is a
doubly-linked list of text lines. A reference is kept to the first
line, last line, line containing the cursor and line at the top of
the screen, as well as the total number of lines in the buffer. The
column of the text cursor is also stored, which may or may not be
beyond the length of the line itself.

When you make changes to the line, it is first copied into a small
gap buffer and the changes are made within that buffer. This gap
buffer then remains for as long as possible, and any reads and writes
to the line go through the buffer. When the gap buffer absolutely
must be purged (if you switch lines, for example) then it's contents
are flushed into the original line again, and the gap buffer is
freed. When the gap buffer exists, the number stored in the cursor
column is irrelevant, and the index of the start of the gap is used
instead. When flushed, this value is copied back into the buffer's
cursor column index.

You can use the gap buffer system as a rudimentary single-step undo
function. When you want to undo your changes, you can just discard
the gap buffer without flushing it's contents. In a `vi`-like editor,
you can force-flush the buffer when entering insert mode. This makes
it fairly intuitive what exactly undoing will remove.

That's all there is to it. If you want to check out the
implementation, see [buffer.c](src/buffer.c).
