A simple shell with tutorial as a base

brennan.io/2015/01/16/write-a-shell-in-c/ 

Alpha 1:
The shell can execute program, change directory, and exit

Alpha 2: 
- Updated pipeline, now can work up to 20 command in pipe separated by |
- Added backslash escaping: now any characters after \ is taken as literal
- Added quotation handling (double quote " will takes all but allow escape, single quote ' will do the same but not allow escape)
- Added common globbing:
+ wild card * will match all characters until encounter (space)
+ Single ? will match any single character
+ Square bracket [] will match all within set of 1 digit range, ex [1-9] will match a character as one of the number in range

Adjustment 2.1
- Fix the bug where tokenizer ignore the open quotations without closing

Alpha 3:
- Added sh_ls: 
+ Command sh_ls list all files and directories on current directory by default when no relative directory name is provided (will not work when directory is inside another directory at current place); Also including hidden like ".." parent and "." self directories.
+ -l flag will display long form information of the files and directories. 
+ Usage syntax: sh_ls (directory_name) (-l), order of flag and the target directory does not matter, directory_name and -l are optional.

- Globbing does nothing issue: no command to take advantage of globbing. Fixed in this version with sh_find

- Added sh_find:
+ Command sh_find print out relative path to the file name provided
+ Support globbing pattern matching. The three common globbing from previous version is applied here (*, ?, and [#]), with same functioning rule as before.
+ Support multiple pattern searching, can search different patterns at the same command
+ Usage syntax: sh_find pattern_1 (pattern_2) ... , pattern_1 is required, pattern_2 and more are optional.