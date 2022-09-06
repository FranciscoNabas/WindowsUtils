. dotnet build Module\src\WindowsUtilsModule.csproj --arch x64 --configuration Release --output Module\src\bin\Release
Copy-Item -Path .\LICENSE -Destination .\Module\src\bin\Release -Force
Compress-Archive -Path .\Module\src\bin\Release\* -DestinationPath .\WindowsUtilsModule.zip