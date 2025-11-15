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