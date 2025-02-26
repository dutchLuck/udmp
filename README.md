# udmp
This utility program is named "udmp" because it provides a dump of 
bytes (8 bit data) sent to it's input or obtained from one or more files.
By default "udmp" dumps standard input or file data to standard out.
This utility may be of more use than other dump utilities if the input
is expected to wholly, or partially, consist of UTF-8 text strings.
If no options are specified then it outputs the input in much the
same fashion as the "cat" utility. For example on MacOS a file
containing hebrew (read right to left); -
```
% ./udmp test/gen1_1.txt
בראשית ברא אלהים את השמים ואת הארץ׃
% cat test/gen1_1.txt 
בראשית ברא אלהים את השמים ואת הארץ׃
```
If any of the options "-I" (index), "-H" (hex), "-A" (ascii) or
"-C" (code point) are activated then the output is switched to columns.
For example when all four options are active; -
```
 % ./udmp -IHAC test/gen1_1.txt
 0  d7 91 d7 a8 d7 90 d7 a9 d7 99 d7 aa  ............  בראשית    X+005d1 X+005e8 X+005d0 X+005e9 X+005d9 X+005ea
12  20 d7 91 d7 a8 d7 90 20 d7 90 d7 9c  ␠......␠....  ␠ברא␠אל    X+00020 X+005d1 X+005e8 X+005d0 X+00020 X+005d0 X+005dc
24  d7 94 d7 99 d7 9d 20 d7 90 d7 aa 20  ......␠....␠  הים␠את␠    X+005d4 X+005d9 X+005dd X+00020 X+005d0 X+005ea X+00020
36  d7 94 d7 a9 d7 9e d7 99 d7 9d 20 d7  ..........␠.  השמים␠ו    X+005d4 X+005e9 X+005de X+005d9 X+005dd X+00020 X+005d5
48  95 d7 90 d7 aa 20 d7 94 d7 90 d7 a8  .....␠......  את␠האר     X+005d0 X+005ea X+00020 X+005d4 X+005d0 X+005e8
60  d7 a5 d7 83 0d 0a                    ....␍␊        ץ׃␍␊       X+005e5 X+005c3 X+0000d X+0000a
```
