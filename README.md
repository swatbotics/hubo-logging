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
