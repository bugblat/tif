tif - a tiny FPGA board
=======================

This is the software for Bugblat's *tiny FPGA* board - the *tif* board.

What is a tif board?
--------------------

A tif board is a small USB dongle which carries a non-volatile Lattice Semiconductor MachXO2 FPGA.

To program the FPGA, you start by writing a firmware program, usually in VHDL or Verilog though there are other options. The firmware program can be  simulated and compiled to a JEDEC bitstream with Lattice's free
*[Diamond](http://www.latticesemi.com/en/Products/DesignSoftwareAndIP/FPGAandLDS/LatticeDiamond.aspx)*
software.

Then you have to inject the bitstream into the tif's FPGA, and that's the job of the software in this repo. Primarily in Python, there are programs to

- program the onboard FPGA with a compiled bitstream
- control the FPGA firmware from a PC

Example FPGA firmware programs are also included,
plus an example of controlling the FPGA from a web application.

More Information
----------------

The tif product pages, including links to the full documentation, are
[here](http://bugblat.com/products/tif).
