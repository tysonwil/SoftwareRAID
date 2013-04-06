///////////////////////////////////////////////////////////////////////////////
// PROGRAM 3 - Software Raid
// Title: README.txt
// Files: raidsim.c, disk.c, disk.h, disk-array.c, disk-array.h, simple-trace.txt, second-trace.txt,
//Makefile
// Semester: CS537 Spring 2013
//
// PAIR PROGRAMMERS
//
// Pair Partner: (Ted) Tianchu Huang thuang33@wisc.edu
// CS Login: Tianchu
// Lecturer's Name: Michael Swift
//
// Pair Partner: Tyson Williams tjwilliams4@wisc.edu
// CS Login: twilliam
// Lecturer's Name: Michael Swift
//
// Pair Partner: Adam Thorson adthorson@wisc.edu
// CS Login: thorson
// Lecturer's Name: Michael Swift
//
//////////////////////////// 80 columns wide //////////////////////////////////

Flow of program: Go through main; jump to choose system (picks one of the 4 doRaid methods); once you
are within a given doRaid function, the rest of the executable code will run the trace file.

For RAID 0 and 10, all tests are passing.  In regards to RAID 4 and 5, all recovered disks displayed 
accurate readings when possible with the tests we exercised. Also, make note that Additive Parity was
implemented in both 4 and 5.  There are no memory leaks nor corruption problems.
