# CS360 Project Level-2 Check List
### Team Members:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Khalil Daoud  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ID: 11505041  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Asher Handaly  &nbsp;&nbsp;&nbsp; ID: 11619990
1. Download ~samples/LEVEL2/disk2. Use it as diskimage for testing  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- file1 : an empty file  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- tiny  : a few lines of text, only 1 data block  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- small : with 2 direct data blocks  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- large : with Indirect data blocks  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- huge  : with Double-Indirect data blocks  
#### A. IF YOU can do 2,3,4 below, you are done, skip Part B below
2. Test YOUR cat:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;cat tiny, cat small, cat large, cat huge: Correct OUTPUTS? ______________ 40
3. Test YOUR cp:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;cp small newsmall; ls: newsmall exist? SAME SIZE? _____________________ 10  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;cp large newlarge; ls: newlarge exist? SAME SIZE? _____________________ 20  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;cp huge newhuge;   ls: newhuge  exist? SAME SIZE? _____________________ 30  
4. Enter quit to exit YOUR program. Check YOUR cp results under Linux:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sudo mount mydisk /mnt  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sudo ls -l /mnt                   # should see all files  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sudo diff /mnt/huge /mnt/newhuge  # diff will show differences, if ANY  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sudo umount /mnt  
    
#### B. If you can NOT cat, cp correctly, do the following  
5. Show your open, pfd, close                                               20  
6. Show you can open small for READ; read a few times from the opened fd    20  
7. Show you can open file1 for WRITE; write a few times to the opened fd    20    
