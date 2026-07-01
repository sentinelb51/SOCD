@echo off

echo Building cleaner...
taskkill /f /im "SOCD_AD_C.exe">NUL 2>&1
gcc -std=c23 -O3 -s -flto -Wall -Wextra -o ./cleaner/SOCD_AD_C.exe ./cleaner/SOCD_AD_C.c
echo Building tester...
taskkill /f /im "SOCD_tester.exe">NUL 2>&1
gcc -std=c23 -O3 -s -flto -Wall -Wextra -o ./tester/SOCD_tester.exe ./tester/SOCD_tester.c
echo Done.