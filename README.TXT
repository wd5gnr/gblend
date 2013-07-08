Copyright 2013 by Al Williams (al.williams@awce.com
Distributed under the terms of the GNU General Public License

    This file is part of GBlend.

    GBlend is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.



What is Gblend?

Gblend allows you to take multiple gcode filies and merge then into a single file, taking layers from each file and joining them.

There are several 3D printing use cases for this:

1) You can slice an object using multiple settings and then take slices from each setting and merge them. For example, you might slice a statue so that the bottom layers are solid and low resolution while the top is at a finer resolution and hollow.

2) You can remove one or more slices of a print.

3) You can merge multiple files together to print them as one, although current slicing software will do a better job of this for you.

4) You can mix different objects. For example, you might blend a base and an object.

5) You can break a file at a layer and insert code such as a filament change.

Note that Gblend does not manipulate coordinates, so any translation of objects (for example, in #3 and #4 above) has to be done in the original source file.

Suppose you have a statue that is 50mm high. The first 26mm is a base and the rest is the body of the statue. You might want to print the base at 0.5mm with 40% honeycomb infill. However, the body might benefit from .1mm layers with 0% infill. These instructions assume you will start with an STL file that describes the model and use a slicing program such as slic3r.

In your slicer configuration, it is helpful, although perhaps not absolutely necessary, to add a special comment to the "preamble" and "post" G-code. That is, the slicer's configuration usually has a way for you to set some G code that is sent before printing and after printing. You often use this to home the head, set calibration parameters, etc. You should place copies of these codes in separate files (for example, start.gcode and end.gcode). You can leave the code in your slicer setup, but add a comment at the end of the start code:

; %%%GBLEND_START

Also add one at the start of the end code:

; %%%GBLEND_END

Now slice the model multiple times using the settings you want. Be sure to save each slicing operation to a separate file. I might create two files for the statue: statue_solid.gcode and statue_hollow.gcode. Gblend can merge these files to create one gcode file that has the desired print.

gblend = start.gcode [ statue_solid.gcode 26.4 statue_hollow.gcode ] = end.gcode >statue-print.gcode

Interpreting this command line in English it says: copy (=) start.gcode to the output. Then take statue_solid.gcode and copy from the start of the actual print ([) up to (but not including) 26.4mm. Then starting at 26.4mm, copy to the end of the print (]) to statue_hollow.gcode and finally copy (=) end.gcode totally.

You can use the following flags for the start:
[ - Start of printable object (after %%%GBLEND_START)
[5.2 - Start at 5.2mm
5.2 - Start at 5.2mm (same as [5.2)

Normally, the start of the next file determines the end of the previous file. If you want to specify a specific stop, you can use a close bracket. For example:

[20.0 foo.gcode 21.0]

will cause Gblend to copy the layers from 20.0mm up to and including 21.00mm. Without the bracket it would not copy the 21.00mm layer since that would be the start of the next file. If you use an explicit end, you will probably want to specify the next file's start as well:

[20.0 foo.gcode 21.0] 21.4 bar.gcode ]

If there is no next file or if you specify ] by itself (like bar.gcode in the above example) gblend copies to the end of the file.

Any time gblend is looking for the start or end of a file, it only looks in between the %%%GBLEND_START and %%%GBLEND_END comments. You can stop that behavior using -S and -E, respectively. You can also specify different tags using -s or -e. The only exception is the = tag which reads the entire file from beginning to end without searching for the tags. Gblend ignores a semicolon at the start of a tag so:

; %%%GBLEND_START

and

;%%%GBLEND_START

will both work. If you specify your own tag, it can not contain any spaces, tabs, or newlines. That is, "%%%GBLEND START" is not a legal tag because it has a space in it.

