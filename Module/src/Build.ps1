[CmdletBinding()]
param (
    [Parameter()]
    [string]$PsgSec
)

## Building project
. dotnet build Module\src\WindowsUtilsModule.csproj --arch x64 --configuration Release --output Module\src\bin\Release

## Copying license and compressing output
Copy-Item -Path .\LICENSE -Destination .\Module\src\bin\Release -Force
Compress-Archive -Path .\Module\src\bin\Release\* -DestinationPath .\WindowsUtilsModule.zip

## Publishing module
try {
    Publish-Module -Path Module\src\bin\Release -NuGetApiKey $PsgSec -Repository PSGallery
}
catch { }