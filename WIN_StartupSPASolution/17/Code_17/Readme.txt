Visual Studio version: Visual Studio 2022 Community Edition
Path to solution: 17/Code_17/StartupSPASolution.sln
Path to AutoTester: 17/Code_17/Release/AutoTester.exe
Path to source txt: 17/Tests_17/Sample_source.txt
Path to queries txt: 17Tests_17/Sample_queries.txt

How to compile the SPA?
1. Open the StartupSPASolution.sln with Visual Studio 2022
2. Right Click AutoTester solution and click "Set as Startup Project"
3. Ensure that build configuration is set to "Release" else the AutoTester.exe will not be created in the correct location
4. Ensure that configuration is under Win32 and not x64, else the build may fail
5. Under the top bar, click Build > Build full program database file for solution

The AutoTester.exe will be create in 17/Code_17/Release/AutoTester.exe

How to run the AutoTester?
1. Open a command prompt and navigate to 17/Code_17/Release
2. Execute the following command in the command prompt
3. AutoTester.exe  ..\..\Tests_17\Sample_source.txt ..\..\Tests_17\Sample_queries.txt ..\..\Tests_17\out.xml

or

1. Click on "Local Windows Debugger" in Visual Studio 2022