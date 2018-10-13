#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include <gtest/gtest.h>

int main(int argc, char **argv) {
   testing::InitGoogleTest(&argc, argv);
   
   int returnCode = RUN_ALL_TESTS();
   
   if (IsDebuggerPresent()) {
      puts("Press any key...");
      _getch();
   }

   return returnCode;
}
