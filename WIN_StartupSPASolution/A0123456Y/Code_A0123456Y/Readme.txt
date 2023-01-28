Visual Studio version: Visual Studio 2022 Community Edition
Path to solution: Code_A0123456Y/StartupSPASolution.sln
Path to AutoTester: Code_A0123456Y/Release/AutoTester.exe
Path to source txt: Code_A0123456Y/Tests/Sample_source.txt
Path to queries txt: Code_A0123456Y/Tests/Sample_queries.txt

How to compile the SPA?
1. Open the StartupSPASolution.sln with Visual Studio 2022
2. Right Click AutoTester solution and click "Set as Startup Project"
3. Ensure that build configuration is set to "Release" else the AutoTester.exe will not be created in the correct location
4. Under the top bar, click Build > Build full program database file for solution

The AutoTester.exe will be create in Code_A0123456Y/Release/AutoTester.exe

How to run the AutoTester?
1. Open a command prompt and navigate to Code_A0123456Y/Release
2. Execute the following command in the command prompt
3. AutoTester.exe  ..\Tests\Sample_source.txt ..\Tests\Sample_queries.txt ..\Tests\out.xml