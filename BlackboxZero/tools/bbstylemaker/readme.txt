
  =================================================================

  bbStyleMaker 1.31
  Copyright 2003-2009 grischka@users.sourceforge.net

  bbStyleMaker is a tool for Blackbox for Windows to create
  and edit blackbox styles in an interactive way.

  BBSTYLEMAKER IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.
  THE AUTHOR DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR
  IMPLIED, INCLUDING THE WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE
  AUTHOR OR ITS SUPPLIERS BE LIABLE FOR ANY DAMAGES WHATSOEVER
  INCLUDING DIRECT, INDIRECT, INCIDENTAL, CONSEQUENTIAL, LOSS
  OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF THE AUTHOR
  OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGES.

  =================================================================

  Quick Start:
  ------------
  - Have blackbox running
  - Load a style that you want to begin with
  - Start bbStyleMaker

  Everything else should explain itself easily step by step while
  you are using it.


  Buttons:
  --------
  [edit]
    Opens the current style in the 'blackbox editor' (as set in
    extensions.rc or bbStyleMaker.rc)

  [save]
    Saves the style to disk.  Asks for a name only if the style
    was not edited previously with bbStyleMaker.  Also, wallpaper
    image changes on the 'bsetroot' section need a 'save' to apply.

  [save as]
    Saves the style to disk. Asks for a name always.  Also lets
    you choose whether you want a clean style or rather keep the
    formattings as is (i.e. leave any additional items and comments
    in the file).

  [reload]
    Reverts to the last saved state.

  [quit]
    Closes bbStyleMaker.

  [?]
    Show bbStyleMaker Info.


  Color picker:
  -------------
  - The topmost color field shows the current style texture.

    You can left click this and hold down the mousebutton to
    pick a color from anywhere of the screen or from the
    palette fields, also.


  Color palette:
  --------------
  - The other 9 color fields are palettes to store a style
    texture temporarily.

  - Right-click to store a texture to the palette.

  - Left click to pick from the palette. The behaviour varies
    accordingly to the 'color' selection:

    Selection:        Properties applied:
    ------------------------------------------------------------
    color 1/2         All (texture, colors, bevel, text, border)
    text              textColor and/or foregroundColor
    border            borderColor


  Options:
  --------
  - *border when set applies border changes to all outer borders
    of the selected part (e.g. with window: title, frame, handle)

  - *font when clicked applies the current shown font setting to
    all parts once.

  - With the sliders you can hold down the control key to link/unlink
    them temporarily.


  bsetroot gradient:
  ------------------
  In the 'other' section you can now interactively set the background
  rootCommand for the style. Note that you need to save the style to
  let image settings take effect (otherwise the update would be too slow)


  Configuration:
  --------------
  bbStyleMaker requires the configuration file 'bbStyleMaker.rc'
  in the same directory. You can open it in a text editor and set any
  options to your preference. You can also change the style of the gui,
  the font and font-size and the overall width/height of the window.


  Changing style syntax:
  ----------------------
  When a style is 'saved as' with the style syntax set changing
  (bb*nix 0.65 <-> bb*nix 0.70) bbStyleMaker will insert a section
  'lost & found', with anything that it does not know what it is.
  It may be other items e.g. for bbpager or just misspelled items.
  You may want to edit this manually before proceeding.


  Requirements:
  -------------
  bbStyleMaker 1.31 requires bblean 1.17


  Changes:
  --------

  [20 May 2009] v 1.31
  - Released with bbLean 1.17

  [15 Mar 2006] v 1.3 - not published
  - New layout, new options
  - Border and margin settings for the new BB 0.70 style conventions
  - HSL (hue, sat, lum) slider mode
  - Interactive setting of the bsetroot background
  - Style info page

  [05 Jul 2004] v 1.2
  - Fixed "Save As..." dialog for win98

  [02 Feb 2004] v 1.1
  - fixed capitalization of style items - for fluxbox compatibility
  - alternate file writing mode, preserves manually added items

  [09 Jan 2004] v 1.0
  - original release
