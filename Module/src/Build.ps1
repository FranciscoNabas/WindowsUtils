param ($psgsec)

## Building project
. dotnet build Module\src\WindowsUtilsModule.csproj --arch x64 --configuration Release --output Module\src\bin\Release

## Copying license and compressing output
Write-Output 'Copying license...'
Copy-Item -Path .\LICENSE -Destination .\Module\src\bin\Release -Force
Write-Output 'Compressing artifact...'
Compress-Archive -Path .\Module\src\bin\Release\* -DestinationPath .\WindowsUtilsModule.zip

## Publishing module
try {
    Write-Output 'Trying to publish module...'
    Publish-Module -Path Module\src\bin\Release -NuGetApiKey $psgsec -Repository PSGallery
}
catch { Write-Output "Failed to publish module. $($PSItem.Exception.Message)" }