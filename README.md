hubo-logging
============

Logging and plotting routines for Hubo.

Please contact Matt Zucker <mzucker1@swarthmore.edu> with questions,
comments, or rants.

Based on MRDPLOT code obtained from Chris Atkeson @ CMU

Building 
========

To build the software, you will need these libraries:

  - Qt4
  - Qwt5

You must also install cmake to build as well. To build:

    cd /path/to/hubo-logging
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make

Programs
========

To log data from the robot (either directly or over an ACH remote
session), run the `hubo-logger` program. You should supply a
destination filename on the command line:

    hubo-logger LOGFILENAME.data

To stop logging, simply interrupt the program with Ctrl+C.

To plot log data, use the plot program:

    plot LOGFILENAME.data

To search for a variable name, use the search box in the top right.
To add a variable to a plot, click the variable name in the list view
on the right, and then click any row of plots on the left. You may add
multiple traces to a single row.

The left hand side of the window has a two-column array of plots. The
left column shows the entire data trace, and the right column shows a
detailed virsion. To zoom in or out in the detail view, use the
mousewheel or Ctrl+EQUALS or Ctrl+MINUS. To reset the zoom, use
Ctrl+0.

You may also load and save plot sets using the file menu. The plot set
files should have the .plots extension.
