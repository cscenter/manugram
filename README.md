manugram
========

Computer Science Center, fall 2014, practice

Building
========

Developed for Qt5, may work under Qt4. Just run qmake (standard `CONFIG+=debug` and `CONFIG+=release` work)
followed by make and you'll get an executable.

Testing
=======

Tests are located in `tests.cpp` and are built in the same project. Just add `CONFIG+=tests`
(you may want it with `CONFIG+=debug` too) and run qmake followed by make.

You'll get a standard Qt testlib executable, which runs all the tests and prints results to stdout.

You can also add extra build configuration in Qt Creator (say, 'tests') based on standard 'debug' configuration.
The only different you shoudl add is `CONFIG+=tests` to qmake parameters.

_For MinGW users_: Please take a look at [this](http://stackoverflow.com/questions/16611108/qt-creator-unit-test-project)
  question on StackOverflow. In short: if your build directory contains spaces and you get gcc's error
  like `<part-of-your-directory-here>: No such file or directory`, locate a file named `testlib_defines.prf`
  in your Qt installation directory and replace triple backslashes by one backslash.

Similar products
================
* <a href="http://sketchometry.org">Sketchometry</a>, <a href="http://habrahabr.ru/post/239259/">article</a> in Russian
* SMART Board software: the 'smart pen' tool (not sure about English name of the tool)

Used resources
==============
Icon from http://www.icons4android.com/icon/1120 was used, edited on http://romannurik.github.io/AndroidAssetStudio/icons-launcher.html
Icon in ICO format (for Windows) converted by http://www.icoconverter.com

Invariants
==========
1. Output operations do not throw exceptions. The only kind of error is underlying stream error - you have to manually check output stream bad bits afterwards, if you wish
2. `exportModelToImageFile` throws exception if something goes wrong with QFile
3. Output to `std::stringstream` is not checked for success - I assume that the only reason for it to fail is `bad_alloc`

Examples
========
Some pictures that I want to be drawable with Manugram are available <a href="https://yadi.sk/d/mynlHT1Vga4A5">here</a>.
